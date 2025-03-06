#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class BaseBlock {
public:
    void Inc() {
        count_++;
    }

    void Dec() {
        count_--;
        if (count_ == 0) {
            ZeroCount();
        }
    }

    int& GetCount() {
        return count_;
    }

    virtual void ZeroCount() = 0;
    virtual ~BaseBlock() = default;

private:
    int count_;
};

template <typename T>
class PtrBlock : public BaseBlock {
public:
    PtrBlock(T* ptr) : ptr_(ptr) {
        GetCount() = 0;
    }

    void ZeroCount() override {
        delete ptr_;
        ptr_ = nullptr;
    }

    T* Get() {
        return ptr_;
    }

    ~PtrBlock() override {
        ZeroCount();
    }

private:
    T* ptr_;
};

template <typename T>
class ValueBlock : public BaseBlock {
public:
    template <typename... Args>
    ValueBlock(Args&&... args) {
        new (&buffer_) T(std::forward<Args>(args)...);
        GetCount() = 0;
    }

    void ZeroCount() override {
        Get()->~T();
    }

    T* Get() {
        return reinterpret_cast<T*>(&buffer_);
    }

    ~ValueBlock() {
        ZeroCount();
    }

private:
    alignas(T) std::byte buffer_[sizeof(T)];
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        block_ = nullptr;
        ptr_ = nullptr;
    }

    SharedPtr(std::nullptr_t) {
        block_ = nullptr;
        ptr_ = nullptr;
    }

    explicit SharedPtr(T* ptr) {
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<T>(ptr);
            AddObj();
        }
        ptr_ = ptr;
    };

    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<Y>(ptr);
            AddObj();
        }
        ptr_ = ptr;
    }

    SharedPtr(const SharedPtr& other) : block_(other.block_), ptr_(other.ptr_) {
        AddObj();
    }

    SharedPtr(SharedPtr&& other) : block_(other.block_), ptr_(other.ptr_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        AddObj();
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        other.SetBlock(nullptr);
        other.SetPtr(nullptr);
    }

    SharedPtr(T* ptr, BaseBlock* block) : block_(block), ptr_(ptr) {
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.GetBlock();
        AddObj();
        ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        Reset();
        block_ = other.block_;
        AddObj();
        ptr_ = other.ptr_;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    template <typename Y>
    SharedPtr<T>& operator=(const SharedPtr<Y>& other) {
        Reset();
        block_ = other.GetBlock();
        AddObj();
        ptr_ = other.GetPtr();
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        Reset();
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        other.SetBlock(nullptr);
        other.SetPtr(nullptr);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor
    ~SharedPtr() {
        if (block_) {
            block_->Dec();
            if (block_->GetCount() == 0) {
                block_->ZeroCount();
                delete block_;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->Dec();
            if (block_->GetCount() == 0) {
                block_->ZeroCount();
                delete block_;
            }
            block_ = nullptr;
            ptr_ = nullptr;
        }
    }

    void Reset(T* ptr) {
        Reset();
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<T>(ptr);
            AddObj();
        }
        ptr_ = ptr;
    }

    template <typename Y>
    void Reset(Y* ptr) {
        Reset();
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<Y>(ptr);
            AddObj();
        }
        ptr_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(ptr_, other.ptr_);
    }

    BaseBlock* GetBlock() const {
        return block_;
    }

    T* GetPtr() const {
        return ptr_;
    }

    void SetBlock(BaseBlock* other) {
        block_ = other;
    }

    void SetPtr(T* ptr) {
        ptr_ = ptr;
    }

    void AddObj() {
        if (block_) {
            block_->Inc();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        if (ptr_) {
            return ptr_;
        }
        return nullptr;
    }

    T& operator*() const {
        return *Get();
    }

    T* operator->() const {
        return Get();
    }

    size_t UseCount() const {
        if (block_) {
            return static_cast<size_t>(block_->GetCount());
        }
        return 0;
    }

    explicit operator bool() const {
        if (block_) {
            return true;
        }
        return false;
    }

private:
    BaseBlock* block_;
    T* ptr_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ValueBlock<T>* tmp = new ValueBlock<T>(std::forward<Args>(args)...);
    tmp->Inc();
    SharedPtr<T> t(tmp->Get(), tmp);
    return t;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
