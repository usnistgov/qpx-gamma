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
 *      Qpx::Spectrum::Factory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::Spectrum::Registrar for registering new spectrum types.
 *
 ******************************************************************************/

#ifndef SPECTRUM_FACTORY_H
#define SPECTRUM_FACTORY_H

#include <memory>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "spectrum.h"

namespace Qpx {
namespace Spectrum {

class Factory {
 public:
  static Factory& getInstance()
  {
    static Factory singleton_instance;
    return singleton_instance;
  }

  void register_type(Metadata tt, std::function<Spectrum*(void)> typeConstructor);
  const std::vector<std::string> types();
  
  Spectrum* create_type(std::string type);
  Spectrum* create_from_prototype(const Metadata& tem);
  Spectrum* create_from_xml(const pugi::xml_node &root);
  Spectrum* create_from_file(std::string filename);

  Metadata* create_prototype(std::string type);

 private:
  std::map<std::string, std::function<Spectrum*(void)>> constructors;
  std::map<std::string, std::string> ext_to_type;
  std::map<std::string, Metadata> prototypes;

  //singleton assurance
  Factory() {}
  Factory(Factory const&);
  void operator=(Factory const&);
};

template<class T>
class Registrar {
public:
  Registrar(std::string)
  {
    Factory::getInstance().register_type(T().metadata(),
                                         [](void) -> Spectrum * { return new T();});
  }
};


}}

#endif // SPECTRUM_H
