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
 *      Qpx::Consumer::Consumer generic Consumer type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include "consumer_factory.h"
#include "custom_logger.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace Qpx {

SinkPtr ConsumerFactory::create_type(std::string type)
{
  auto it = constructors.find(type);
  if(it != constructors.end())
    return SinkPtr(it->second());
  else
    return SinkPtr();
}

SinkPtr ConsumerFactory::create_copy(SinkPtr other)
{
  return SinkPtr(other->clone());
}


SinkPtr ConsumerFactory::create_from_prototype(const ConsumerMetadata& tem)
{
//  DBG << "<ConsumerFactory> creating " << tem.type();
  SinkPtr instance = create_type(tem.type());
  if (instance && instance->from_prototype(tem))
    return instance;
  return SinkPtr();
}

SinkPtr ConsumerFactory::create_from_xml(const pugi::xml_node &root)
{
  if (!root.attribute("type"))
    return SinkPtr();

//  DBG << "<ConsumerFactory> making " << root.attribute("type").value();

  SinkPtr instance = create_type(std::string(root.attribute("type").value()));
  if (instance && instance->load(root))
    return instance;

  return SinkPtr();
}

#ifdef H5_ENABLED
SinkPtr ConsumerFactory::create_from_h5(H5CC::Group &group, bool withdata)
{
  if (!group.has_attribute("type"))
    return SinkPtr();

//  DBG << "<ConsumerFactory> making " << root.attribute("type").value();

  SinkPtr instance = create_type(group.read_attribute<std::string>("type"));
  if (instance && instance->load(group, withdata))
    return instance;

  return SinkPtr();
}
#endif

SinkPtr ConsumerFactory::create_from_file(std::string filename)
{
  std::string ext(boost::filesystem::extension(filename));
  if (ext.size())
    ext = ext.substr(1, ext.size()-1);
  boost::algorithm::to_lower(ext);
  auto it = ext_to_type.find(ext);
  SinkPtr instance;
  if (it != ext_to_type.end())
    instance = SinkPtr(create_type(it->second));
  if (instance && instance->read_file(filename, ext))
    return instance;
  return SinkPtr();
}

ConsumerMetadata ConsumerFactory::create_prototype(std::string type)
{
  auto it = prototypes.find(type);
  if(it != prototypes.end())
    return it->second;
  else
    return ConsumerMetadata();
}

void ConsumerFactory::register_type(ConsumerMetadata tt, std::function<Consumer*(void)> typeConstructor)
{
  LINFO << "<ConsumerFactory> registering sink type '" << tt.type() << "'";
  constructors[tt.type()] = typeConstructor;
  prototypes[tt.type()] = tt;
  for (auto &q : tt.input_types())
    ext_to_type[q] = tt.type();
}

const std::vector<std::string> ConsumerFactory::types() {
  std::vector<std::string> all_types;
  for (auto &q : constructors)
    all_types.push_back(q.first);
  return all_types;
}

}
