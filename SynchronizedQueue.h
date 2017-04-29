//
// Created by vlad on 4/29/17.
//

#ifndef RAY_TRACING_SYNCHRONIZEDQUEUE_H
#define RAY_TRACING_SYNCHRONIZEDQUEUE_H

#include <vector>
#include <mutex>

template<typename T>
class SynchronizedQueue {
private:
    std::vector<T> mData;
    std::mutex mMutex;

public:
    void push_back(const T &item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mData.push_back(item);
    }

    T pop_back() {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mData.size() > 0) {
            T retVal = mData.back();
            mData.pop_back();
            return retVal;
        } else {
            throw std::out_of_range("Empty container");
        }
    }
};


#endif //RAY_TRACING_SYNCHRONIZEDQUEUE_H
