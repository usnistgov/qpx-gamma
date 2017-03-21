#include <sstream>
#include "H5CC_Types.h"

namespace H5CC
{

#define TT template<typename T>

TT Enum<T>::Enum(std::list<std::string> options)
{
  T t {0};
  for (auto o : options)
  {
    options_[t] = o;
    ++t;
  }
}

TT void Enum<T>::set_option(T t, std::string o)
{
  options_[t] = o;
}

TT void Enum<T>::set(T t)
{
  if (options_.count(t))
    val_ = t;
}

TT void Enum<T>::choose(std::string choice)
{
  int key = val_;
  for (auto it : options_)
    if (it.second == choice)
    {
      key = it.first;
      break;
    }

  val_ = key;
}

TT std::map<T, std::string> Enum<T>::options() const
{
  return options_;
}

TT T Enum<T>::val() const
{
  return val_;
}

TT std::string Enum<T>::choice() const
{
  if (options_.count(val_))
    return options_.at(val_);
  else
    return "";
}

TT H5::DataType Enum<T>::h5_type() const
{
  H5::EnumType t(pred_type_of(T()));
  for (auto i : options_)
  {
    auto v = i.first;
    t.insert(i.second, &v);
  }
  return t;
}

TT bool Enum<T>::operator == (const Enum<T>& other)
{
  return (options_ == other.options_) && (val_ == other.val_);
}

TT void Enum<T>::write(H5::Attribute& attr) const
{
  auto val = val_;
  attr.write(h5_type(), &val);
}

TT void Enum<T>::read(const H5::Attribute& attr)
{
  auto dtype = attr.getEnumType();
  options_.clear();
  for (int i=0; i < dtype.getNmembers(); ++i)
  {
    T t;
    dtype.getMemberValue(i, &t);
    std::string name = dtype.nameOf(&t, 1000);
    options_[t] = name;
  }
  attr.read(h5_type(), &val_);
}

TT std::string Enum<T>::to_string() const
{
  std::stringstream ss;
  ss << val_;
  if (options_.count(val_))
    ss << "(" << options_.at(val_) << ")";

  std::string opts;
  for (auto o : options_)
    opts += "\"" + o.second + "\", ";
  if (!opts.empty())
    ss << "  [" << opts.substr(0, opts.size()-2) << "]";

  return ss.str();
}

// prefix
TT Enum<T>& Enum<T>::operator++()
{
  if (!options_.empty())
  {
    auto current = options_.find(val_);
    if ((current == options_.end()) || ((++current) == options_.end()))
      val_ = options_.rbegin()->first;
    else
      val_ = current->first;
  }
  else
    val_ = -1;
  return *this;
}

TT Enum<T>& Enum<T>::operator--()
{
  if (!options_.empty())
  {
    auto current = std::map<int, std::string>::reverse_iterator(options_.find(val_));
    if ((current == options_.rend()) || ((++current) == options_.rend()))
      val_ = options_.begin()->first;
    else
      val_ = current->first;
  }
  else
    val_ = -1;
  return *this;
}

// postfix
TT Enum<T> Enum<T>::operator++(int)
{
  Enum tmp(*this);
  operator++();
  return tmp;
}

TT Enum<T> Enum<T>::operator--(int)
{
  Enum tmp(*this);
  operator--();
  return tmp;
}

#undef TT

}



