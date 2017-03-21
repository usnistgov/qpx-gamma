#include "H5CC_DataSet.h"
#include "H5CC_Exception.h"
#include "H5CC_Enum.h"
#include <sstream>

namespace H5CC {

DataSet::DataSet() {}

DataSet::DataSet(H5::DataSet ds, std::string name) : Location<H5::DataSet>(ds, name)
{
  try
  {
    shape_ = Shape(ds.getSpace());
    type_ = ds.getDataType();
  }
  catch (...)
  {
    shape_ = Shape();
    Exception::rethrow();
  }
}

Shape DataSet::slab_shape(const std::vector<hsize_t>& list) const
{
  return shape_.slab_shape(list);
}

Shape DataSet::shape() const
{
  return shape_;
}

H5::DataType DataSet::type() const
{
  return type_;
}

bool DataSet::is_chunked() const
{
  auto prop = Location<H5::DataSet>::location_.getCreatePlist();
  return (H5D_CHUNKED == prop.getLayout());
}


Shape DataSet::chunk_shape() const
{
  auto prop = Location<H5::DataSet>::location_.getCreatePlist();
  if (H5D_CHUNKED != prop.getLayout())
    return Shape();
  std::vector<hsize_t> ret;
  ret.resize(shape_.rank());
  prop.getChunk(ret.size(), ret.data());
  return Shape(ret);
}

std::string DataSet::debug() const
{
  std::stringstream ss;
  ss << name() << " " << shape_.debug()
     << " (" << shape_.data_size() << ")";
  return ss.str();
}




}
