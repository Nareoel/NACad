
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <nfd.h>

#include "ShaderProgram.h"
#include "Mesh.h"
#include "Camera.h"
#include "Utils.h"
#include "Model.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <functional>
#include <math.h>

namespace {
// set GLViewport if user changes screen size
void sFramebufferSizeCallback(GLFWwindow* window, int width, int height);
// catch key released callback
void sProcessInput(GLFWwindow* window);
// catch key pressed once callback
void sLightsInputKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
// catch mouse callbacks
void sMouseCallback(GLFWwindow* window, double xPos, double yPos);
void sScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

// Initialize ImGui
void sSetupImGui(GLFWwindow* window);

// Cleanup ImGui
void sCleanupImGui();

// Draw ImGui frame
void sRenderImGui();

}  // namespace

// ToDo remove global variables
Camera camera;
bool firstWindowFocus{true};
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;
float lastXPos = 800 / 2;
float lastYPos = 600 / 2;

bool globalLightOn{true};
glm::vec3 defaultGlobalLightColor = glm::vec3(1.0f, 0.925f, 0.5568f);
bool pointLightOn{true};
// point light is the same color as global because light cubes shares one shader
glm::vec3 defaultPointLightColor = defaultGlobalLightColor;  // glm::vec3(0.8906f, 0.4375f, 0.144531f);
bool spotLightOn{true};
glm::vec3 defaultSpotLightColor = glm::vec3(1.0f, 1.0f, 1.f);

bool interactiveMode{false};

const int cPointLightsNumber{4};

int main() {
    // GLFW initialization -- addon to OpenGL to manages windows
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // window creation
    GLFWwindow* window = glfwCreateWindow(1600, 1200, "LearnOpenGL", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, sFramebufferSizeCallback);
    glfwMakeContextCurrent(window);

    // GLAD initialization - manages function pointers to OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GlobalLight globalLight;
    globalLight.color = defaultGlobalLightColor;
    globalLight.position = glm::vec3(10.0, 10.0, 10.0);

    std::vector<PointLight> pointLights(cPointLightsNumber);
    const glm::vec3 pointLightShift{glm::vec3(0.0, 2.0, 0.0)};
    for (int i = 0; i < cPointLightsNumber; ++i) {
        auto& pointLight = pointLights[i];
        pointLight.color = defaultPointLightColor;
        pointLight.position = glm::vec3(0.0, 2.0, 0.0) + pointLightShift * static_cast<float>(i);
    }

    SpotLight spotLight;

    Material containerMaterial;
    {
        std::unordered_set<std::filesystem::path> containerImages{
            "samples/container.png",
            "samples/containerMetalBorder.png",
            "samples/matrix.jpg",
        };
        auto containerTextureInfo = Utils::createTextureFromImages(containerImages);
        TextureData textureData;
        textureData.id = containerTextureInfo.first;
        textureData.textures = {
            {TextureType::Diffuse, {containerTextureInfo.second["samples/container.png"]}},
            {TextureType::Specular, {containerTextureInfo.second["samples/containerMetalBorder.png"]}},
            {TextureType::Emission, {containerTextureInfo.second["samples/matrix.jpg"]}},
        };
        containerMaterial.textureData = std::move(textureData);
        containerMaterial.color = glm::vec3(0, 0, 0);
        containerMaterial.shininess = 1024;
    }
    Material lightSourceMaterial;
    {
        lightSourceMaterial.color = defaultGlobalLightColor;
    }

    auto shaderProgram = ShaderProgram::createShaderProgram("shaders/shader.vs", "shaders/shader.fs");
    if (!shaderProgram) {
        return 0;
    }
    auto cubeMesh = createCubeMesh(Material());

    Model backpackModel("samples/backpack/backpack.obj");

    Model spiderModel("samples/spider/Only_Spider_with_Animations_Export.obj");
    spiderModel.setModelTr(
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -2.0f, 0.0f)), glm::vec3(0.02f)));

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, sMouseCallback);
    glfwSetScrollCallback(window, sScrollCallback);

    glfwSetKeyCallback(window, sLightsInputKeyCallback);
    sSetupImGui(window);
    while (!glfwWindowShouldClose(window)) {
        // catch key released callbacks
        sProcessInput(window);

        if (interactiveMode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPosCallback(window, nullptr);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(window, sMouseCallback);
        }
        // --------------- actual render here
        // set some color. This color will be setted each time we call glClear
        // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram->use();
        globalLight.color = globalLightOn ? defaultGlobalLightColor : glm::vec3(0.0);
        shaderProgram->setUniform("globalLight", globalLight);
        for (int i = 0; i < cPointLightsNumber; ++i) {
            auto& pointLight = pointLights[i];

            pointLight.color = pointLightOn ? defaultPointLightColor : glm::vec3(0.0);
            auto uniformName = "pointlights[" + std::to_string(i) + "]";
            shaderProgram->setUniform(uniformName, pointLight);
        }
        spotLight.color = spotLightOn ? defaultSpotLightColor : glm::vec3(0);
        spotLight.position = camera.position();
        spotLight.direction = camera.front();
        shaderProgram->setUniform("spotLight", spotLight);

        shaderProgram->setUniform("viewPosition", camera.position());
        shaderProgram->setUniform("viewTr", camera.viewMatrix());
        shaderProgram->setUniform("projectionTr", glm::perspective(glm::radians(camera.fieldOfView()),
                                                                   800.0f / 600.0f, 0.1f, 100.0f));
        shaderProgram->setUniform("time", float(glfwGetTime()));

        // containers
        cubeMesh->setMaterial(containerMaterial);
        int cubeNumberXYPlane = 6;
        double positionRadius = 3;
        for (int i = 1; i <= cubeNumberXYPlane; i++) {
            auto modelTr = glm::translate(
                glm::mat4(1.0f),
                glm::vec3(positionRadius * cos(2 * double(i) * M_PI / cubeNumberXYPlane),
                          positionRadius * sin(2 * double(i) * M_PI / cubeNumberXYPlane), double(i) / 2));
            cubeMesh->setModelTr(modelTr);
            cubeMesh->draw(*shaderProgram);
        }

        // global light source
        cubeMesh->resetModelTr();
        cubeMesh->setModelTr(glm::translate(glm::mat4(1.0f), globalLight.position));
        lightSourceMaterial.color = globalLight.color;
        cubeMesh->setMaterial(lightSourceMaterial);
        cubeMesh->draw(*shaderProgram);

        // point light sources
        cubeMesh->resetModelTr();
        cubeMesh->setLocalTr(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)));
        for (int i = 0; i < cPointLightsNumber; ++i) {
            lightSourceMaterial.color = pointLights[i].color;
            cubeMesh->setMaterial(lightSourceMaterial);
            cubeMesh->setModelTr(glm::translate(glm::mat4(1.0f), pointLights[i].position));
            cubeMesh->draw(*shaderProgram);
        }

        cubeMesh->resetLocalTr();
        cubeMesh->resetModelTr();

        backpackModel.draw(*shaderProgram);
        spiderModel.draw(*shaderProgram);

        sRenderImGui();

        // swap front and back buffers
        glfwSwapBuffers(window);
        // check if any event triggered, like keyboard pressed or mose movement
        glfwPollEvents();
    }

    sCleanupImGui();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

namespace {

void sFramebufferSizeCallback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void sLightsInputKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Only triggered once per key press
        if (key == GLFW_KEY_G) {
            globalLightOn = !globalLightOn;
        }
        if (key == GLFW_KEY_P) {
            pointLightOn = !pointLightOn;
        }
        if (key == GLFW_KEY_Q) {
            spotLightOn = !spotLightOn;
        }
        if (key == GLFW_KEY_I) {
            interactiveMode = !interactiveMode;
        }
    }
}

void sProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        camera.resetView();
    }
    deltaTime = glfwGetTime() - lastFrameTime;
    lastFrameTime = glfwGetTime();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Forward, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Backward, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Right, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Left, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Down, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.processKeyboard(Camera::MovementType::Up, deltaTime);
    }
}

void sMouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }

    if (firstWindowFocus) {
        lastXPos = xPos;
        lastYPos = yPos;
        firstWindowFocus = false;
    }

    auto xOffset = xPos - lastXPos;
    auto yOffset = yPos - lastYPos;
    lastXPos = xPos;
    lastYPos = yPos;

    camera.processMouse(xOffset, yOffset);
}

void sScrollCallback(GLFWwindow* window, double, double yOffset) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }
    camera.processScroll(yOffset);
}

void sSetupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void sCleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void sRenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Example window
    ImGui::Begin("Hello, world!");
    ImGui::Text("This is a simple window!");
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace
