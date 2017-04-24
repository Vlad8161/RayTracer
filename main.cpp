#include <iostream>
#include <chrono>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>

#include "ImageBitmap.h"
#include "Scene.h"
#include "json.h"

#define WIDTH (800)
#define HEIGHT (600)
#define RENDER_COUNT (1)

void render(const ImageBitmap &img);

std::ostream &operator<<(std::ostream &os, glm::mat3 x);

std::ostream &operator<<(std::ostream &os, glm::vec3 x);

int main() {
    if (!glfwInit()) {
        std::cerr << "Error initializing glfw" << std::endl;
        return 1;
    }

    glfwDefaultWindowHints();

    GLFWwindow *mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Ray Tracing", nullptr, nullptr);
    if (mainWindow == nullptr) {
        std::cerr << "Error creating window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1);
    glfwShowWindow(mainWindow);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DOUBLEBUFFER);
    glDepthFunc(GL_LESS);

    glClearColor(1, 1, 1, 1);

    Scene scene;
    loadScene(scene, "/home/vlad/projects/blender/hello.scene");
    ImageBitmap img(WIDTH, HEIGHT);

    std::vector<long> durations;
    for (int i = 0; i < RENDER_COUNT; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        renderScene(img, scene);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        durations.push_back(duration);
    }
    long sum = std::accumulate(durations.begin(), durations.end(), 0);
    std::cout << "Время выполнения: " << static_cast<double>(sum) / durations.size() << std::endl;

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
    glDrawPixels(img.getWidth(), img.getHeight(), GL_RGB, GL_FLOAT, img.getRawData());
}


std::ostream &operator<<(std::ostream &os, glm::vec3 x) {
    os << "{" << x[0] << " " << x[1] << " " << x[2] << "}";
    return os;
}

std::ostream &operator<<(std::ostream &os, glm::mat3 x) {
    os << "{" << std::endl
       << "    " << x[0][0] << " " << x[0][1] << " " << x[0][2] << std::endl
       << "    " << x[1][0] << " " << x[1][1] << " " << x[1][2] << std::endl
       << "    " << x[2][0] << " " << x[2][1] << " " << x[2][2] << std::endl
       << "}";
    return os;
}
