#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

class BigInt {
 public:
  BigInt();
  BigInt(int64_t);
  BigInt(const std::string&);
  BigInt(const BigInt&);

  friend bool operator==(const BigInt&, const BigInt&);
  friend bool operator!=(const BigInt&, const BigInt&);
  bool operator<(const BigInt&) const;
  bool operator>(const BigInt&) const;
  bool operator<=(const BigInt&) const;
  bool operator>=(const BigInt&) const;

  BigInt& operator=(const BigInt&);
  BigInt operator-() const;
  BigInt operator+=(const BigInt&);
  BigInt operator-=(const BigInt&);
  BigInt operator*=(const BigInt&);
  BigInt operator/=(const BigInt&);
  BigInt operator%=(const BigInt&);

  friend BigInt operator+(const BigInt&, const BigInt&);
  friend BigInt operator-(const BigInt&, const BigInt&);
  friend BigInt operator*(const BigInt&, const BigInt&);
  friend BigInt operator/(const BigInt&, const BigInt&);
  friend BigInt operator%(const BigInt&, const BigInt&);

  BigInt operator++();
  BigInt operator++(int);
  BigInt operator--();
  BigInt operator--(int);

  BigInt& GeneralDiv(const BigInt&);
  BigInt& DeleteZeros();
  BigInt& OneSignPlus(const BigInt&);
  BigInt& DiffSignPlus(const BigInt&);

  friend std::ostream& operator<<(std::ostream&, const BigInt&);
  friend std::istream& operator>>(std::istream&, BigInt&);

 private:
  void SubtractNumbers(BigInt& first, const BigInt& second);

  std::vector<int> number_;
  bool is_negative_;
  bool is_null_;
  const int kBase = 10;
};
