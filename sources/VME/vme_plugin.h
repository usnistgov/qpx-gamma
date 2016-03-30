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
 *      QpxVmePlugin
 *
 ******************************************************************************/

#ifndef QPX_VME_PLUGIN
#define QPX_VME_PLUGIN

#include "daq_source.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

class VmeController;
class VmeModule;

namespace Qpx {

class QpxVmePlugin : public Source {
  
public:
  QpxVmePlugin();
  ~QpxVmePlugin();

  static std::string plugin_name() {return "VME";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override;


  bool daq_init();
  bool daq_start(SynchronizedQueue<Spill*>* out_queue);
  bool daq_stop();
  bool daq_running();

private:
  //no copying
  void operator=(QpxVmePlugin const&);
  QpxVmePlugin(const QpxVmePlugin&);

protected:

  void rebuild_structure(Qpx::Setting &set);

  bool read_register(Qpx::Setting& set) const;
  bool write_register(Qpx::Setting& set);

  std::string controller_name_;
  VmeController *controller_;

  std::map<std::string, std::shared_ptr<VmeModule>> modules_;

  //Multithreading
  boost::atomic<int> run_status_;
  boost::thread *runner_;
  boost::thread *parser_;
  SynchronizedQueue<Spill*>* raw_queue_;

};


}

#endif
