//
// Created by vlad on 4/9/17.
//

#ifndef RAY_TRACING_SCENE_H
#define RAY_TRACING_SCENE_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <iostream>
#include "lodepng.h"
#include "ImageBitmap.h"

#define EPS (0.0001)

typedef struct _Hit {
    bool isHit;
    float t;
    glm::vec3 point;
    glm::vec3 norm;
    glm::vec3 color;

    _Hit(bool isHit = false,
         float t = 0.0f,
         const glm::vec3 &point = glm::vec3(),
         const glm::vec3 &norm = glm::vec3(),
         const glm::vec3 &color = glm::vec3()
    ) : isHit(isHit), t(t), point(point), norm(norm), color(color) {}
} Hit;


class TexImage {
private:
    std::vector<unsigned char> mImageData;
    unsigned int mWidth;
    unsigned int mHeight;
    float scaleX;
    float scaleY;
    bool initialized;

    TexImage() {}

public:
    static std::shared_ptr<TexImage> createImage(const std::string& filename) {
        std::shared_ptr<TexImage> retVal(new TexImage);
        if (lodepng::decode(retVal->mImageData, retVal->mWidth, retVal->mHeight, filename)) {
            retVal.reset();
        }
        return retVal;
    }

    std::vector<unsigned char>* getRawData() {return &mImageData;}
    glm::vec4 get(unsigned int x, unsigned int y) const {
        unsigned realX = x >= mWidth ? x % mWidth : x;
        unsigned realY = y >= mWidth ? y % mHeight : y;
        unsigned index = (unsigned int) (mImageData.size() - (realY + 1) * mHeight * 4 + realX * 4);
        return glm::vec4(
                static_cast<float>(mImageData[index]) / 255.0f,
                static_cast<float>(mImageData[index + 1]) / 255.0f,
                static_cast<float>(mImageData[index + 2]) / 255.0f,
                static_cast<float>(mImageData[index + 3]) / 255.0f
        );
    }

    unsigned int getWidth() const {
        return mWidth;
    }

    unsigned int getHeight() const {
        return mHeight;
    }

    float getScaleX() const {
        return scaleX;
    }

    void setScaleX(float scaleX) {
        TexImage::scaleX = scaleX;
    }

    float getScaleY() const {
        return scaleY;
    }

    void setScaleY(float scaleY) {
        TexImage::scaleY = scaleY;
    }
};


typedef struct _Material {
    float diffusiveIntensity;
    std::shared_ptr<TexImage> texImage;
    glm::vec3 diffusiveColor;

    _Material(float diffusiveIntensity = 0.8, const glm::vec3 &diffusiveColor = glm::vec3(0.8, 0.8, 0.8))
            : diffusiveIntensity(diffusiveIntensity),
              diffusiveColor(diffusiveColor) {}
} Material;


typedef struct _Triangle {
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;
    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec2 uv3;
    std::shared_ptr<Material> material;

    _Triangle(
            const glm::vec3 &p1,
            const glm::vec3 &p2,
            const glm::vec3 &p3,
            const glm::vec2 &uv1,
            const glm::vec2 &uv2,
            const glm::vec2 &uv3,
            const std::shared_ptr<Material> &material)
            : p1(p1), p2(p2), p3(p3), uv1(uv1), uv2(uv2), uv3(uv3), material(material) {}
} Triangle;


typedef struct _Sphere {
    glm::vec3 center;
    float radius;
    std::shared_ptr<Material> material;

    _Sphere(const glm::vec3 &center, float radius, const std::shared_ptr<Material> &material) : center(center),
                                                                                                radius(radius),
                                                                                                material(material) {}
} Sphere;


typedef struct _Lamp {
    glm::vec3 pos;
    float intensity;

    _Lamp(const glm::vec3 &pos, float intensity) : pos(pos), intensity(intensity) {}
} Lamp;


typedef struct _Scene {
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Triangle>> triangles;
    std::vector<std::shared_ptr<Sphere>> spheres;
    std::vector<std::shared_ptr<Lamp>> lamps;
    glm::vec3 camPos;
    glm::mat3 camMat;
    glm::vec3 worldHorizonColor;
    glm::vec3 worldAmbientColor;
    float worldAmbientFactor;
} Scene;


void loadScene(Scene &outScene, const std::string &pathToScene);

void renderScene(ImageBitmap &outImg, const Scene &scene);

#endif //RAY_TRACING_SCENE_H
