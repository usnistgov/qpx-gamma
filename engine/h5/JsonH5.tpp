namespace H5CC
{

template<typename T> void to_json(json& j, const H5CC::Groupoid<T>& g)
{
  for (auto gg : g.groups())
    to_json(j[gg], g.open_group(gg));
  for (auto aa : g.attributes())
    j[aa] = attribute_to_json(g, aa);
  for (auto dd : g.datasets())
    j[dd] = g.open_dataset(dd);
}

template<typename T> json attribute_to_json(const H5CC::Location<T>& g,
                                            const std::string& name)
{
  if (g.template attr_has_type<float>(name))
    return json(g.template read_attribute<float>(name));
  else if (g.template attr_has_type<double>(name))
    return json(g.template read_attribute<double>(name));
  else if (g.template attr_has_type<long double>(name))
    return json(g.template read_attribute<long double>(name));
  else if (g.template attr_has_type<int8_t>(name))
    return json(g.template read_attribute<int8_t>(name));
  else if (g.template attr_has_type<int16_t>(name))
    return json(g.template read_attribute<int16_t>(name));
  else if (g.template attr_has_type<int32_t>(name))
    return json(g.template read_attribute<int32_t>(name));
  else if (g.template attr_has_type<int64_t>(name))
    return json(g.template read_attribute<int64_t>(name));
  else if (g.template attr_has_type<uint8_t>(name))
    return json(g.template read_attribute<uint8_t>(name));
  else if (g.template attr_has_type<uint16_t>(name))
    return json(g.template read_attribute<uint16_t>(name));
  else if (g.template attr_has_type<uint32_t>(name))
    return json(g.template read_attribute<uint32_t>(name));
  else if (g.template attr_has_type<uint64_t>(name))
    return json(g.template read_attribute<uint64_t>(name));
  else if (g.template attr_has_type<std::string>(name))
    return json(g.template read_attribute<std::string>(name));
  else if (g.template attr_has_type<bool>(name))
    return json(g.template read_attribute<bool>(name));
  else if (g.template attr_is_enum<int16_t>(name))
    return json(g.template read_enum<int16_t>(name));
  else
    return "ERROR: H5CC::to_json unimplemented attribute type";
}

template<typename T> void attribute_from_json(const json& j, const std::string& name,
                                              H5CC::Location<T>& g)
{
  if (j.count("___options") && j.count("___choice"))
  {
    Enum<int16_t> e = j;
    g.write_enum(name, e);
  }
  else if (j.is_number_float())
    g.write_attribute(name, j.get<double>());
  else if (j.is_number_unsigned())
    g.write_attribute(name, j.get<uint32_t>());
  else if (j.is_number_integer())
    g.write_attribute(name, j.get<int64_t>());
  else if (j.is_boolean())
    g.write_attribute(name, j.get<bool>());
  else if (j.is_string())
    g.write_attribute(name, j.get<std::string>());
}

template<typename T> void dataset_from_json(const json& j, const std::string& name,
                                            H5CC::Groupoid<T>& g)
{
  std::vector<hsize_t> shape = j["___shape"];
  std::vector<hsize_t> chunk;
  if (j.count("___extends") && j["___extends"].is_array())
    shape = j["___extends"].get<std::vector<hsize_t>>();
  if (j.count("___chunk") && j["___chunk"].is_array())
    chunk = j["___chunk"].get<std::vector<hsize_t>>();
  auto dset = g.template create_dataset<int>(name, shape, chunk);
  for (json::const_iterator it = j.begin(); it != j.end(); ++it)
  {
    attribute_from_json(json(it.value()), std::string(it.key()), dset);
  }
}

template<typename T> void from_json(const json& j, H5CC::Groupoid<T>& g)
{
  bool is_array = j.is_array();
  uint32_t i = 0;
  size_t len = 0;
  if (is_array && j.size())
    len = std::to_string(j.size() - 1).size();

  for (json::const_iterator it = j.begin(); it != j.end(); ++it)
  {
    std::string name;
    if (is_array)
    {
      name = std::to_string(i++);
      if (name.size() < len)
        name = std::string(len - name.size(), '0').append(name);
    }
    else
      name = it.key();

    if (it.value().count("___shape") && it.value()["___shape"].is_array())
    {
      dataset_from_json(it.value(), name, g);
    }
    else if (it.value().is_number() ||
             it.value().is_boolean() ||
             it.value().is_string() ||
             it.value().count("___options") ||
             it.value().count("___choice"))
    {
      attribute_from_json(it.value(), name, g);
    }
    else
    {
      auto gg = g.create_group(name);
      from_json(it.value(), gg);
    }
  }
}

}

