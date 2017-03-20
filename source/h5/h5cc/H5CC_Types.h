#pragma once

#include "H5DataType.h"
#include "H5AtomType.h"
#include "H5PredType.h"
#include "H5EnumType.h"
#include "H5StrType.h"
#include <string>
#include <stdint.h>

namespace H5CC
{

struct pred_type_visitor
{
  inline H5::PredType operator () (const int8_t&) const  { return H5::PredType::NATIVE_INT8; }
  inline H5::PredType operator () (const int16_t&) const { return H5::PredType::NATIVE_INT16; }
  inline H5::PredType operator () (const int32_t&) const { return H5::PredType::NATIVE_INT32; }
  inline H5::PredType operator () (const int64_t&) const { return H5::PredType::NATIVE_INT64; }

  inline H5::PredType operator () (const uint8_t&) const  { return H5::PredType::NATIVE_UINT8; }
  inline H5::PredType operator () (const uint16_t&) const { return H5::PredType::NATIVE_UINT16; }
  inline H5::PredType operator () (const uint32_t&) const { return H5::PredType::NATIVE_UINT32; }
  inline H5::PredType operator () (const uint64_t&) const { return H5::PredType::NATIVE_UINT64; }

  inline H5::PredType operator () (const float&) const { return H5::PredType::NATIVE_FLOAT; }
  inline H5::PredType operator () (const double&) const { return H5::PredType::NATIVE_DOUBLE; }
  inline H5::PredType operator () (const long double&) const { return H5::PredType::NATIVE_LDOUBLE; }
};

struct type_visitor
{
  inline H5::PredType operator () (const int8_t&) const  { return H5::PredType::NATIVE_INT8; }
  inline H5::PredType operator () (const int16_t&) const { return H5::PredType::NATIVE_INT16; }
  inline H5::PredType operator () (const int32_t&) const { return H5::PredType::NATIVE_INT32; }
  inline H5::PredType operator () (const int64_t&) const { return H5::PredType::NATIVE_INT64; }

  inline H5::PredType operator () (const uint8_t&) const  { return H5::PredType::NATIVE_UINT8; }
  inline H5::PredType operator () (const uint16_t&) const { return H5::PredType::NATIVE_UINT16; }
  inline H5::PredType operator () (const uint32_t&) const { return H5::PredType::NATIVE_UINT32; }
  inline H5::PredType operator () (const uint64_t&) const { return H5::PredType::NATIVE_UINT64; }

  inline H5::PredType operator () (const float&) const { return H5::PredType::NATIVE_FLOAT; }
  inline H5::PredType operator () (const double&) const { return H5::PredType::NATIVE_DOUBLE; }
  inline H5::PredType operator () (const long double&) const { return H5::PredType::NATIVE_LDOUBLE; }

  inline H5::StrType operator () (const std::string&) const
  {
    return H5::StrType(H5::PredType::C_S1, H5T_VARIABLE);
  }

  inline H5::EnumType operator () (const bool&) const
  {
    H5::EnumType t(H5::PredType::NATIVE_INT8);
    int8_t True = 1;
    int8_t False = 0;
    t.insert("True", &True);
    t.insert("False", &False);
    return t;
  }
};

template <typename T>
inline H5::DataType type_of(const T& t)
{
  type_visitor v;
  return v(t);
}

template <typename T>
inline H5::PredType pred_type_of(const T& t)
{
  pred_type_visitor v;
  return v(t);
}

}
