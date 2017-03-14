#pragma once

#include "H5CC_Common.h"
#include "H5DataType.h"
#include <string>
#include <map>
#include <list>

namespace H5CC {

template <typename T>
class Enum
{
public:
  Enum() {}
  Enum(T t, std::map<T, std::string> options) : val_(t), options_(options) {}
  Enum(std::list<std::string> options);

  std::map<T, std::string> options() const;
  void set_option(T t, std::string o);

  void set(T t);
  T val() const;

  void choose(std::string t);
  std::string choice() const;


  H5::DataType h5_type() const;

  bool operator == (const Enum& other);

  void write(H5::Attribute& attr) const;
  void read(const H5::Attribute& attr);

  std::string to_string() const;

  // prefix
  Enum& operator++();
  Enum& operator--();
  // postfix
  Enum operator++(int);
  Enum operator--(int);

protected:
  T val_ {T(-1)};
  std::map<T, std::string> options_;
};

}

#include "H5CC_Enum.tpp"
