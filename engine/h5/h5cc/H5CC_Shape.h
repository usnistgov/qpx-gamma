#pragma once

#include "H5CC_Common.h"
#include <vector>

namespace H5CC {

class Shape
{
public:
  Shape() {}
  Shape(const H5::DataSpace& sp);
  Shape(std::vector<hsize_t> dimensions,
        std::vector<hsize_t> max_dimensions = {});

  Shape slab_shape(std::vector<hsize_t> list) const;
  std::vector<hsize_t> max_extent(const Shape& slab, std::vector<hsize_t> index) const;
  size_t data_size() const;

  bool contains(const Shape& other) const;
  bool contains(const Shape& other, const std::vector<hsize_t>& coords) const;

  bool can_contain(const Shape& other) const;
  bool can_contain(const Shape& other, const std::vector<hsize_t>& coords) const;

  bool is_extendable() const;

  void select_slab(const Shape& slabspace, std::vector<hsize_t> index);
  void select_element(std::vector<hsize_t> index);

  size_t rank() const;

  hsize_t dim(size_t) const;
  hsize_t max_dim(size_t) const;

  std::vector<hsize_t> shape() const { return dims_; }
  std::vector<hsize_t> max_shape() const { return max_dims_; }

  H5::DataSpace dataspace() const { return dataspace_; }

  std::string debug() const;
  static std::string dims_to_string(const std::vector<hsize_t>& d);
  static bool extendable(const std::vector<hsize_t>);
  static bool fits_space(const std::vector<hsize_t> superset,
                         const std::vector<hsize_t> subset);
  static bool fits_slab(const std::vector<hsize_t> superset,
                        const std::vector<hsize_t> coords,
                        const std::vector<hsize_t> subset);

private:
  H5::DataSpace dataspace_;
  std::vector<hsize_t> dims_;
  std::vector<hsize_t> max_dims_;

};

}
