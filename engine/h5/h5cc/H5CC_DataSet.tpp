#include "H5CC_Types.h"
#include "H5CC_Exception.h"

namespace H5CC {

#define TT template<typename T>

TT void DataSet::write(const T& data, const std::vector<hsize_t>& index)
{
  try
  {
    auto shape = shape_;
    shape.select_element(index);
    Shape dspace(std::vector<hsize_t>({1}));
    Location<H5::DataSet>::location_.write(&data, pred_type_of(T()),
                                           dspace.dataspace(), shape.dataspace());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT void DataSet::write(const std::vector<T>& data, Shape slab,
                       const std::vector<hsize_t>& index)
{
  try
  {
    auto shape = shape_;
    if (shape.contains(slab, index))
      shape.select_slab(slab, index);
    else if (shape.can_contain(slab, index))
    {
      Location<H5::DataSet>::location_.extend(shape.max_extent(slab, index).data());
      shape = shape_ = Shape(Location<H5::DataSet>::location_.getSpace());
      shape.select_slab(slab, index);
    }
    else
      throw std::out_of_range("slab selection out of range");
    Location<H5::DataSet>::location_.write(data.data(), pred_type_of(T()),
                                           slab.dataspace(), shape.dataspace());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT void DataSet::write(const std::vector<T>& data,
                       const std::vector<hsize_t>& slab_size,
                       const std::vector<hsize_t>& index)
{
  write(data, shape_.slab_shape(slab_size), index);
}

TT void DataSet::write(const std::vector<T>& data)
{
  if (data.size() != shape_.data_size())
    throw std::out_of_range("Data size does not match dataset size");
  try
  {
    Location<H5::DataSet>::location_.write(data.data(), pred_type_of(T()), shape_.dataspace());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}


TT T DataSet::read(const std::vector<hsize_t>& index) const
{
  try
  {
    T data;
    auto shape = shape_;
    shape.select_element(index);
    Shape dspace(std::vector<hsize_t>({1}));
    Location<H5::DataSet>::location_.read(&data, pred_type_of(T()),
                                          dspace.dataspace(), shape.dataspace());
    return data;
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT void DataSet::read(std::vector<T>& data, Shape slab, const std::vector<hsize_t>& index) const
{
  try
  {
    auto shape = shape_;
    shape.select_slab(slab, index);
    if (data.size() < slab.data_size())
      data.resize(slab.data_size());
    Location<H5::DataSet>::location_.read(data.data(), pred_type_of(T()),
                                          slab.dataspace(), shape.dataspace());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

TT void DataSet::read(std::vector<T>& data,
                      const std::vector<hsize_t>& slab_size,
                      const std::vector<hsize_t>& index) const
{
  read(data, shape_.slab_shape(slab_size), index);
}

TT std::vector<T> DataSet::read(const std::vector<hsize_t>& slab_size,
                                const std::vector<hsize_t>& index) const
{
  auto slab = shape_.slab_shape(slab_size);
  std::vector<T> data(slab.data_size());
  read(data, slab, index);
  return data;
}

TT std::vector<T> DataSet::read() const
{
  std::vector<T> data;
  try
  {
    data.resize(shape_.data_size());
    Location<H5::DataSet>::location_.read(data.data(), pred_type_of(T()),
                                          shape_.dataspace());
  }
  catch (...)
  {
    Exception::rethrow();
  }
  return data;
}

#undef TT

}
