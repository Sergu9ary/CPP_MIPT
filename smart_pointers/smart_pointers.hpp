#pragma once
#include <iostream>
#include <memory>

namespace control_block {
struct Counter {
 public:
  size_t count_shared = 0;
  size_t count_weak = 0;
  void* object_ptr;
  virtual void deallocate_block() {}
  virtual void delete_ptr() {}
  virtual ~Counter() = default;
};

template <typename T, typename Deleter = std::default_delete<T>,
          typename Alloc = std::allocator<T>>
struct Base : public Counter {
  Deleter deleter;
  Alloc alloc;

  using alloc_traits = std::allocator_traits<Alloc>;
  using block_alloc = typename alloc_traits::template rebind_alloc<
      control_block::Base<T, Deleter, Alloc>>;
  using block_alloc_traits = typename alloc_traits::template rebind_traits<
      control_block::Base<T, Deleter, Alloc>>;

  template <typename Y>
  Base(Y* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc())
      : deleter(deleter), alloc(alloc) {
    Counter::object_ptr = ptr;
  }

  Base(std::nullptr_t, Deleter deleter = Deleter(), Alloc alloc = Alloc())
      : deleter(deleter), alloc(alloc) {
    Counter::object_ptr = nullptr;
  }

  void deallocate_block() {
    block_alloc block_alloc = alloc;
    block_alloc_traits::deallocate(block_alloc, this, 1);
  }

  void delete_ptr() { deleter(static_cast<T*>(Counter::object_ptr)); }
  ~Base() = default;
};
template <typename T, typename Alloc = std::allocator<T>>
struct Make : Counter {
  T object;
  Alloc alloc;
  using alloc_traits = std::allocator_traits<Alloc>;
  using block_alloc = typename alloc_traits::template rebind_alloc<
      control_block::Make<T, Alloc>>;
  using block_alloc_traits = typename alloc_traits::template rebind_traits<
      control_block::Make<T, Alloc>>;

  template <typename... Args>
  Make(Args&&... args) : object(std::forward<Args>(args)...) {
    alloc = Alloc();
    Counter::object_ptr = &object;
    ++Counter::count_shared;
  }

  void deallocate_block() {
    block_alloc block_al = alloc;
    block_alloc_traits::deallocate(block_al, this, 1);
  }

  void delete_ptr() {
    alloc_traits::destroy(alloc, static_cast<T*>(Counter::object_ptr));
  }
};
};  // namespace control_block

template <typename T>
class SharedPtr {
 private:
  T* ptr_;
  control_block::Counter* control_block_ = nullptr;

  template <typename U, typename... Args>
  friend SharedPtr<U> MakeShared(Args&&... args);

  template <typename U, typename Alloc>
  SharedPtr(control_block::Make<U, Alloc>* control_block) {
    control_block_ = control_block;
    ptr_ = static_cast<T*>(control_block_->object_ptr);
  }

  template <typename U>
  friend class WeakPtr;

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> AllocateShared(const Alloc& alloc, Args&&... args);

  template <typename Y>
  friend class SharedPtr;

 public:
  SharedPtr() : ptr_(nullptr) {}
  SharedPtr(std::nullptr_t) : ptr_(nullptr) {}

  template <typename Y>
  SharedPtr(Y* ptr) {
    using alloc_traits = std::allocator_traits<std::allocator<T>>;
    using block_alloc = typename alloc_traits::template rebind_alloc<
        control_block::Base<T, std::default_delete<T>, std::allocator<T>>>;
    using block_alloc_traits = typename alloc_traits::template rebind_traits<
        control_block::Base<T, std::default_delete<T>, std::allocator<T>>>;

    block_alloc alloc;
    control_block_ = block_alloc_traits::allocate(alloc, 1);
    block_alloc_traits::construct(
        alloc, reinterpret_cast<control_block::Base<T>*>(control_block_), ptr);
    ptr_ = static_cast<T*>(control_block_->object_ptr);
    ++control_block_->count_shared;
  }

  ~SharedPtr() {
    if (!ptr_) {
      return;
    }
    if (control_block_->count_shared != 0) {
      --control_block_->count_shared;
    }
    if (control_block_->count_shared == 0) {
      control_block_->delete_ptr();
      if (control_block_->count_weak == 0) {
        control_block_->deallocate_block();
      }
    }
  }

  template <typename Y>
  SharedPtr(const SharedPtr<Y>& other)
      : control_block_(other.control_block_), ptr_(other.ptr_) {
    ++other.control_block_->count_shared;
  }

  SharedPtr(const SharedPtr<T>& other)
      : control_block_(other.control_block_), ptr_(other.ptr_) {
    ++control_block_->count_shared;
  }

  SharedPtr(SharedPtr<T>&& other)
      : control_block_(std::move(other.control_block_)),
        ptr_(std::move(other.ptr_)) {
    other.control_block_ = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename Y>
  SharedPtr<T>& operator=(const SharedPtr<Y>& other) {
    std::cout << "operator=\n";
    SharedPtr<T> dop(other);
    std::swap(control_block_, dop.control_block_);
    std::swap(ptr_, dop.ptr_);
    return *this;
  }

  SharedPtr<T>& operator=(const SharedPtr<T>& other) {
    SharedPtr<T> dop(other);
    std::swap(control_block_, dop.control_block_);
    std::swap(ptr_, dop.ptr_);
    return *this;
  }

  SharedPtr<T>& operator=(SharedPtr<T>&& other) {
    SharedPtr<T> dop(std::move(other));
    std::swap(control_block_, dop.control_block_);
    std::swap(ptr_, dop.ptr_);
    return *this;
  }

  template <typename Y, typename Deleter>
  SharedPtr(Y* ptr, const Deleter& deleter) {
    using alloc_traits = std::allocator_traits<std::allocator<T>>;
    using block_alloc = typename alloc_traits::template rebind_alloc<
        control_block::Base<T, Deleter, std::allocator<T>>>;
    using block_alloc_traits = typename alloc_traits::template rebind_traits<
        control_block::Base<T, Deleter, std::allocator<T>>>;

    block_alloc alloc;
    control_block_ = block_alloc_traits::allocate(alloc, 1);
    block_alloc_traits::construct(
        alloc,
        reinterpret_cast<control_block::Base<T, Deleter>*>(control_block_), ptr,
        deleter);
    ptr_ = static_cast<T*>(control_block_->object_ptr);
    ++control_block_->count_shared;
  }

  template <typename Y, typename Deleter, typename Alloc>
  SharedPtr(Y* ptr, const Deleter& deleter, const Alloc& alloc) {
    using alloc_traits = std::allocator_traits<Alloc>;
    using block_alloc = typename alloc_traits::template rebind_alloc<
        control_block::Base<T, Deleter, Alloc>>;
    using block_alloc_traits = typename alloc_traits::template rebind_traits<
        control_block::Base<T, Deleter, Alloc>>;

    block_alloc alloc_block = alloc;
    control_block_ = block_alloc_traits::allocate(alloc_block, 1);
    block_alloc_traits::construct(
        alloc_block,
        reinterpret_cast<control_block::Base<T, Deleter, Alloc>*>(
            control_block_),
        ptr, deleter, alloc_block);
    ptr_ = static_cast<T*>(control_block_->object_ptr);
    ++control_block_->count_shared;
  }

  size_t use_count() const {
    if (!ptr_) {
      return 0;
    }
    return control_block_->count_shared;
  }

  T* get() const { return ptr_; }

  T& operator*() const { return *ptr_; }

  T* operator->() const { return ptr_; }

  void reset() {
    if (!ptr_) {
      return;
    }
    if (control_block_->count_shared != 0) {
      --control_block_->count_shared;
    }
    if (control_block_->count_shared == 0) {
      control_block_->delete_ptr();
      if (control_block_->count_weak == 0) {
        control_block_->deallocate_block();
      }
    }
    ptr_ = nullptr;
  }
};

template <typename T>
class WeakPtr {
 private:
  T* ptr_;
  control_block::Counter* control_block_;

 public:
  WeakPtr(const SharedPtr<T>& ptr)
      : control_block_(ptr.control_block_), ptr_(ptr.ptr_) {
    ++control_block_->count_weak;
  }

  ~WeakPtr() {
    --control_block_->count_weak;
    if ((control_block_->count_weak == 0) &&
        (control_block_->count_shared == 0)) {
      control_block_->deallocate_block();
    }
  }

  bool expired() const { return control_block_->count_shared == 0; }

  SharedPtr<T> lock() const { return SharedPtr<T>(ptr_); }
};

template <typename U, typename... Args>
SharedPtr<U> MakeShared(Args&&... args) {
  auto control_block = new control_block::Make<U>(std::forward<Args>(args)...);
  return SharedPtr<U>(control_block);
}

template <typename U, typename Alloc, typename... Args>
SharedPtr<U> AllocateShared(const Alloc& alloc, Args&&... args) {
  using alloc_traits = std::allocator_traits<Alloc>;
  using block_alloc = typename alloc_traits::template rebind_alloc<
      control_block::Make<U, Alloc>>;
  using block_alloc_traits = typename alloc_traits::template rebind_traits<
      control_block::Make<U, Alloc>>;

  block_alloc alloc_new = alloc;
  auto control_block = block_alloc_traits::allocate(alloc_new, 1);
  block_alloc_traits::construct(alloc_new, control_block,
                                std::forward<Args>(args)...);
  return SharedPtr<U>(control_block);
}