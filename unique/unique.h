#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t

struct Slug {};

template <typename T>
class MyDeleter {
public:
    MyDeleter() {
    }

    MyDeleter(T value) {
    }

    template <typename U>
    MyDeleter(U&& other) {
    }

    void operator()(T* ptr) {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void_v<T>);
        delete ptr;
    }
};

template <typename T>
class MyDeleter<T[]> {
public:
    void operator()(T* ptr) {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void_v<T>);
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = MyDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)) {
    }

    UniquePtr(UniquePtr&& other) noexcept
        : data_(other.data_.GetFirst(), std::move(other.GetDeleter())) {
        other.data_.GetFirst() = nullptr;
    }

    template <typename U, typename UDeleter>
    UniquePtr(UniquePtr<U, UDeleter>&& other)
        : data_(other.Release(), std::move(other.GetDeleter())) {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // operator=-s

    UniquePtr& operator=(const UniquePtr& other) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            data_.GetSecond()(data_.GetFirst());
            data_.GetFirst() = other.data_.GetFirst();
            data_.GetSecond() = std::move(other.GetDeleter());
            other.data_.GetFirst() = nullptr;
        }
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* tmp = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return tmp;
    }

    void Reset(T* ptr = nullptr) {
        auto tmp = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(tmp);
    }

    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        if (data_.GetFirst() != nullptr) {
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *data_.GetFirst();
    }

    T* operator->() const {
        return data_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> data_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)) {
    }

    UniquePtr(UniquePtr&& other) noexcept
        : data_(other.data_.GetFirst(), std::move(other.GetDeleter())) {
        other.data_.GetFirst() = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // operator=-s

    UniquePtr& operator=(const UniquePtr& other) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            data_.GetSecond()(data_.GetFirst());
            data_.GetFirst() = other.data_.GetFirst();
            data_.GetSecond() = std::move(other.GetDeleter());
            other.data_.GetFirst() = nullptr;
        }
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* tmp = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return tmp;
    }

    void Reset(T* ptr = nullptr) {
        auto tmp = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(tmp);
    }

    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        if (data_.GetFirst() != nullptr) {
            return true;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Array subscript operators

    T& operator[](size_t index) {
        return data_.GetFirst()[index];
    }

    const T& operator[](size_t index) const {
        return data_.GetFirst()[index];
    }

private:
    CompressedPair<T*, Deleter> data_;
};
