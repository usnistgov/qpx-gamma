#include "H5CC_Exception.h"
#include "H5CC_Types.h"

#define TT template<typename T>
#define TDT template<typename DT>

namespace H5CC {

TT Location<T>::Location()
{}

TT Location<T>::Location(T t, std::string name)
  : location_(t)
  , name_ (name)
{}

TT std::string Location<T>::name() const
{
  return name_;
}

TT std::list<std::string> Location<T>::attributes() const
{
  std::list<std::string> ret;
  try
  {
    for (int i=0; i < location_.getNumAttrs(); ++i)
    {
      H5::Attribute attr = location_.openAttribute(i);
      ret.push_back(attr.getName());
    }
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return ret;
}

TT bool Location<T>::has_attribute(std::string name) const
{
  bool ret {false};
  try
  {
    ret = location_.attrExists(name);
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return ret;
}

TT void Location<T>::remove_attribute(std::string name)
{
  try
  {
    location_.removeAttr(name);
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT TDT void Location<T>::write_attribute(std::string name, DT val)
{
  if (has_attribute(name))
    remove_attribute(name);
  try
  {
    auto attribute = location_.createAttribute(name, type_of(val), H5::DataSpace(H5S_SCALAR));
    attr_write(attribute, val);
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT TDT DT Location<T>::read_attribute(std::string name) const
{
  DT ret;
  try
  {
    auto attribute = location_.openAttribute(name);
    attr_read(attribute, ret);
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return ret;
}

TT TDT void Location<T>::write_enum(std::string name, const Enum<DT>& val)
{
  if (has_attribute(name))
    remove_attribute(name);
  try
  {
    auto attribute = location_.createAttribute(name, val.h5_type(), H5::DataSpace(H5S_SCALAR));
    val.write(attribute);
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT TDT Enum<DT> Location<T>::read_enum(std::string name) const
{
  Enum<DT> ret;
  try
  {
    auto attribute = location_.openAttribute(name);
    ret.read(attribute);
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return ret;
}

TT TDT bool Location<T>::attr_has_type(const std::string& name) const
{
  try
  {
    auto attribute = location_.openAttribute(name);
    return (attribute.getDataType() == type_of(DT()));
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return false;
}

TT TDT bool Location<T>::attr_is_enum(const std::string& name) const
{
  try
  {
    auto dtype = location_.openAttribute(name).getDataType();
    return (dtype.getClass() == H5T_ENUM) &&
           (dtype.getSuper() == type_of(DT()));
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return false;
}

TT TDT void Location<T>::attr_write(H5::Attribute& attr, DT val)
{
  attr.write(type_of(val), &val );
}

TT void Location<T>::attr_write(H5::Attribute& attr, std::string val)
{
  attr.write(type_of(val), val );
}

TT TDT void Location<T>::attr_read(const H5::Attribute& attr, DT& val) const
{
  attr.read(type_of(val), &val );
}

TT void Location<T>::attr_read(const H5::Attribute& attr, std::string& val) const
{
  attr.read(type_of(val), val );
}


}

#undef TT
#undef TDT
