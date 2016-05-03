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
 *      Qpx::Sink::Sink generic Sink type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 ******************************************************************************/

#include "daq_sink_factory.h"
#include "custom_logger.h"

namespace Qpx {

SinkPtr SinkFactory::create_type(std::string type)
{
  auto it = constructors.find(type);
  if(it != constructors.end())
    return SinkPtr(it->second());
  else
    return SinkPtr();
}

SinkPtr SinkFactory::create_from_prototype(const Metadata& tem)
{
  SinkPtr instance = create_type(tem.type());
  if (instance && instance->from_prototype(tem));
    return instance;
  return SinkPtr();
}

SinkPtr SinkFactory::create_from_xml(const pugi::xml_node &root)
{
//  if (std::string(root.name()) != "Spectrum")               //GENERALIZE
//    return SinkPtr();
  if (!root.attribute("type"))
    return SinkPtr();

//  DBG << "<SinkFactory> making " << root.attribute("type").value();

  SinkPtr instance = create_type(std::string(root.attribute("type").value()));
  if (instance && instance->from_xml(root))
    return instance;

  return SinkPtr();
}

SinkPtr SinkFactory::create_from_file(std::string filename)
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

Metadata SinkFactory::create_prototype(std::string type)
{
  auto it = prototypes.find(type);
  if(it != prototypes.end())
    return it->second;
  else
    return Metadata();
}

void SinkFactory::register_type(Metadata tt, std::function<Sink*(void)> typeConstructor)
{
  INFO << "<SinkFactory> registering sink type '" << tt.type() << "'";
  constructors[tt.type()] = typeConstructor;
  prototypes[tt.type()] = tt;
  for (auto &q : tt.input_types())
    ext_to_type[q] = tt.type();
}

const std::vector<std::string> SinkFactory::types() {
  std::vector<std::string> all_types;
  for (auto &q : constructors)
    all_types.push_back(q.first);
  return all_types;
}

}
