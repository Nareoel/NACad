#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/Camera.h"
#include "core/MouseLook.h"
#include "Model.h"
#include "RenderMesh.h"
#include "ShaderProgram.h"

struct GLFWwindow;

namespace Viewer {

class App {
   public:
    App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    ~App();

    bool init();
    void run();
    // safe to call more than once; also run by the destructor
    void shutdown();

    // forwarded from the GLFW callbacks
    void onFramebufferSize(int width, int height);
    void onKey(int key, int action);
    void onMouseMove(double xPos, double yPos);
    void onScroll(double yOffset);

   private:
    GLFWwindow* window_{nullptr};
    bool glfwInitialized_{false};
    bool imGuiInitialized_{false};
    std::filesystem::path assetRoot_;
    // ImGui keeps only a pointer to this string
    std::string imGuiIniPath_;

    Camera camera_;
    MouseLook mouseLook_;
    double lastFrameTime_{0.0};
    int framebufferWidth_{0};
    int framebufferHeight_{0};
    bool interactiveMode_{false};

    bool globalLightOn_{true};
    bool pointLightOn_{true};
    bool spotLightOn_{true};
    GlobalLight globalLight_;
    std::vector<PointLight> pointLights_;
    SpotLight spotLight_;

    TextureID containerTextureId_{0};
    Material containerMaterial_;
    Material lightSourceMaterial_;
    std::optional<ShaderProgram> shaderProgram_;
    std::unique_ptr<RenderMesh> cubeMesh_;
    std::optional<Model> backpackModel_;
    std::optional<Model> spiderModel_;

    bool initWindow_();
    bool initScene_();
    void destroyScene_();
    void initImGui_();
    void drawImGui_();
    void shutdownImGui_();
    void applyCursorMode_();
    void frame_();
    void processInput_();
    void drawScene_();
};

}  // namespace Viewer
