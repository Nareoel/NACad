#include "App.h"
#include "core/AssetPaths.h"
#include "core/Projection.h"
#include "GlApi.h"
#include "Utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <cstdlib>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#endif

namespace {
constexpr int cWindowWidth{1600};
constexpr int cWindowHeight{1200};
constexpr int cPointLightsNumber{4};  // must equal NR_POINT_LIGHTS in shaders/shader.fs
constexpr int cCubeNumberXYPlane{6};
constexpr double cCubePositionRadius{3.0};

const glm::vec3 cDefaultGlobalLightColor{1.0f, 0.925f, 0.5568f};
// point light is the same color as global because light cubes shares one shader
const glm::vec3 cDefaultPointLightColor{cDefaultGlobalLightColor};
const glm::vec3 cDefaultSpotLightColor{1.0f, 1.0f, 1.0f};

#ifdef __EMSCRIPTEN__
constexpr const char* cImGuiGlslVersion{"#version 300 es"};
// pointer lock needs a user gesture, so the page starts with a free cursor
constexpr bool cStartInteractive{true};
#else
constexpr const char* cImGuiGlslVersion{"#version 330"};
constexpr bool cStartInteractive{false};
#endif

// empty if the platform can't tell; findAssetRoot then falls back to the CWD
std::filesystem::path sExecutablePath() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length == buffer.size()) {
        return {};
    }
    buffer.resize(length);
    return std::filesystem::path(buffer);
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    buffer.resize(std::char_traits<char>::length(buffer.c_str()));
    std::error_code ec;
    auto path = std::filesystem::canonical(buffer, ec);
    return ec ? std::filesystem::path{} : path;
#elif defined(__linux__)
    std::error_code ec;
    auto path = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path{} : path;
#else
    return {};
#endif
}

Viewer::App* sApp(GLFWwindow* window) { return static_cast<Viewer::App*>(glfwGetWindowUserPointer(window)); }

void sFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    sApp(window)->onFramebufferSize(width, height);
}

void sKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    sApp(window)->onKey(key, action);
}

void sMouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }
    sApp(window)->onMouseMove(xPos, yPos);
}

void sScrollCallback(GLFWwindow* window, double /*xOffset*/, double yOffset) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }
    sApp(window)->onScroll(yOffset);
}

#ifdef __EMSCRIPTEN__
// the browser releases the pointer lock on its own (Esc), so go back to interactive mode when it does
bool sPointerLockChangeCallback(int /*eventType*/, const EmscriptenPointerlockChangeEvent* event,
                                void* userData) {
    if (!event->isActive) {
        static_cast<Viewer::App*>(userData)->setInteractiveMode(true);
    }
    return false;
}

bool sPointerLockErrorCallback(int /*eventType*/, const void* /*reserved*/, void* userData) {
    static_cast<Viewer::App*>(userData)->setInteractiveMode(true);
    return false;
}
#endif

}  // namespace

namespace Viewer {

App::~App() { shutdown(); }

bool App::init() {
    std::error_code ec;
    const auto currentDir = std::filesystem::current_path(ec);
    assetRoot_ = findAssetRoot(sExecutablePath(), currentDir, [](const std::filesystem::path& dir) {
        std::error_code existsError;
        return std::filesystem::exists(dir / "shaders" / "shader.vs", existsError);
    });
    std::cout << "Asset root: " << assetRoot_ << std::endl;

    if (!initWindow_()) {
        return false;
    }
    if (!initScene_()) {
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    interactiveMode_ = cStartInteractive;
    applyCursorMode_();
    glfwSetCursorPosCallback(window_, sMouseCallback);
    glfwSetScrollCallback(window_, sScrollCallback);
    glfwSetKeyCallback(window_, sKeyCallback);
#ifdef __EMSCRIPTEN__
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, false,
                                              sPointerLockChangeCallback);
    emscripten_set_pointerlockerror_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, false,
                                             sPointerLockErrorCallback);
#endif
    // after our callbacks: the ImGui backend chains to the callbacks installed before it
    initImGui_();
    return true;
}

void App::run() {
    if (window_ == nullptr || !shaderProgram_) {
        return;
    }
#ifdef __EMSCRIPTEN__
    // the browser drives the frames; with simulate_infinite_loop this call does not return
    emscripten_set_main_loop_arg([](void* app) { static_cast<App*>(app)->frame_(); }, this, 0, true);
#else
    while (!glfwWindowShouldClose(window_)) {
        frame_();
    }
#endif
}

void App::shutdown() {
    if (window_ != nullptr) {
        // GL objects must be freed while the context still exists
        glfwMakeContextCurrent(window_);
        destroyScene_();
        shutdownImGui_();
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

void App::onFramebufferSize(int width, int height) {
    framebufferWidth_ = width;
    framebufferHeight_ = height;
    glViewport(0, 0, width, height);
}

void App::onKey(int key, int action) {
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_G) {
        globalLightOn_ = !globalLightOn_;
    }
    if (key == GLFW_KEY_P) {
        pointLightOn_ = !pointLightOn_;
    }
    if (key == GLFW_KEY_Q) {
        spotLightOn_ = !spotLightOn_;
    }
    if (key == GLFW_KEY_I) {
        setInteractiveMode(!interactiveMode_);
    }
}

void App::setInteractiveMode(bool interactive) {
    interactiveMode_ = interactive;
    // the cursor moves freely in interactive mode, so its last position is stale after the switch
    mouseLook_.reset();
    applyCursorMode_();
}

void App::onMouseMove(double xPos, double yPos) {
    if (interactiveMode_) {
        return;  // the cursor belongs to ImGui in interactive mode
    }
    if (const auto delta = mouseLook_.delta(xPos, yPos)) {
        camera_.processMouse(delta->x, delta->y);
    }
}

void App::onScroll(double yOffset) { camera_.processScroll(yOffset); }

void App::applyCursorMode_() {
    glfwSetInputMode(window_, GLFW_CURSOR, interactiveMode_ ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool App::initWindow_() {
#ifndef __EMSCRIPTEN__
    // NACAD_GLFW_PLATFORM=x11 forces X11 (XWayland) on a Wayland session, e.g. to compare resize behaviour
    if (const char* platform = std::getenv("NACAD_GLFW_PLATFORM");
        platform != nullptr && std::string_view(platform) == "x11") {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }
#endif
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwInitialized_ = true;
#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    window_ = glfwCreateWindow(cWindowWidth, cWindowHeight, "NACad", nullptr, nullptr);
    if (window_ == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        return false;
    }
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, sFramebufferSizeCallback);
    glfwMakeContextCurrent(window_);
    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);

#ifdef __EMSCRIPTEN__
    GLint maxFragmentUniformVectors{0};
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
    std::cout << "OpenGL renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
              << "OpenGL version: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n'
              << "Max fragment uniform vectors: " << maxFragmentUniformVectors << std::endl;
#else
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    std::cout << "OpenGL renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
              << "GLFW platform: " << (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND ? "Wayland" : "X11")
              << std::endl;
#endif
    return true;
}

bool App::initScene_() {
    globalLight_.color = cDefaultGlobalLightColor;
    globalLight_.position = glm::vec3(10.0, 10.0, 10.0);

    pointLights_.assign(cPointLightsNumber, PointLight{});
    const glm::vec3 pointLightShift{0.0, 2.0, 0.0};
    for (int i = 0; i < cPointLightsNumber; ++i) {
        auto& pointLight = pointLights_[i];
        pointLight.color = cDefaultPointLightColor;
        pointLight.position = glm::vec3(0.0, 2.0, 0.0) + pointLightShift * static_cast<float>(i);
    }

    {
        const auto diffuseImage = assetRoot_ / "samples" / "container.png";
        const auto specularImage = assetRoot_ / "samples" / "containerMetalBorder.png";
        const auto emissionImage = assetRoot_ / "samples" / "matrix.jpg";
        auto containerTextureInfo =
            Utils::createTextureFromImages({diffuseImage, specularImage, emissionImage});
        containerTextureId_ = containerTextureInfo.first;
        if (containerTextureId_ != 0) {
            TextureData textureData;
            textureData.id = containerTextureInfo.first;
            textureData.textures = {
                {TextureType::Diffuse, {containerTextureInfo.second[diffuseImage]}},
                {TextureType::Specular, {containerTextureInfo.second[specularImage]}},
                {TextureType::Emission, {containerTextureInfo.second[emissionImage]}},
            };
            containerMaterial_.textureData = std::move(textureData);
        }
        containerMaterial_.color = glm::vec3(0, 0, 0);
        containerMaterial_.shininess = 1024;
    }
    lightSourceMaterial_.color = cDefaultGlobalLightColor;

    shaderProgram_ = ShaderProgram::createShaderProgram(assetRoot_ / "shaders" / "shader.vs",
                                                        assetRoot_ / "shaders" / "shader.fs");
    if (!shaderProgram_) {
        return false;
    }
    cubeMesh_ = createCubeRenderMesh(Material());

#ifndef __EMSCRIPTEN__
    backpackModel_.emplace(assetRoot_ / "samples" / "backpack" / "backpack.obj");

    spiderModel_.emplace(assetRoot_ / "samples" / "spider" / "Only_Spider_with_Animations_Export.obj");
    spiderModel_->setModelTr(
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -2.0f, 0.0f)), glm::vec3(0.02f)));
#endif
    return true;
}

void App::destroyScene_() {
#ifndef __EMSCRIPTEN__
    spiderModel_.reset();
    backpackModel_.reset();
#endif
    cubeMesh_.reset();
    shaderProgram_.reset();
    containerMaterial_.textureData.reset();
    if (containerTextureId_ != 0) {
        glDeleteTextures(1, &containerTextureId_);
        containerTextureId_ = 0;
    }
}

void App::initImGui_() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imGuiIniPath_ = (assetRoot_ / "imgui.ini").string();
    ImGui::GetIO().IniFilename = imGuiIniPath_.c_str();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window_, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(cImGuiGlslVersion);
    imGuiInitialized_ = true;
}

void App::drawImGui_() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello, world!");
    ImGui::Text("This is a simple window!");
    const float framerate = ImGui::GetIO().Framerate;
    ImGui::Text("%.1f FPS (%.2f ms/frame)", framerate, framerate > 0.0f ? 1000.0f / framerate : 0.0f);
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::shutdownImGui_() {
    if (!imGuiInitialized_) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    imGuiInitialized_ = false;
}

void App::frame_() {
    processInput_();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene_();
    drawImGui_();

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void App::processInput_() {
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, true);
    }
    if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_PRESS) {
        camera_.resetView();
    }
    const double now = glfwGetTime();
    const double deltaTime = now - lastFrameTime_;
    lastFrameTime_ = now;
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Forward, deltaTime);
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Backward, deltaTime);
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Right, deltaTime);
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Left, deltaTime);
    }
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Down, deltaTime);
    }
    if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera_.processKeyboard(Camera::MovementType::Up, deltaTime);
    }
}

void App::drawScene_() {
    auto& shader = *shaderProgram_;
    shader.use();
    globalLight_.color = globalLightOn_ ? cDefaultGlobalLightColor : glm::vec3(0.0);
    shader.setUniform("globalLight", globalLight_);
    for (int i = 0; i < cPointLightsNumber; ++i) {
        auto& pointLight = pointLights_[i];
        pointLight.color = pointLightOn_ ? cDefaultPointLightColor : glm::vec3(0.0);
        shader.setUniform("pointlights[" + std::to_string(i) + "]", pointLight);
    }
    spotLight_.color = spotLightOn_ ? cDefaultSpotLightColor : glm::vec3(0);
    spotLight_.position = camera_.position();
    spotLight_.direction = camera_.front();
    shader.setUniform("spotLight", spotLight_);

    shader.setUniform("viewPosition", camera_.position());
    shader.setUniform("viewTr", camera_.viewMatrix());
    shader.setUniform("projectionTr",
                      perspective(camera_.fieldOfView(), aspectRatio(framebufferWidth_, framebufferHeight_)));
    shader.setUniform("time", static_cast<float>(glfwGetTime()));

    // containers
    cubeMesh_->setMaterial(containerMaterial_);
    for (int i = 1; i <= cCubeNumberXYPlane; i++) {
        const double angle = 2 * double(i) * std::numbers::pi / cCubeNumberXYPlane;
        auto modelTr =
            glm::translate(glm::mat4(1.0f), glm::vec3(cCubePositionRadius * std::cos(angle),
                                                      cCubePositionRadius * std::sin(angle), double(i) / 2));
        cubeMesh_->setModelTr(modelTr);
        cubeMesh_->draw(shader);
    }

    // global light source
    cubeMesh_->resetModelTr();
    cubeMesh_->setModelTr(glm::translate(glm::mat4(1.0f), globalLight_.position));
    lightSourceMaterial_.color = globalLight_.color;
    cubeMesh_->setMaterial(lightSourceMaterial_);
    cubeMesh_->draw(shader);

    // point light sources
    cubeMesh_->resetModelTr();
    cubeMesh_->setLocalTr(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)));
    for (int i = 0; i < cPointLightsNumber; ++i) {
        lightSourceMaterial_.color = pointLights_[i].color;
        cubeMesh_->setMaterial(lightSourceMaterial_);
        cubeMesh_->setModelTr(glm::translate(glm::mat4(1.0f), pointLights_[i].position));
        cubeMesh_->draw(shader);
    }

    cubeMesh_->resetLocalTr();
    cubeMesh_->resetModelTr();

#ifndef __EMSCRIPTEN__
    backpackModel_->draw(shader);
    spiderModel_->draw(shader);
#endif
}

}  // namespace Viewer
