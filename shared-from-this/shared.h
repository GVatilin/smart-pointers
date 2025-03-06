#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class BaseBlock {
public:
    void WeakInc() {
        weak_count_++;
    }

    void WeakDec() {
        weak_count_--;
        /*if (weak_count_ == 0) {
            ZeroWeakCount();
        }*/
    }

    int& GetWeakCount() {
        return weak_count_;
    }

    void StrongInc() {
        strong_count_++;
    }

    void StrongDec() {
        strong_count_--;
    }

    int& GetStrongCount() {
        return strong_count_;
    }

    // virtual void ZeroWeakCount() = 0;
    virtual void ZeroStrongCount() = 0;
    virtual ~BaseBlock() = default;

private:
    int weak_count_;
    int strong_count_;
};

template <typename T>
class PtrBlock : public BaseBlock {
public:
    PtrBlock(T* ptr) : ptr_(ptr) {
        GetWeakCount() = 0;
        GetStrongCount() = 0;
    }

    /*void ZeroWeakCount() override {
        if (GetStrongCount() == 0) {
            delete this;
        }
    }*/

    void ZeroStrongCount() override {
        delete ptr_;
        ptr_ = nullptr;
    }

    T* Get() {
        return ptr_;
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
        GetWeakCount() = 0;
        GetStrongCount() = 0;
    }

    void ZeroStrongCount() override {
        Get()->~T();
    }

    /*void ZeroWeakCount() override {
        if (GetStrongCount() == 0) {
            delete this;
        }
    }*/

    T* Get() {
        return reinterpret_cast<T*>(&buffer_);
    }

private:
    alignas(T) std::byte buffer_[sizeof(T)];
};
class EnableSharedFromThisBase;
// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        block_ = nullptr;
        ptr_ = nullptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(nullptr);
        }
    }

    SharedPtr(std::nullptr_t) {
        block_ = nullptr;
        ptr_ = nullptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(nullptr);
        }
    }

    explicit SharedPtr(T* ptr) {
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<T>(ptr);
            AddObj();
        }
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    };

    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        block_ = nullptr;
        if (ptr) {
            block_ = new PtrBlock<Y>(ptr);
            AddObj();
        }
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    SharedPtr(const SharedPtr& other) : block_(other.block_), ptr_(other.ptr_) {
        AddObj();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    SharedPtr(SharedPtr&& other) : block_(other.block_), ptr_(other.ptr_) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        AddObj();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        other.SetBlock(nullptr);
        other.SetPtr(nullptr);
    }

    SharedPtr(T* ptr, BaseBlock* block) : block_(block), ptr_(ptr) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.GetBlock();
        AddObj();
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        if (block_ && block_->GetStrongCount() == 0) {
            throw BadWeakPtr();
        }
        if (block_) {
            block_->StrongInc();
        }
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        Reset();
        block_ = other.block_;
        AddObj();
        ptr_ = other.ptr_;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        return *this;
    }

    template <typename Y>
    SharedPtr<T>& operator=(const SharedPtr<Y>& other) {
        Reset();
        block_ = other.GetBlock();
        AddObj();
        ptr_ = other.GetPtr();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        Reset();
        block_ = other.GetBlock();
        ptr_ = other.GetPtr();
        other.SetBlock(nullptr);
        other.SetPtr(nullptr);
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            ptr_->SetWeak(*this);
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor
    ~SharedPtr() {
        if (block_) {
            block_->StrongDec();
            if (block_->GetStrongCount() == 0) {
                if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
                    block_->WeakInc();
                }
                block_->ZeroStrongCount();
                if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
                    block_->WeakDec();
                }
                if (block_->GetWeakCount() == 0) {
                    delete block_;
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->StrongDec();
            if (block_->GetStrongCount() == 0) {
                if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
                    block_->WeakInc();
                }
                block_->ZeroStrongCount();
                if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
                    block_->WeakDec();
                }
                if (block_->GetWeakCount() == 0) {
                    delete block_;
                }
            }
        }
        block_ = nullptr;
        ptr_ = nullptr;
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
            block_->StrongInc();
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
            return static_cast<size_t>(block_->GetStrongCount());
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
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    if (left.GetBlock() == right.GetBlock() && left.GetPtr() == right.GetPtr()) {
        return true;
    }
    return false;
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ValueBlock<T>* tmp = new ValueBlock<T>(std::forward<Args>(args)...);
    tmp->StrongInc();
    SharedPtr<T> t(tmp->Get(), tmp);
    return t;
}

class EnableSharedFromThisBase {};

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_);
    }

    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(weak_);
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_);
    }

    void SetWeak(const WeakPtr<T>& weak) {
        weak_ = weak;
        if (!flag_ && weak_.GetBlock()) {
            weak_.GetBlock()->WeakInc();
            flag_ = true;
        }
    }

    void SetWeak(const SharedPtr<T>& weak) {
        weak_.SetBlock(weak.GetBlock());
        weak_.SetPtr(weak.GetPtr());
        if (!flag_ && weak_.GetBlock()) {
            weak_.GetBlock()->WeakInc();
            flag_ = true;
        }
    }

private:
    WeakPtr<T> weak_;
    bool flag_ = false;
};
