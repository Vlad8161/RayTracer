//
// Created by vlad on 4/2/17.
//

#ifndef RAY_TRACING_IMAGEBITMAP_H
#define RAY_TRACING_IMAGEBITMAP_H

#include <memory>
#include <cstring>

class image_bitmap {
private:
    float* mData;
    int mWidth;
    int mHeight;
    image_bitmap(const image_bitmap& imageBitmap) {}

public:
    image_bitmap(int width, int height) {
        this->mWidth = width;
        this->mHeight = height;
        this->mData = new float[mWidth * mHeight * 3];
        for (int i = 0; i < mWidth * mHeight * 3; i++) {
            this->mData[i] = 0.0f;
        }
    }

    ~image_bitmap() {
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

    inline float* getPixel(int x, int y) const {
        return mData + mWidth * y * 3 + x * 3;
    }

    inline int getHeight() const {
        return mHeight;
    }

    inline int getWidth() const {
        return mWidth;
    }

    inline void clear() {
        memset(mData, 0, (size_t) (mWidth * mHeight * 3 * sizeof(float)));
    }

    inline void copyImageTo(int x, int y, const image_bitmap& img) {
        for (int i = 0; i < img.getHeight() && i + y < mHeight; i++) {
            for (int j = 0; j < img.getWidth() && j + x < mWidth; j++) {
                float* inputPixel = img.getPixel(j, i);
                setPixel(x + j, y + i, inputPixel[0], inputPixel[1], inputPixel[2]);
            }
        }
    }
};

#endif //RAY_TRACING_IMAGEBITMAP_H
