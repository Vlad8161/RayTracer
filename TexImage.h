//
// Created by vlad on 4/23/17.
//

#ifndef RAY_TRACING_TEXIMAGE_H
#define RAY_TRACING_TEXIMAGE_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "lodepng.h"

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

#endif //RAY_TRACING_TEXIMAGE_H
