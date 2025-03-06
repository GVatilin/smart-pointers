#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        count_++;
        return count_;
    }

    size_t DecRef() {
        count_--;
        return count_;
    }

    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (counter_.DecRef() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

    RefCounted& operator=(const RefCounted&) {
        return *this;
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() {
        ptr_ = nullptr;
    }

    IntrusivePtr(std::nullptr_t) {
        ptr_ = nullptr;
    }

    IntrusivePtr(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            ptr_->IncRef();
        }
    }

    template <typename U>
    IntrusivePtr(U* ptr) : ptr_(ptr) {
        if (ptr) {
            ptr_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->IncRef();
        }
    }

    IntrusivePtr(IntrusivePtr&& other) : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (ptr_ != other.ptr_) {
            Reset();
            if (other.ptr_) {
                ptr_ = other.ptr_;
                ptr_->IncRef();
            }
        }
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }

        return *this;
    }

    template <typename U>
    IntrusivePtr& operator=(const IntrusivePtr<U>& other) {
        if (this != &other) {
            Reset();
            ptr_ = other.ptr_;
            if (ptr_) {
                ptr_->IncRef();
            }
            return *this;
        }
    }

    template <typename U>
    IntrusivePtr& operator=(IntrusivePtr<U>&& other) {
        if (ptr_ != other.ptr_) {
            Reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        Reset();
    }

    // Modifiers
    void Reset() {
        if (ptr_) {
            ptr_->DecRef();
            ptr_ = nullptr;
        }
    }

    void Reset(T* ptr) {
        if (ptr_) {
            ptr_->DecRef();
        }
        ptr_ = ptr;
        if (ptr) {
            ptr_->IncRef();
        }
    }

    void Swap(IntrusivePtr& other) {
        std::swap(ptr_, other.ptr_);
    }

    // Observers
    T* Get() const {
        if (ptr_) {
            return ptr_;
        }
        return nullptr;
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    size_t UseCount() const {
        if (ptr_) {
            return ptr_->RefCount();
        }
        return 0;
    }
    explicit operator bool() const {
        if (ptr_) {
            return true;
        }
        return false;
    }

private:
    T* ptr_;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    T* a = new T(std::forward<Args>(args)...);
    IntrusivePtr<T> t(a);
    return t;
}
