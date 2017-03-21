#pragma once

#include "json.hpp"
#include "H5CC_Enum.h"
#include "H5CC_File.h"

using json = nlohmann::json;

namespace H5CC
{

void to_json(json& j, const Enum<int16_t>& e);
void from_json(const json& j, Enum<int16_t>& e);

void to_json(json& j, const H5CC::DataSet& d);

template<typename T> void to_json(json& j, const H5CC::Groupoid<T>& g);

template<typename T> void from_json(const json& j, H5CC::Groupoid<T>& g);

template<typename T> json attribute_to_json(const H5CC::Location<T>& g,
                                            const std::string& name);
template<typename T> void attribute_from_json(const json& j, const std::string& name,
                                              H5CC::Location<T>& g);
template<typename T> void dataset_from_json(const json& j, const std::string& name,
                                            H5CC::Groupoid<T>& g);

}

#include "JsonH5.tpp"

