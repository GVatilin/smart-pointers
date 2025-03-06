#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : block_(nullptr), ptr_(nullptr) {
    }

    WeakPtr(const WeakPtr& other) : block_(other.block_), ptr_(other.ptr_) {
        if (block_) {
            block_->WeakInc();
        }
    }

    WeakPtr(WeakPtr&& other) : block_(other.block_), ptr_(other.ptr_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : block_(other.GetBlock()), ptr_(other.GetPtr()) {
        if (block_) {
            block_->WeakInc();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            Reset();
            block_ = other.block_;
            if (block_) {
                block_->WeakInc();
            }
            ptr_ = other.ptr_;
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        Reset();
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (block_) {
            block_->WeakDec();
            if (block_->GetStrongCount() == 0 && block_->GetWeakCount() == 0) {
                delete block_;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->WeakDec();
            if (block_->GetStrongCount() == 0 && block_->GetWeakCount() == 0) {
                delete block_;
            }
        }
        MakeNull();
    }

    void MakeNull() {
        block_ = nullptr;
        ptr_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(ptr_, other.ptr_);
    }

    BaseBlock* GetBlock() const {
        return block_;
    }

    T* GetPtr() const {
        return ptr_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->GetStrongCount();
        }
        return 0;
    }

    bool Expired() const {
        if (!block_ || block_->GetStrongCount() == 0) {
            return true;
        }
        return false;
    }

    SharedPtr<T> Lock() const {
        if (block_ && block_->GetStrongCount() != 0) {
            return SharedPtr<T>(*this);
        } else {
            return SharedPtr<T>();
        }
    }

private:
    BaseBlock* block_;
    T* ptr_;
};
