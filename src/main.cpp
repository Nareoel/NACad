
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShaderProgram.h"
#include "Mesh.h"
#include "camera.h"
#include "Utils.h"
#include "Model.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <functional>
#include <math.h>

// set GLViewport if user changes screen size
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// catch key released callback
void processInput(GLFWwindow* window);
// catch key pressed once callback
void lightsInputkeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
// catch mouse callbacks
void mouseCallback(GLFWwindow* window, double xPos, double yPos);
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

// ToDo remove global variables
Camera camera;
bool firstWidowFocus{true};
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;
float lastXPos = 800 / 2;
float lastYPos = 600 / 2;

bool globalLightOn{true};
glm::vec3 defaultGlobalLightColor = glm::vec3(1.0f, 0.925f, 0.5568f);
bool pointLightOn{true};
// point light is the same color as global because light cubes shares one shader
glm::vec3 defualtPointLightColor = defaultGlobalLightColor;  // glm::vec3(0.8906f, 0.4375f, 0.144531f);
bool spotLightOn{true};
glm::vec3 defualtSpotLightColor = glm::vec3(1.0f, 1.0f, 1.f);

const int cPointLightsNumber{4};

int main() {
    // GLFW initialization -- addon to OpenGL to manages windows
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // window creation
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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
        pointLight.color = defualtPointLightColor;
        pointLight.position = glm::vec3(0.0, 2.0, 0.0) + pointLightShift * static_cast<float>(i);
    }

    SpotLight spotLight;

    Material containerMaterial;
    {
        if (auto diffuseMapId = Utils::loadTexture("samples/container.png")) {
            containerMaterial.textures.emplace_back(Texture{*diffuseMapId, TextureType::Diffuse});
        }
        if (auto specularMapId = Utils::loadTexture("samples/containerMetalBorder.png")) {
            containerMaterial.textures.emplace_back(Texture{*specularMapId, TextureType::Specular});
        }
        if (auto emissionMapId = Utils::loadTexture("samples/matrix.jpg")) {
            containerMaterial.textures.emplace_back(Texture{*emissionMapId, TextureType::Emission});
        }
        containerMaterial.color = glm::vec3(0, 0, 0);
        containerMaterial.shininess = 1024;
    }
    Material lightSourceMaterial;
    { lightSourceMaterial.color = defaultGlobalLightColor; }

    auto shaderProgram = ShaderProgram::createShaderProgram("shaders/shader.vs", "shaders/shader.fs");
    if (!shaderProgram) {
        return 0;
    }
    auto cubeMesh = createCubeMesh(Material());

    Model backpackModel("samples/backpack/backpack.obj");

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glfwSetKeyCallback(window, lightsInputkeyCallback);
    while (!glfwWindowShouldClose(window)) {
        // catch key released callbacks
        processInput(window);

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

            pointLight.color = pointLightOn ? defualtPointLightColor : glm::vec3(0.0);
            auto uniformName = "pointlights[" + std::to_string(i) + "]";
            shaderProgram->setUniform(uniformName, pointLight);
        }
        spotLight.color = spotLightOn ? defualtSpotLightColor : glm::vec3(0);
        spotLight.position = camera.position();
        spotLight.direction = camera.front();
        shaderProgram->setUniform("spotLight", spotLight);

        shaderProgram->setUniform("viewPosition", camera.position());
        shaderProgram->setUniform("viewTr", camera.viewMatrix());
        shaderProgram->setUniform("projectionTr", glm::perspective(glm::radians(camera.fieldOfView()),
                                                                   800.0f / 600.0f, 0.1f, 100.0f));
        shaderProgram->setUniform("time", float(glfwGetTime()));

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

        cubeMesh->resetModelTr();
        cubeMesh->setModelTr(glm::translate(glm::mat4(1.0f), globalLight.position));
        lightSourceMaterial.color = globalLight.color;
        cubeMesh->setMaterial(lightSourceMaterial);
        cubeMesh->draw(*shaderProgram);

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

        // swap front and back buffers
        glfwSwapBuffers(window);
        // check if any event triggered, like keyboard pressed or mose movement
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void lightsInputkeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        camera.resetView();
    }
    deltaTime = glfwGetTime() - lastFrameTime;
    lastFrameTime = glfwGetTime();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::DOWN, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.processKeyboard(Camera::movementType::UP, deltaTime);
    }
}

void mouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }

    if (firstWidowFocus) {
        lastXPos = xPos;
        lastYPos = yPos;
        firstWidowFocus = false;
    }

    auto xOffset = xPos - lastXPos;
    auto yOffset = yPos - lastYPos;
    lastXPos = xPos;
    lastYPos = yPos;

    camera.processMouse(xOffset, yOffset);
}

void scrollCallback(GLFWwindow* window, double, double yOffset) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }
    camera.processScroll(yOffset);
}
