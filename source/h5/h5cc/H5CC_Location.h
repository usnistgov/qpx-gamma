#pragma once

#include "H5CC_Common.h"
#include <list>
#include "H5CC_Types.h"
#include "H5CC_Enum.h"

namespace H5CC {

template<typename T>
class Location
{
public:
  Location();
  Location(T t, std::string name);
  std::string name() const;

  std::list<std::string> attributes() const;
  bool has_attribute(std::string name) const;
  void remove_attribute(std::string name);
  template <typename DT> bool attr_has_type(const std::string& name) const;
  template <typename DT> bool attr_is_enum(const std::string& name) const;

  template <typename DT> void write_attribute(std::string name, DT val);
  template <typename DT> DT read_attribute(std::string name) const;

  template <typename DT> void write_enum(std::string name, const Enum<DT>& val);
  template <typename DT> Enum<DT> read_enum(std::string name) const;


protected:
  T location_;
  std::string name_;

  template<typename DT>
  void attr_write(H5::Attribute& attr, DT val);
  void attr_write(H5::Attribute& attr, std::string val);

  template<typename DT>
  void attr_read(const H5::Attribute& attr, DT& val) const;
  void attr_read(const H5::Attribute& attr, std::string& val) const;
};

}

#include "H5CC_Location.tpp"

