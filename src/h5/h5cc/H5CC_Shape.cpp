#include "H5CC_Shape.h"
#include "H5CC_Exception.h"
#include <sstream>

//#include <iostream>

namespace H5CC {

Shape::Shape(const H5::DataSpace& sp)
{
  std::vector<hsize_t> dims;
  std::vector<hsize_t> max_dims;
  try
  {
    int rank = sp.getSimpleExtentNdims();
    if (rank > 0)
    {
      dims.resize(rank, 0);
      max_dims.resize(rank, 0);
      sp.getSimpleExtentDims(dims.data(), max_dims.data());
    }
  }
  catch (...)
  {
    Exception::rethrow();
  }
  dataspace_ = sp;
  dims_ = dims;
  max_dims_ = max_dims;
}

Shape::Shape (std::vector<hsize_t> dimensions,
              std::vector<hsize_t> max_dimensions)
{
  try
  {
    if (max_dimensions.size() && (max_dimensions.size() == dimensions.size()))
      dataspace_ = H5::DataSpace(dimensions.size(), dimensions.data(), max_dimensions.data());
    else
    {
      dataspace_ = H5::DataSpace(dimensions.size(), dimensions.data());
      max_dimensions = dimensions;
    }
    dims_ = dimensions;
    max_dims_ = max_dimensions;
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

size_t Shape::rank() const
{
  return dims_.size();
}

hsize_t Shape::dim(size_t d) const
{
  if (d < dims_.size())
    return dims_.at(d);
  else
    return 0;
}

hsize_t Shape::max_dim(size_t d) const
{
  if (d < max_dims_.size())
    return max_dims_.at(d);
  else
    return 0;
}

Shape Shape::slab_shape(std::vector<hsize_t> list) const
{
  Shape ret;
  if (list.size() <= dims_.size())
  {
    auto newdims = dims_;
    int i=0;
    for (auto &d : list)
    {
      if ((d != kMax) && (d < dims_[i]))
        newdims[i] = d;
      i++;
    }

    try
    {
      ret.dataspace_ = H5::DataSpace(newdims.size(), newdims.data());
    }
    catch (...)
    {
      Exception::rethrow();
    }

    ret.dims_ = newdims;
  }
  return ret;
}

std::vector<hsize_t> Shape::max_extent(const Shape& slab, std::vector<hsize_t> index) const
{
  std::vector<hsize_t> ret;
  for (size_t i=0; i < rank(); ++i)
    ret.push_back(std::max(dim(i), slab.dim(i) + index.at(i)));
  return ret;
}

size_t Shape::data_size() const
{
  if (!rank())
    return 0;
  size_t ret = 1;
  for (auto &d : dims_)
    ret *= d;
  return ret;
}

bool Shape::fits_space(const std::vector<hsize_t> superset,
                       const std::vector<hsize_t> subset)
{
  if (subset.empty() || (superset.size() != subset.size()))
    return false;
  for (size_t i=0; i < subset.size(); ++i)
    if (subset.at(i) > superset.at(i))
      return false;
  return true;
}

bool Shape::fits_slab(const std::vector<hsize_t> superset,
                      const std::vector<hsize_t> coords,
                      const std::vector<hsize_t> subset)
{
  if (subset.empty() ||
      (superset.size() != subset.size()) ||
      (coords.size() != subset.size()))
    return false;
  for (size_t i=0; i < subset.size(); ++i)
    if ((coords.at(i) + subset.at(i)) > superset.at(i))
      return false;
  return true;
}

bool Shape::contains(const Shape& other) const
{
  return fits_space(dims_, other.dims_);
}

bool Shape::contains(const Shape& other, const std::vector<hsize_t>& coords) const
{
  return fits_slab(dims_, coords, other.dims_);
}

bool Shape::can_contain(const Shape& other) const
{
  return fits_space(max_dims_, other.dims_);
}

bool Shape::can_contain(const Shape& other, const std::vector<hsize_t>& coords) const
{
  return fits_slab(max_dims_, coords, other.dims_);
}

void Shape::select_slab(const Shape& slabspace, std::vector<hsize_t> index)
{
  if (!contains(slabspace, index))
    throw std::out_of_range("slab selection out of data shape range");
  try
  {
    dataspace_.selectHyperslab(H5S_SELECT_SET, slabspace.dims_.data(), index.data());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

void Shape::select_element(std::vector<hsize_t> index)
{
  if (!contains(Shape(index)))
    throw std::out_of_range("element selection out of data shape range");
  try
  {
    dataspace_.selectElements(H5S_SELECT_SET, 1, index.data());
  }
  catch (...)
  {
    Exception::rethrow();
  }
}

std::string Shape::debug() const
{
  std::stringstream ss;
  ss << dims_to_string(dims_);
  if (!max_dims_.empty() && (max_dims_ != dims_))
  {
    ss << " max(" << dims_to_string(max_dims_) << ")";
  }
  if (!dataspace_.isSimple())
    ss << " nonsimple";
  return ss.str();
}

bool Shape::extendable(const std::vector<hsize_t> dims)
{
  for (const auto &d : dims)
    if (d == kMax)
      return true;
  return false;
}

bool Shape::is_extendable() const
{
  return extendable(max_dims_) && can_contain(Shape(dims_));
}

std::string Shape::dims_to_string(const std::vector<hsize_t>& d)
{
  if (d.empty())
    return "nullsize";
  std::stringstream ss;
  if (d.at(0) == kMax)
    ss << "U";
  else
    ss << d.at(0);
  for (size_t i=1; i < d.size(); ++i)
    if (d.at(i) == kMax)
      ss << "xU";
    else
      ss << "x" << d.at(i);
  return ss.str();
}


}
