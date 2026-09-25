#pragma once

#include <cstddef>
#include <algorithm>

namespace engine {

template<typename T, size_t Capacity>
class FixedVector {
public:
    FixedVector() : size_(0) {}

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    constexpr size_t capacity() const { return Capacity; }

    void clear() { size_ = 0; }

    bool push_back(const T& value) {
        if (size_ >= Capacity) {
            return false;
        }
        data_[size_++] = value;
        return true;
    }

    bool pop_back() {
        if (size_ == 0) return false;
        --size_;
        return true;
    }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    T& back() { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }

    T* begin() { return data_; }
    const T* begin() const { return data_; }
    T* end() { return data_ + size_; }
    const T* end() const { return data_ + size_; }

    bool erase(size_t index) {
        if (index >= size_) return false;
        for (size_t i = index; i + 1 < size_; ++i) {
            data_[i] = data_[i + 1];
        }
        --size_;
        return true;
    }

private:
    T data_[Capacity];
    size_t size_;
};

} // namespace engine
