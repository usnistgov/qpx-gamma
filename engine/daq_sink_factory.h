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
 *
 *      Qpx::Sink::Factory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::Sink::Registrar for registering new Sink types.
 *
 ******************************************************************************/

#ifndef SINK_FACTORY_H
#define SINK_FACTORY_H

#include <memory>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "daq_sink.h"

namespace Qpx {

class SinkFactory {
 public:
  static SinkFactory& getInstance()
  {
    static SinkFactory singleton_instance;
    return singleton_instance;
  }

  void register_type(Metadata tt, std::function<Sink*(void)> typeConstructor);
  const std::vector<std::string> types();
  
  SinkPtr create_type(std::string type);
  SinkPtr create_from_prototype(const Metadata& tem);
  SinkPtr create_from_xml(const pugi::xml_node &root);
  SinkPtr create_from_file(std::string filename);
  SinkPtr create_copy(SinkPtr other);

  Metadata create_prototype(std::string type);

 private:
  std::map<std::string, std::function<Sink*(void)>> constructors;
  std::map<std::string, std::string> ext_to_type;
  std::map<std::string, Metadata> prototypes;

  //singleton assurance
  SinkFactory() {}
  SinkFactory(SinkFactory const&);
  void operator=(SinkFactory const&);
};

template<class T>
class SinkRegistrar {
public:
  SinkRegistrar(std::string)
  {
    SinkFactory::getInstance().register_type(T().metadata(),
                                         [](void) -> Sink * { return new T();});
  }
};


}

#endif
