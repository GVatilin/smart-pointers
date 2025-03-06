#pragma once

#include <type_traits>
#include <utility>

template <typename T, bool Filled = !std::is_empty<T>::value || std::is_final<T>::value>
class First;

template <typename T>
class First<T, false> : public T {
public:
    template <typename S>
    First(S&& value) : T(std::forward<S>(value)) {
    }

    const T& Value() const {
        return *this;
    }

    T& Value() {
        return *this;
    }
};

template <typename T>
class First<T, true> {
public:
    First() {
        value_ = T();
    }

    T value_;

    template <typename S>
    First(S&& value) : value_(std::forward<S>(value)) {
    }

    const T& Value() const {
        return value_;
    }

    T& Value() {
        return value_;
    }
};

template <typename T, bool Filled = !std::is_empty<T>::value || std::is_final<T>::value>
class Second;

template <typename T>
class Second<T, false> : public T {
public:
    template <typename S>
    Second(S&& value) : T(std::forward<S>(value)) {
    }

    const T& Value() const {
        return *this;
    }

    T& Value() {
        return *this;
    }
};

template <typename T>
class Second<T, true> {
public:
    Second() {
        value_ = T();
    }

    T value_;

    template <typename S>
    Second(S&& value) : value_(std::forward<S>(value)) {
    }

    const T& Value() const {
        return value_;
    }

    T& Value() {
        return value_;
    }
};

template <typename F, typename S>
class CompressedPair : public First<F>, public Second<S> {
public:
    CompressedPair() = default;

    template <typename T1, typename T2>
    CompressedPair(T1&& first, T2&& second)
        : First<F>(std::forward<T1>(first)), Second<S>(std::forward<T2>(second)) {
    }

    const F& GetFirst() const {
        return First<F>::Value();
    }

    const S& GetSecond() const {
        return Second<S>::Value();
    }

    F& GetFirst() {
        return First<F>::Value();
    }

    S& GetSecond() {
        return Second<S>::Value();
    }
};