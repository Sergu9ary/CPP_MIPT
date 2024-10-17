#include <algorithm>
#include <iostream>
#include <memory>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  template <bool IsConst>
  class BaseIterator;
  template <bool IsConst>
  friend class BaseIterator;

 public:
  using value_type = T;
  using allocator_type = Allocator;

  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = value_type*;
  using const_pointer = value_type const*;
  using iterator = BaseIterator<false>;
  using const_iterator = BaseIterator<true>;
  using difference_type = std::ptrdiff_t;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  size_t size_ = 0;
  struct BaseNode {
    BaseNode* next = nullptr;
    BaseNode* prev = nullptr;
  };
  struct Node : public BaseNode {
    T value;
    Node() : value(T()) {}
    Node(const T& val) : value(val) {}
  };
  Node* head_ = nullptr;
  Node* tail_ = nullptr;
  BaseNode fake_node_;
  BaseNode* fake_ = nullptr;
  using alloc_traits = std::allocator_traits<allocator_type>;
  using node_alloc = typename alloc_traits::template rebind_alloc<Node>;
  using node_alloc_traits = typename alloc_traits::template rebind_traits<Node>;
  node_alloc alloc_;

  template <bool IsConst = false>
  class BaseIterator {
   public:
    using value_type = typename std::conditional<IsConst, const T, T>::type;
    using reference = typename std::conditional<IsConst, const T&, T&>::type;
    using pointer = typename std::conditional<IsConst, const T*, T*>::type;
    using iterator_category = typename std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using fake_node_pointer =
        typename std::conditional<IsConst, const BaseNode*, BaseNode*>::type;
    using node_pointer =
        typename std::conditional<IsConst, const Node*, Node*>::type;
    BaseIterator(node_pointer node, node_pointer head, node_pointer tail,
                 fake_node_pointer fake)
        : current_(node), head_(head), tail_(tail), fake_(fake) {}
    BaseIterator(const BaseIterator& src)
        : current_(src.current_),
          tail_(src.tail_),
          head_(src.head_),
          fake_(src.fake_) {}

    typename std::conditional<IsConst, const T&, T&>::type operator*() const {
      return current_->value;
    }
    typename std::conditional<IsConst, const T*, T*>::type operator->() const {
      return &(current_->value);
    }
    BaseIterator& operator=(const BaseIterator& src) {
      BaseIterator copy = src;
      std::swap(current_, copy.current_);
      std::swap(head_, copy.head_);
      std::swap(tail_, copy.tail_);
      std::swap(fake_, copy.fake_);
      return *this;
    }
    BaseIterator operator++(int) {
      BaseIterator prev = *this;
      ++(*this);
      return prev;
    }
    BaseIterator& operator--() {
      current_ = static_cast<node_pointer>(current_->prev);
      return *this;
    }
    BaseIterator& operator++() {
      current_ = static_cast<node_pointer>(current_->next);
      return *this;
    }
    BaseIterator operator--(int) {
      BaseIterator prev = *this;
      --(*this);
      return prev;
    }
    bool operator==(const BaseIterator& other) const {
      return (current_ == other.current_);
    }
    bool operator!=(const BaseIterator& other) const {
      return !(*this == other);
    }

   private:
    node_pointer tail_;
    node_pointer head_;
    node_pointer current_;
    fake_node_pointer fake_;
  };

 public:
  List() {
    size_ = 0;
    head_ = nullptr;
    tail_ = nullptr;
    fake_ = &fake_node_;
  }
  explicit List(size_t count, const Allocator& alloc = allocator_type())
      : alloc_(alloc) {
    size_ = count;
    Node* current = nullptr;
    for (size_t i = 0; i < count; ++i) {
      Node* temp;
      temp = node_alloc_traits::allocate(alloc_, 1);
      try {
        node_alloc_traits::construct(alloc_, temp);
      } catch (...) {
        node_alloc_traits::deallocate(alloc_, temp, 1);
        if (head_ != nullptr) {
          Node* ptr = head_;
          while (ptr != nullptr) {
            head_ = static_cast<Node*>(head_->next);
            node_alloc_traits::destroy(alloc_, ptr);
            node_alloc_traits::deallocate(alloc_, ptr, 1);
            ptr = head_;
          }
        }
        throw;
      }
      temp->prev = current;
      current = temp;
      if (temp->prev != nullptr) {
        temp->prev->next = current;
      }
      if (i == 0) {
        head_ = temp;
      } else if (i == size_ - 1) {
        tail_ = temp;
      }
    }
    fake_ = &fake_node_;
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  List(size_t count, const T& value,
       const allocator_type& alloc = allocator_type())
      : alloc_(alloc) {
    size_ = count;
    Node* current = nullptr;
    for (size_t i = 0; i < count; ++i) {
      Node* temp;
      try {
        temp = node_alloc_traits::allocate(alloc_, 1);
      } catch (const std::bad_alloc&) {
        throw;
      }
      try {
        node_alloc_traits::construct(alloc_, temp, value);
      } catch (...) {
        node_alloc_traits::deallocate(alloc_, temp, 1);
        throw;
      }
      temp->prev = current;
      // temp->value = value;
      current = temp;
      if (temp->prev != nullptr) {
        temp->prev->next = current;
      }
      if (i == 0) {
        head_ = temp;
      } else if (i == count - 1) {
        tail_ = temp;
      }
    }
    fake_ = &fake_node_;
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  List(const List& other)
      : alloc_(node_alloc_traits::select_on_container_copy_construction(
            other.alloc_)) {
    size_ = other.size_;
    Node* current = nullptr;
    Node* other_current = other.head_;
    for (size_t i = 0; i < size_; ++i) {
      Node* temp = nullptr;
      try {
        temp = node_alloc_traits::allocate(alloc_, 1);
      } catch (const std::bad_alloc&) {
        throw;
      }
      try {
        node_alloc_traits::construct(alloc_, temp, other_current->value);
      } catch (...) {
        node_alloc_traits::deallocate(alloc_, temp, 1);
        Node* ptr = head_;
        while (ptr != nullptr) {
          head_ = static_cast<Node*>(head_->next);
          node_alloc_traits::destroy(alloc_, ptr);
          node_alloc_traits::deallocate(alloc_, ptr, 1);
          ptr = head_;
        }
        throw;
      }
      temp->prev = current;
      current = temp;
      if (temp->prev != nullptr) {
        temp->prev->next = current;
      }
      if (i == 0) {
        head_ = temp;
      } else if (i == size_ - 1) {
        tail_ = temp;
      }
      other_current = static_cast<Node*>(other_current->next);
    }
    fake_ = &fake_node_;
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  List(std::initializer_list<T> init,
       const allocator_type& alloc = allocator_type())
      : alloc_(alloc) {
    size_ = init.size();
    Node* current = nullptr;
    for (auto it = init.begin(); it != init.end(); ++it) {
      Node* temp;
      try {
        temp = node_alloc_traits::allocate(alloc_, 1);
      } catch (const std::bad_alloc&) {
        throw;
      }
      try {
        node_alloc_traits::construct(alloc_, temp, *it);
      } catch (...) {
        node_alloc_traits::deallocate(alloc_, temp, 1);
        Node* ptr = head_;
        while (ptr != nullptr) {
          head_ = static_cast<Node*>(head_->next);
          node_alloc_traits::destroy(alloc_, ptr);
          node_alloc_traits::deallocate(alloc_, ptr, 1);
          ptr = head_;
        }
        throw;
      }
      temp->prev = current;
      current = temp;
      if (temp->prev != nullptr) {
        temp->prev->next = current;
      }
      if (it == init.begin()) {
        head_ = temp;
      } else if (it == init.end() - 1) {
        tail_ = temp;
      }
    }
    fake_ = &fake_node_;
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  List(const List& other, const Allocator& alloc) : alloc_(alloc) {
    size_ = other.size_;
    Node* current = nullptr;
    Node* other_current = other.head_;
    for (size_t i = 0; i < size_; ++i) {
      Node* temp = nullptr;
      try {
        temp = node_alloc_traits::allocate(alloc_, 1);
      } catch (const std::bad_alloc&) {
        throw;
      }
      try {
        node_alloc_traits::construct(alloc_, temp, other_current->value);
      } catch (...) {
        node_alloc_traits::deallocate(alloc_, temp, 1);
        Node* ptr = head_;
        while (ptr != nullptr) {
          head_ = static_cast<Node*>(head_->next);
          node_alloc_traits::destroy(alloc_, ptr);
          node_alloc_traits::deallocate(alloc_, ptr, 1);
          ptr = head_;
        }
        throw;
      }
      temp->prev = current;
      current = temp;
      if (temp->prev != nullptr) {
        temp->prev->next = current;
      }
      if (i == 0) {
        head_ = temp;
      } else if (i == size_ - 1) {
        tail_ = temp;
      }
      other_current = static_cast<Node*>(other_current->next);
    }
    fake_ = &fake_node_;
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  ~List() {
    Node* current = head_;
    for (size_t i = 0; i < size_; ++i) {
      current = head_;
      head_ = static_cast<Node*>(head_->next);
      node_alloc_traits::destroy(alloc_, current);
      node_alloc_traits::deallocate(alloc_, current, 1);
    }
  }
  List& operator=(const List& other) {
    if (this != &other) {
      if (alloc_traits::propagate_on_container_copy_assignment::value) {
        alloc_ = other.alloc_;
      }
      List copy(other, alloc_);
      std::swap(size_, copy.size_);
      std::swap(head_, copy.head_);
      std::swap(tail_, copy.tail_);
      std::swap(fake_, copy.fake_);
    }
    return *this;
  }

  T& front() { return head_->value; }
  const T& front() const { return head_->value; }
  T& back() { return tail_->value; }
  const T& back() const { return tail_->value; };
  bool empty() { return (size_ == 0); }

  size_t size() const { return size_; }
  void push_front(const T& value) {
    size_++;
    Node* temp = nullptr;
    try {
      temp = node_alloc_traits::allocate(alloc_, 1);
    } catch (const std::bad_alloc&) {
      throw;
    }
    try {
      temp->value = value;
    } catch (...) {
      node_alloc_traits::deallocate(alloc_, temp, 1);
      throw;
    }
    temp->next = head_;
    temp->prev = nullptr;
    head_->prev = temp;
    head_ = temp;
    if (size_ == 1) {
      tail_ = head_;
    }
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  void push_back(const T& value) {
    size_++;
    Node* temp;
    try {
      temp = node_alloc_traits::allocate(alloc_, 1);
    } catch (const std::bad_alloc&) {
      throw;
    }
    try {
      temp->value = value;
    } catch (...) {
      node_alloc_traits::deallocate(alloc_, temp, 1);
      throw;
    }
    temp->prev = tail_;
    temp->next = nullptr;
    tail_->next = temp;
    tail_ = temp;
    if (size_ == 1) {
      head_ = tail_;
    }
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  void push_back(T&& value) {
    size_++;
    Node* temp;
    try {
      temp = node_alloc_traits::allocate(alloc_, 1);
    } catch (const std::bad_alloc&) {
      throw;
    }
    try {
      temp->value = std::move(value);
    } catch (...) {
      node_alloc_traits::deallocate(alloc_, temp, 1);
      throw;
    }
    temp->prev = tail_;
    temp->next = nullptr;
    if (tail_ != nullptr) {
      tail_->next = temp;
    }
    tail_ = temp;
    if (size_ == 1) {
      head_ = tail_;
    }
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  void push_front(T&& value) {
    size_++;
    Node* temp = nullptr;
    try {
      temp = node_alloc_traits::allocate(alloc_, 1);
    } catch (const std::bad_alloc&) {
      throw;
    }
    try {
      temp->value = std::move(value);
    } catch (...) {
      node_alloc_traits::deallocate(alloc_, temp, 1);
      throw;
    }
    temp->next = head_;
    temp->prev = nullptr;
    if (head_ != nullptr) {
      head_->prev = temp;
    }
    head_ = temp;
    if (size_ == 1) {
      tail_ = head_;
    }
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  void pop_back() {
    if (size_ == 0) {
      return;
    }
    size_--;
    Node* temp = tail_;
    tail_ = static_cast<Node*>(tail_->prev);
    tail_->next = nullptr;
    node_alloc_traits::destroy(alloc_, temp);
    node_alloc_traits::deallocate(alloc_, temp, 1);
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  void pop_front() {
    if (size_ == 0) {
      return;
    }
    size_--;
    Node* temp = head_;
    head_ = static_cast<Node*>(head_->next);
    head_->prev = nullptr;
    node_alloc_traits::destroy(alloc_, temp);
    node_alloc_traits::deallocate(alloc_, temp, 1);
    head_->prev = fake_;
    tail_->next = fake_;
    fake_->next = head_;
    fake_->prev = tail_;
  }
  allocator_type get_allocator() { return alloc_; }
  iterator begin() const { return iterator(head_, head_, tail_, fake_); };
  iterator end() const { return ++iterator(tail_, head_, tail_, fake_); }
  const_iterator cbegin() const {
    return const_iterator(head_, head_, tail_, fake_);
  }
  const_iterator cend() const { return ++iterator(tail_, head_, tail_, fake_); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }
  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }
};