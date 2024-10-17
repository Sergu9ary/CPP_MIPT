#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 public:
  using value_type = T;
  using allocator_type = Allocator;

  Deque(const Allocator& alloc = Allocator()) : alloc_(alloc) {}

  Deque(const Deque& other)
      : alloc_(std::allocator_traits<Allocator>::
                   select_on_container_copy_construction(other.alloc_)) {
    try {
      for (size_t ind = 0; ind < other.size(); ++ind) {
        push_back(other[ind]);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  Deque(Deque&& other) noexcept
      : alloc_(std::move(other.alloc_)),
        array_(std::move(other.array_)),
        size_(other.size_),
        back_index_(other.back_index_),
        front_index_(other.front_index_) {
    other.size_ = 0;
    other.back_index_ = 0;
    other.front_index_ = 0;
  }

  Deque(size_t count, const Allocator& alloc = Allocator()) : alloc_(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back();
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  Deque(size_t count, const T& value, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    try {
      auto iter = init.begin();
      for (size_t i = 0; i < init.size(); ++i) {
        push_back(*iter);
        ++iter;
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  ~Deque() { clear(); }

  void swap(Deque& other) {
    std::swap(size_, other.size_);
    std::swap(alloc_, other.alloc_);
    std::swap(back_index_, other.back_index_);
    std::swap(front_index_, other.front_index_);
    std::swap(array_, other.array_);
  }

  Allocator get_allocator() const { return alloc_; }

  Deque& operator=(Deque&& other) noexcept {
    if (this != &other) {
      Deque copy = std::move(other);
      if (std::allocator_traits<
              Allocator>::propagate_on_container_copy_assignment::value) {
        alloc_ = std::move(other.alloc_);
      }
      *this = std::move(copy);
    }
    return *this;
  }

  Deque& operator=(const Deque& other) {
    if (this == &other) {
      return *this;
    }
    Deque copy = other;
    swap(copy);
    copy.alloc_ = alloc_;
    if (std::allocator_traits<
            Allocator>::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    return *this;
  }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  T& operator[](size_t index) {
    size_t block_index = (front_index_ + index) / kBlockSize;
    size_t offset = (front_index_ + index) % kBlockSize;
    return array_[block_index][offset];
  }

  const T& operator[](size_t index) const {
    size_t block_index = (front_index_ + index) / kBlockSize;
    size_t offset = (front_index_ + index) % kBlockSize;
    return array_[block_index][offset];
  }

  const T& at(size_t index) const {
    if (index >= size()) {
      throw std::out_of_range("out of range");
    }
    return Deque<T>::operator[](index);
  }

  T& at(size_t index) {
    if (index >= size()) {
      throw std::out_of_range("out of range");
    }
    return Deque<T>::operator[](index);
  }

  void push_back(const T& value) { emplace_back(value); }

  void push_back(T&& value) { emplace_back(std::move(value)); }

  void push_back() { emplace_back(); }

  template <class... Args>
  void emplace_back(Args&&... args) {
    ensure_back_capacity();
    size_t x_cor = back_index_ / kBlockSize;
    size_t y_cor = back_index_ % kBlockSize;

    try {
      std::allocator_traits<Allocator>::construct(alloc_, array_[x_cor] + y_cor,
                                                  std::forward<Args>(args)...);
    } catch (...) {
      handle_failed_construction();
      throw;
    }

    ++back_index_;
    ++size_;
  }

  void push_front(const T& value) { emplace_front(value); }

  void push_front(T&& value) { emplace_front(std::move(value)); }

  template <class... Args>
  void emplace_front(Args&&... args) {
    ensure_front_capacity();
    size_t x_cor = (front_index_ - 1) / kBlockSize;
    size_t y_cor = (front_index_ - 1) % kBlockSize;

    std::allocator_traits<Allocator>::construct(alloc_, array_[x_cor] + y_cor,
                                                std::forward<Args>(args)...);

    --front_index_;
    ++size_;
  }

  void pop_back() {
    size_t x_cor = (back_index_ - 1) / kBlockSize;
    size_t y_cor = (back_index_ - 1) % kBlockSize;

    std::allocator_traits<Allocator>::destroy(alloc_, array_[x_cor] + y_cor);

    --back_index_;
    --size_;
  }

  void pop_front() {
    size_t x_cor = front_index_ / kBlockSize;
    size_t y_cor = front_index_ % kBlockSize;

    std::allocator_traits<Allocator>::destroy(alloc_, array_[x_cor] + y_cor);

    ++front_index_;
    --size_;
  }

  template <bool IsConst>
  class DequeIterator {
   private:
    using deque_type = std::conditional_t<IsConst, const Deque<T, Allocator>*,
                                          Deque<T, Allocator>*>;
    deque_type deque_ptr_;
    size_t index_ = 0;

   public:
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;

    DequeIterator() : deque_ptr_(nullptr) {}

    DequeIterator(deque_type deque, size_t index)
        : deque_ptr_(deque), index_(index) {}

    reference operator*() const { return (*deque_ptr_)[index_]; }

    pointer operator->() const { return &((*deque_ptr_)[index_]); }

    DequeIterator(const DequeIterator& other)
        : deque_ptr_(other.deque_ptr_), index_(other.index_) {}

    const DequeIterator<IsConst>& operator=(
        const DequeIterator<IsConst>& other) {
      index_ = other.index_;
      return *this;
    }

    DequeIterator<IsConst>& operator++() {
      ++index_;
      return *this;
    }

    DequeIterator<IsConst> operator++(int) {
      DequeIterator temp = *this;
      ++index_;
      return temp;
    }

    DequeIterator<IsConst>& operator--() {
      --index_;
      return *this;
    }

    DequeIterator<IsConst> operator--(int) {
      DequeIterator temp = *this;
      --index_;
      return temp;
    }

    DequeIterator<IsConst>& operator+=(difference_type n) {
      index_ += n;
      return *this;
    }

    DequeIterator<IsConst>& operator-=(difference_type n) {
      index_ -= n;
      return *this;
    }

    DequeIterator<IsConst> operator+(difference_type n) const {
      return DequeIterator<IsConst>(deque_ptr_, index_ + n);
    }

    DequeIterator<IsConst> operator-(difference_type n) const {
      return DequeIterator(deque_ptr_, index_ - n);
    }

    difference_type operator-(const DequeIterator& other) const {
      return index_ - other.index_;
    }

    bool operator==(const DequeIterator& other) const {
      return index_ == other.index_;
    }

    bool operator!=(const DequeIterator& other) const {
      return index_ != other.index_;
    }

    bool operator<(const DequeIterator& other) const {
      return index_ < other.index_;
    }

    bool operator>(const DequeIterator& other) const {
      return index_ > other.index_;
    }

    bool operator<=(const DequeIterator& other) const {
      return index_ <= other.index_;
    }

    bool operator>=(const DequeIterator& other) const {
      return index_ >= other.index_;
    }
  };

  using iterator = DequeIterator<false>;
  using const_iterator = DequeIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(this, 0); }

  iterator end() { return iterator(this, size_); }

  const_iterator cbegin() const { return const_iterator(this, 0); }

  const_iterator cend() const { return const_iterator(this, size_); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  void insert(iterator iter, const T& value) {
    push_back(value);
    size_t pos = iter - begin();
    for (size_t i = size_ - 1; i > pos; --i) {
      std::swap((*this)[i], (*this)[i - 1]);
    }
  }

  void emplace(iterator iter, T&& value) {
    push_back(std::move(value));
    size_t pos = iter - begin();
    for (size_t i = size_ - 1; i > pos; --i) {
      std::swap((*this)[i], (*this)[i - 1]);
    }
  }

  void erase(iterator iter) {
    size_t pos = iter - begin();
    for (size_t i = pos; i < size_ - 1; ++i) {
      std::swap((*this)[i], (*this)[i + 1]);
    }
    pop_back();
  }

 private:
  Allocator alloc_;
  size_t size_ = 0;
  const size_t kBlockSize = 1000;
  size_t front_index_ = 0;
  size_t back_index_ = 0;
  std::vector<T*> array_;

  void ensure_back_capacity() {
    if (back_index_ % kBlockSize == 0) {
      array_.push_back(allocate_block());
    }
  }

  void ensure_front_capacity() {
    if (front_index_ == 0) {
      array_.insert(array_.begin(), allocate_block());
      front_index_ = kBlockSize;
      back_index_ += kBlockSize;
    }
  }

  T* allocate_block() {
    return std::allocator_traits<Allocator>::allocate(alloc_, kBlockSize);
  }

  void handle_failed_construction() {
    if (back_index_ % kBlockSize == 0 && !array_.empty()) {
      std::allocator_traits<Allocator>::deallocate(alloc_, array_.back(),
                                                   kBlockSize);
      array_.pop_back();
    }
  }

  void clear() {
    for (size_t i = 0; i < size_; ++i) {
      size_t block = (front_index_ + i) / kBlockSize;
      size_t offset = (front_index_ + i) % kBlockSize;
      std::allocator_traits<Allocator>::destroy(alloc_, array_[block] + offset);
    }
    size_ = 0;

    for (auto& block : array_) {
      std::allocator_traits<Allocator>::deallocate(alloc_, block, kBlockSize);
    }
    array_.clear();
    front_index_ = 0;
    back_index_ = 0;
  }
};