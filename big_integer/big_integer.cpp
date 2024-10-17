#include "big_integer.hpp"

BigInt::BigInt() {
  is_negative_ = false;
  is_null_ = false;
}

BigInt::BigInt(int64_t num) : is_negative_(num < 0), is_null_(num == 0) {
  std::string big;
  big = std::to_string(num);
  *this = BigInt(big);
  is_negative_ = (num < 0);
  is_null_ = (num == 0);
}

BigInt::BigInt(const BigInt& other) = default;

BigInt::BigInt(const std::string& str) {
  if (str.empty()) {
    is_negative_ = false;
    is_null_ = true;
    return;
  }
  std::string num = str;
  if (num[0] == '-') {
    is_negative_ = true;
    num = num.substr(1, num.size() - 1);
  } else {
    is_negative_ = false;
  }
  is_null_ = num.length() == 1 && num[0] == '0';
  std::vector<int64_t> diff;
  for (size_t i = 0; i < num.size(); ++i) {
    diff.push_back(num[i] - '0');
  }
  while (!diff.empty()) {
    number_.push_back(diff.back());
    diff.pop_back();
  }
  if (number_.size() == 1 && number_[0] == 0) {
    is_negative_ = false;
    is_null_ = true;
  }
}

bool operator==(const BigInt& left, const BigInt& right) {
  return (left.is_negative_ == right.is_negative_ &&
          left.is_null_ == right.is_null_ && left.number_ == right.number_);
}

bool operator!=(const BigInt& left, const BigInt& right) {
  return !(left == right);
}

bool BigInt::operator>(const BigInt& other) const {
  if (is_negative_ != other.is_negative_) {
    return other.is_negative_;
  }
  if (number_.size() != other.number_.size()) {
    if (is_negative_) {
      return number_.size() < other.number_.size();
    }
    return number_.size() > other.number_.size();
  }
  for (int ind = number_.size() - 1; ind >= 0; --ind) {
    if (number_[ind] != other.number_[ind]) {
      if (is_negative_) {
        return number_[ind] < other.number_[ind];
      }
      return number_[ind] > other.number_[ind];
    }
  }
  return false;
}

bool BigInt::operator<(const BigInt& other) const { return other > *this; }
bool BigInt::operator>=(const BigInt& other) const { return !(*this < other); }
bool BigInt::operator<=(const BigInt& other) const { return !(*this > other); }

BigInt& BigInt::operator=(const BigInt& other) {
  number_ = other.number_;
  is_negative_ = other.is_negative_;
  return *this;
}

BigInt BigInt::operator-() const {
  if (*this == 0) {
    return *this;
  }
  BigInt result = *this;
  result.is_negative_ = !result.is_negative_;
  return result;
}

BigInt operator-(const BigInt& left, const BigInt& right) {
  BigInt result = left;
  result -= right;
  return result;
}

BigInt BigInt::operator-=(const BigInt& other) {
  BigInt copy = other;
  copy.is_negative_ ^= 1;
  *this += copy;
  return *this;
}

BigInt BigInt::operator+=(const BigInt& other) {
  this->OneSignPlus(other);
  this->DiffSignPlus(other);
  this->DeleteZeros();
  return *this;
}

BigInt operator+(const BigInt& left, const BigInt& right) {
  BigInt result = left;
  result += right;
  return result;
}

BigInt operator*(const BigInt& left, const BigInt& right) {
  if (left.is_null_ || right.is_null_) {
    return {0};
  }
  BigInt copy = left;
  copy *= right;
  return copy;
}

BigInt BigInt::operator*=(const BigInt& other) {
  if (is_null_ || other.is_null_) {
    return {0};
  }
  std::vector<int> res(this->number_.size() + other.number_.size(), 0);
  for (size_t i = 0; i < this->number_.size(); ++i) {
    for (size_t j = 0; j < other.number_.size(); ++j) {
      res[i + j] += this->number_[i] * other.number_[j];
    }
  }
  int64_t add = 0;
  for (size_t i = 0; i < res.size(); i++) {
    res[i] += add;
    add = res[i] / (kBase);
    res[i] -= add * kBase;
  }
  while (add > 0) {
    res.push_back(add % kBase);
    add /= kBase;
  }
  number_ = res;
  while (number_[number_.size() - 1] == 0) {
    number_.pop_back();
  }
  if (is_negative_ == other.is_negative_ || *this == 0 || other == 0) {
    is_negative_ = false;
  } else if (is_negative_ != other.is_negative_) {
    is_negative_ = true;
  }
  return *this;
}

BigInt operator/(const BigInt& left, const BigInt& right) {
  BigInt result = left;
  result /= right;
  return result;
}

BigInt BigInt::operator/=(const BigInt& other) {
  if (other.is_null_) {
    throw("Division by zero!");
  }
  BigInt first = *this;
  first.is_negative_ = false;
  BigInt second = other;
  second.is_negative_ = false;
  if (first < second) {
    *this = BigInt(0);
    is_null_ = true;
    return *this;
  }
  if (*this == other) {
    *this = BigInt(1);
    is_negative_ = false;
    return *this;
  }
  return GeneralDiv(other);
}

BigInt operator%(const BigInt& left, const BigInt& right) {
  BigInt result = left;
  result %= right;
  return result;
}

BigInt BigInt::operator%=(const BigInt& other) {
  if (other.is_null_) {
    throw std::invalid_argument("Division by zero");
  }

  BigInt divide = *this / other;
  *this -= divide * other;
  return *this;
}

BigInt BigInt::operator++() {
  *this += 1;
  return *this;
}

BigInt BigInt::operator++(int) {
  BigInt result = *this;
  ++(*this);
  return result;
}

BigInt BigInt::operator--() {
  *this -= 1;
  return *this;
}

BigInt BigInt::operator--(int) {
  BigInt result = *this;
  --(*this);
  return result;
}

std::ostream& operator<<(std::ostream& ost, const BigInt& out) {
  if (out.is_negative_) {
    ost << '-';
  }
  for (int i = out.number_.size() - 1; i >= 0; --i) {
    ost << out.number_[i];
  }
  return ost;
}

std::istream& operator>>(std::istream& ist, BigInt& inside) {
  std::string temp;
  ist >> temp;
  BigInt answer(temp);
  inside = answer;
  return ist;
}

BigInt& BigInt::GeneralDiv(const BigInt& other) {
  is_negative_ = (other.is_negative_ != is_negative_);
  BigInt result = BigInt(0);
  BigInt big_zero = BigInt(0);
  int64_t work_size = number_.size() - 1;
  BigInt other_copy = other;
  other_copy.is_negative_ = false;

  while (work_size >= 0) {
    big_zero *= kBase;
    big_zero += BigInt(number_[work_size--]);
    int64_t ind = 0;
    while (big_zero >= (other_copy * ind)) {
      ++ind;
    }
    --ind;
    result *= kBase;
    result += ind;
    big_zero -= (other_copy * ind);
  }

  number_ = result.number_;
  DeleteZeros();
  return *this;
}

BigInt& BigInt::DeleteZeros() {
  if (!number_.empty()) {
    while (number_.back() == 0 && number_.size() > 1) {
      number_.pop_back();
    }
    if (number_.size() == 1 && number_.back() == 0) {
      is_null_ = true;
      is_negative_ = false;
    } else {
      is_null_ = false;
    }
  }
  return *this;
}

BigInt& BigInt::OneSignPlus(const BigInt& num) {
  if (is_negative_ == num.is_negative_) {
    size_t first_num_size = number_.size();
    size_t second_size_plus = num.number_.size();
    int64_t add = 0;
    if (first_num_size < second_size_plus) {
      for (size_t i = first_num_size; i < second_size_plus; ++i) {
        number_.push_back(0);
      }
    }
    for (size_t i = 0; i < number_.size(); ++i) {
      if (i < second_size_plus) {
        number_[i] += num.number_[i] + add;
      } else {
        number_[i] += add;
      }
      add = number_[i] / kBase;
      number_[i] %= kBase;
    }
    if (add > 0) {
      number_.push_back(add);
    }
  }
  return *this;
}

BigInt& BigInt::DiffSignPlus(const BigInt& num) {
  if (is_negative_ == num.is_negative_) {
    return *this;
  }

  BigInt first = *this;
  BigInt second = num;
  first.is_negative_ = false;
  second.is_negative_ = false;

  if (first < second) {
    std::swap(first.number_, second.number_);
    std::swap(first.is_null_, second.is_null_);
    is_negative_ = num.is_negative_;
  }

  SubtractNumbers(first, second);
  return *this;
}

void BigInt::SubtractNumbers(BigInt& first, const BigInt& second) {
  size_t s_size = second.number_.size();
  int64_t ind = 0;
  number_.resize(s_size, 0);

  for (size_t i = 0; i < first.number_.size(); ++i) {
    if (i < s_size) {
      first.number_[i] -= second.number_[i] + ind;
    } else {
      first.number_[i] -= ind;
    }
    ind = 0;
    if (first.number_[i] < 0) {
      ind = 1;
      first.number_[i] += kBase;
    }
  }
  number_ = first.number_;
}