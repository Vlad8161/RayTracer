#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>

#include "ImageBitmap.h"
#include "Scene.h"
#include "json.h"

using Json = nlohmann::json;

void render(const ImageBitmap &img);
Scene loadScene(const std::string scenePath);

int main() {
    if (!glfwInit()) {
        std::cerr << "Error initializing glfw" << std::endl;
        return 1;
    }

    glfwDefaultWindowHints();

    GLFWwindow *mainWindow = glfwCreateWindow(300, 300, "Ray Tracing", nullptr, nullptr);
    if (mainWindow == nullptr) {
        std::cerr << "Error creating window" << std::endl;
        glfwTerminate();
        return 1;
    }

    loadScene("/home/vlad/projects/blender/exported_scene_hello/");

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1);
    glfwShowWindow(mainWindow);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DOUBLEBUFFER);
    glDepthFunc(GL_LESS);

    glClearColor(1, 1, 1, 1);

    ImageBitmap img(300, 300);
    while (glfwWindowShouldClose(mainWindow) == GL_FALSE) {
        render(img);
        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
    }

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
    return 0;
}

void render(const ImageBitmap &img) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawPixels(img.getWidth(), img.getHeight(), GL_RGB, GL_UNSIGNED_BYTE, img.getRawData());
}

Scene loadScene(const std::string &scenePath) {
    std::ifstream is(scenePath + "index");
    Json j;
    is >> j;

    for (Json& i : j) {
        std::string type = i["type"];
        std::string name = i["name"];
        if (type == "MESH") {

        }

        std::cout << type << std::endl;
    }

    return Scene();
}

std::vector<glm::vec3[3]> loadMesh(const std::string &filePath) {
    std::vector<glm::vec3> vertices;
    std::vector<int[3]> faces;
    std::vector<glm::vec3[3]> retVal;

    std::ifstream is(filePath);

    std::string line;
    while (std::getline(is, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;
        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            vertices.push_back(glm::vec3(x, y, z));
        } else if (type == "f") {
            int v1, v2, v3;
            ss >> v1 >> v2 >> v3;
            faces.push_back({v1, v2, v3});
        }
    }

    for (auto& i : faces) {
        glm::vec3 v1 = vertices[i[0]];
        glm::vec3 v2 = vertices[i[1]];
        glm::vec3 v3 = vertices[i[2]];
        retVal.push_back({v1, v2, v3});
    }

    return retVal;
}
