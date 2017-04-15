#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>

#include "ImageBitmap.h"
#include "Scene.h"
#include "json.h"

#define WIDTH (640)
#define HEIGHT (480)

using Json = nlohmann::json;

void render(const ImageBitmap &img);

void renderScene(const Scene &scene, ImageBitmap &viewport);

std::shared_ptr<Scene> loadScene(const std::string &scenePath);

std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> loadMesh(const std::string &filePath);

std::ostream &operator<<(std::ostream &os, glm::mat3 x);

std::ostream &operator<<(std::ostream &os, glm::vec3 x);

std::array<float, 3>
computeLight(const glm::vec3 &point, const glm::vec3 &norm, const std::vector<std::shared_ptr<SceneObject>> objects,
             const std::vector<std::shared_ptr<Lamp>> lamps, const Material &material);

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

    auto scene = loadScene("/home/vlad/projects/blender/exported_scene_hello/");

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1);
    glfwShowWindow(mainWindow);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DOUBLEBUFFER);
    glDepthFunc(GL_LESS);

    glClearColor(1, 1, 1, 1);

    ImageBitmap img(WIDTH, HEIGHT);
    renderScene(*scene, img);
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

std::shared_ptr<Scene> loadScene(const std::string &scenePath) {
    std::ifstream is(scenePath + "index");
    std::shared_ptr<Scene> retVal(new Scene);
    Json j;
    is >> j;

    for (Json &i : j) {
        std::string type = i["type"];
        std::string name = i["name"];
        if (type == "MESH") {
            std::string objFilePath = scenePath + name + ".obj";
            auto translate = i["translate"];
            auto transform = i["transform"];
            auto mesh = loadMesh(objFilePath);
            glm::vec3 pos(static_cast<float>(translate[0]), static_cast<float>(translate[1]),
                          static_cast<float>(translate[2]));
            glm::mat3 mat(
                    static_cast<float>(transform[0][0]), static_cast<float>(transform[0][1]),
                    static_cast<float>(transform[0][2]),
                    static_cast<float>(transform[1][0]), static_cast<float>(transform[1][1]),
                    static_cast<float>(transform[1][2]),
                    static_cast<float>(transform[2][0]), static_cast<float>(transform[2][1]),
                    static_cast<float>(transform[2][2])
            );
            Material material(1, 1, 1, 1);
            retVal->objects.push_back(std::shared_ptr<SceneObject>(new MeshObject(material, mesh, pos, mat)));
            std::cout << "mesh : pos = " << pos << " mat = " << mat << std::endl;
        } else if (type == "CAMERA") {
            auto translate = i["translate"];
            auto transform = i["transform"];
            glm::vec3 pos(static_cast<float>(translate[0]), static_cast<float>(translate[1]),
                          static_cast<float>(translate[2]));
            glm::mat3 mat(
                    static_cast<float>(transform[0][0]), static_cast<float>(transform[0][1]),
                    static_cast<float>(transform[0][2]),
                    static_cast<float>(transform[1][0]), static_cast<float>(transform[1][1]),
                    static_cast<float>(transform[1][2]),
                    static_cast<float>(transform[2][0]), static_cast<float>(transform[2][1]),
                    static_cast<float>(transform[2][2])
            );
            retVal->camPos = pos;
            retVal->camMat = mat;
            std::cout << "camera : pos = " << pos << " dir = " << mat << std::endl;
        } else if (type == "LAMP") {
            auto translate = i["translate"];
            glm::vec3 pos(static_cast<float>(translate[0]), static_cast<float>(translate[1]),
                          static_cast<float>(translate[2]));
            retVal->lamps.push_back(std::shared_ptr<Lamp>(new Lamp(pos, 25)));
        }
    }

    return retVal;
}

std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> loadMesh(const std::string &filePath) {
    std::vector<glm::vec3> vertices;
    std::vector<std::array<int, 3>> faces;
    std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> retVal(new std::vector<std::array<glm::vec3, 3>>);

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
            std::array<int, 3> face = {v1, v2, v3};
            faces.push_back(face);
        }
    }

    for (auto &i : faces) {
        glm::vec3 v1 = vertices[i[0]];
        glm::vec3 v2 = vertices[i[1]];
        glm::vec3 v3 = vertices[i[2]];
        std::array<glm::vec3, 3> face = {v1, v2, v3};
        retVal->push_back(face);
    }

    return retVal;
}

void renderScene(const Scene &scene, ImageBitmap &viewport) {
    auto width = viewport.getWidth();
    auto height = viewport.getHeight();
    float camHeight = 1.0;
    float camWidth = static_cast<float>(width) * (camHeight / static_cast<float>(height));
    float camDist = 1.0;
    float dh = camHeight / static_cast<float>(height);
    float dw = camWidth / static_cast<float>(width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float rayX = -(camWidth / 2) + j * dw;
            float rayY = -(camHeight / 2) + i * dh;
            glm::vec3 rayCamDir(rayX, rayY, -camDist);
            glm::vec3 rayWorldDir = rayCamDir * scene.camMat;
            viewport.setPixel(j, i, 0, 0, 0);
            bool foundIntersection = false;
            float foundT = 0.0;
            std::shared_ptr<SceneObject> foundObj;
            for (auto &obj : scene.objects) {
                auto intersection = obj->intersection(scene.camPos, rayWorldDir);
                if (std::get<0>(intersection)) {
                    if (!foundIntersection || std::get<1>(intersection) > foundT) {
                        foundIntersection = true;
                        foundT = std::get<1>(intersection);
                        foundObj = obj;
                    }
                }
                if (foundIntersection) {
                    auto ptColor = computeLight(std::get<2>(intersection).point,
                                                glm::normalize(std::get<2>(intersection).norm),
                                                scene.objects, scene.lamps, obj->material);
                    uint8_t r = static_cast<uint8_t>(std::min(255.0f, std::get<0>(ptColor)));
                    uint8_t g = static_cast<uint8_t>(std::min(255.0f, std::get<1>(ptColor)));
                    uint8_t b = static_cast<uint8_t>(std::min(255.0f, std::get<2>(ptColor)));
                    //std::cout << (int) r << " " << (int) g << " " << (int) b << std::endl;
                    viewport.setPixel(j, i, r, g, b);
                }
            }
        }
    }
}


std::array<float, 3>
computeLight(const glm::vec3 &point, const glm::vec3 &norm, const std::vector<std::shared_ptr<SceneObject>> objects,
             const std::vector<std::shared_ptr<Lamp>> lamps, const Material &material) {
    float retR = 0.0;
    float retG = 0.0;
    float retB = 0.0;
    for (auto &lamp : lamps) {
        bool inShade = false;
        for (auto &obj : objects) {
            if (std::get<0>(obj->intersection(point, lamp->pos - point))) {
                inShade = true;
            }
        }
        if (!inShade) {
            std::cout << "lala" << std::endl;
            auto toTheLight = glm::normalize(lamp->pos - point);
            auto k = material.diffusiveIntensity * glm::dot(toTheLight, norm);
            if (k > 0) {
                retR += material.diffusiveColorRed * k;
                retG += material.diffusiveColorGreen * k;
                retB += material.diffusiveColorBlue * k;
            }
        }
    }
    return {retR, retG, retB};
};


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
