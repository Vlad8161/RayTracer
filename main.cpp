#include "config.h"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>

#include "image_bitmap.h"
#include "synchronized_queue.h"
#include "scene.h"
#include "lib/json.h"

void render(const image_bitmap &img);

std::ostream &operator<<(std::ostream &os, glm::mat3 x);

std::ostream &operator<<(std::ostream &os, glm::vec3 x);

int main() {
    std::shared_ptr<Scene> scene(new Scene());
    std::shared_ptr<image_bitmap> img(new image_bitmap(WIDTH, HEIGHT));
    std::shared_ptr<synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>> queue(
            new synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>());
    loadScene(*scene, "/home/vlad/projects/blender/hello.scene");

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

#ifdef RENDER_PARALLEL
    bool timePrinted = false;
    int renderTimesLeft = RENDER_COUNT;
    std::shared_ptr<bool> finishedFlag(new bool(true));
    std::vector<long> durations;
    auto start = std::chrono::high_resolution_clock::now();

    while (glfwWindowShouldClose(mainWindow) == GL_FALSE) {
        if (*finishedFlag && renderTimesLeft > 0) {
            img->clear();
            finishedFlag = renderParallel(scene, queue, WIDTH, HEIGHT);
            start = std::chrono::high_resolution_clock::now();
            renderTimesLeft--;
        }

        if (*finishedFlag) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            durations.push_back(duration);
        }

        if (*finishedFlag && renderTimesLeft == 0 && !timePrinted) {
            long sum = std::accumulate(durations.begin(), durations.end(), 0);
            std::cout << "Время выполнения: " << static_cast<double>(sum) / durations.size() << std::endl;
            timePrinted = true;
        }

        try {
            auto subBlock = queue->pop_back();
            img->copyImageTo(std::get<0>(subBlock), std::get<1>(subBlock), *std::get<2>(subBlock));
            render(*img);
        } catch (std::out_of_range &e) {
            // unused
        }

        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
    }
#else
    std::vector<long> durations;
#ifdef GPU_ACCELERATION
    std::shared_ptr<OpenClExecutor> clExecutor(new OpenClExecutor(*scene));
#else
    std::shared_ptr<OpenClExecutor> clExecutor(nullptr);
#endif
    for (int i = 0; i < RENDER_COUNT; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        renderScene(*img, *scene, clExecutor);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        durations.push_back(duration);
    }
    long sum = std::accumulate(durations.begin(), durations.end(), 0);
    std::cout << "Время выполнения: " << static_cast<double>(sum) / durations.size() << std::endl;

    while (glfwWindowShouldClose(mainWindow) == GL_FALSE) {
        render(*img);
        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
    }
#endif

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
    return 0;
}


void render(const image_bitmap &img) {
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
