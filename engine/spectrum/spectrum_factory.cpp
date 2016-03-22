/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Qpx::Spectrum::Spectrum generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include "spectrum_factory.h"
#include "custom_logger.h"

namespace Qpx {
namespace Spectrum {

Spectrum* Factory::create_type(std::string type)
{
  Spectrum *instance = nullptr;
  auto it = constructors.find(type);
  if(it != constructors.end())
    instance = it->second();
  return instance;
}

Spectrum* Factory::create_from_prototype(const Metadata& tem)
{
  Spectrum* instance = create_type(tem.type());
  if (instance != nullptr) {
    bool success = instance->from_prototype(tem);
    if (success)
      return instance;
    else {
      delete instance;
      return nullptr;
    }
  }
}

Spectrum* Factory::create_from_xml(const pugi::xml_node &root)
{
  if (std::string(root.name()) != "Spectrum")
    return nullptr;
  if (!root.attribute("type"))
    return nullptr;

  Spectrum* instance = create_type(std::string(root.attribute("type").value()));
  if (instance != nullptr) {
    bool success = instance->from_xml(root);
    if (success)
      return instance;
    else {
      delete instance;
      return nullptr;
    }
  }
}

Spectrum* Factory::create_from_file(std::string filename)
{
  std::string ext(boost::filesystem::extension(filename));
  if (ext.size())
    ext = ext.substr(1, ext.size()-1);
  boost::algorithm::to_lower(ext);
  Spectrum *instance = nullptr;
  auto it = ext_to_type.find(ext);
  if (it != ext_to_type.end())
    instance = create_type(it->second);;
  if (instance != nullptr) {
    bool success = instance->read_file(filename, ext);
    if (success)
      return instance;
    else {
      delete instance;
      return nullptr;
    }
  }
}

Metadata* Factory::create_prototype(std::string type)
{
  auto it = prototypes.find(type);
  if(it != prototypes.end())
    return new Metadata(it->second);
  else
    return nullptr;
}

void Factory::register_type(Metadata tt, std::function<Spectrum*(void)> typeConstructor)
{
  PL_INFO << "<Spectrum::Factory> registering spectrum type '" << tt.type() << "'";
  constructors[tt.type()] = typeConstructor;
  prototypes[tt.type()] = tt;
  for (auto &q : tt.input_types())
    ext_to_type[q] = tt.type();
}

const std::vector<std::string> Factory::types() {
  std::vector<std::string> all_types;
  for (auto &q : constructors)
    all_types.push_back(q.first);
  return all_types;
}

}}
