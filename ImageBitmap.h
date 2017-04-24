//
// Created by vlad on 4/2/17.
//

#ifndef RAY_TRACING_IMAGEBITMAP_H
#define RAY_TRACING_IMAGEBITMAP_H

#include <memory>

class ImageBitmap {
private:
    float* mData;
    int mWidth;
    int mHeight;
    ImageBitmap(const ImageBitmap& imageBitmap) {}

public:
    ImageBitmap(int width, int height) {
        this->mWidth = width;
        this->mHeight = height;
        this->mData = new float[mWidth * mHeight * 3];
        for (int i = 0; i < mWidth * mHeight * 3; i++) {
            this->mData[i] = 0.0f;
        }
    }

    ~ImageBitmap() {
        delete[] mData;
    }

    inline void* getRawData() const {
        return mData;
    }

    inline void setPixel(int x, int y, float r, float g, float b) {
        if (x < 0 || x >= mWidth) {
            return;
        }

        if (y < 0 || y >= mHeight) {
            return;
        }

        float* needPixelPtr = mData + mWidth * y * 3 + x * 3;
        needPixelPtr[0] = r;
        needPixelPtr[1] = g;
        needPixelPtr[2] = b;
    }

    inline int getHeight() const {
        return mHeight;
    }

    inline int getWidth() const {
        return mWidth;
    }
};

#endif //RAY_TRACING_IMAGEBITMAP_H
