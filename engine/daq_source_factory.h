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
 *      Qpx::Source abstract class
 *
 ******************************************************************************/

#pragma once

#include <memory>
#include "daq_source.h"

namespace Qpx {

class SourceFactory {
public:
  static SourceFactory& getInstance()
  {
    static SourceFactory singleton_instance;
    return singleton_instance;
  }

  SourcePtr create_type(std::string type, std::string file);
  void register_type(std::string name, std::function<Source*(void)> typeConstructor);
  const std::vector<std::string> types();

private:
  std::map<std::string, std::function<Source*(void)>> constructors;

  //singleton assurance
  SourceFactory() {}
  SourceFactory(SourceFactory const&);
  void operator=(SourceFactory const&);
};

template<class T>
class SourceRegistrar {
public:
  SourceRegistrar(std::string)
  {
    SourceFactory::getInstance().register_type(T::plugin_name(),
                                               [](void) -> Source * { return new T();});
  }
};

}
