//
// Created by vlad on 4/2/17.
//

#ifndef RAY_TRACING_IMAGEBITMAP_H
#define RAY_TRACING_IMAGEBITMAP_H

#include <memory>

class ImageBitmap {
private:
    std::unique_ptr<uint8_t> mData;
    int mWidth;
    int mHeight;

public:
    ImageBitmap(int width, int height) {
        this->mWidth = width;
        this->mHeight = height;
        this->mData.reset(new uint8_t[mWidth * mHeight * 3]);
        for (int i = 0; i < mWidth * mHeight * 3; i++) {
            this->mData.get()[i] = 0;
        }
    }

    inline void* getRawData() const {
        return mData.get();
    }

    inline uint32_t getPixel(int x, int y) const {
         if (x < 0 || x >= mWidth) {
            return 0xFFFFFFFF;
        }

        if (y < 0 || y >= mHeight) {
            return 0xFFFFFFFF;
        }

        uint8_t* data = mData.get();
        uint8_t* needPixelPtr = data + mWidth * y + x;
        uint8_t r = needPixelPtr[0];
        uint8_t g = needPixelPtr[1];
        uint8_t b = needPixelPtr[2];
        return
                ((uint32_t) r) |
                ((uint32_t) g << 8) |
                ((uint32_t) b << 16);
    }

    inline void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= mWidth) {
            return;
        }

        if (y < 0 || y >= mHeight) {
            return;
        }

        uint8_t* data = mData.get();
        uint8_t* needPixelPtr = data + mWidth * y + x;
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
