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
 *      Qpx::Project container class for managing simultaneous
 *      acquisition and output of spectra. Thread-safe, contingent
 *      upon stored spectra types being thread-safe.
 *
 ******************************************************************************/

#ifndef SPECTRA_SET_H
#define SPECTRA_SET_H

#include "daq_sink.h"

namespace Qpx {

class Project {
 public:
  Project(): ready_(false), newdata_(false), terminating_(false), changed_(false),
    identity_("New project") {}
  ~Project();

  ////control//////
  void clear();
  void activate();    //force release of cond var
  void terminate();   //explicitly in case other threads waiting for cond var

  //populate one of these ways
  void set_spectra(const XMLableDB<Metadata>&);
  void add_spectrum(std::shared_ptr<Sink> newSpectrum);
  void add_spectrum(Metadata spectrum);

  void import_spn(std::string file_name);

  void save();
  void save_as(std::string file_name);

  void read_xml(std::string file_name, bool with_spectra = true, bool with_full_spectra = true);

  void delete_spectrum(std::string name);

  //acquisition feeds events to all spectra
  void add_spill(Spill* one_spill);
  void flush();

  //status inquiry
  bool wait_ready(); //wait for cond variable
  bool new_data();    //any new since last readout?
  bool empty() const; 

  //report on contents
  std::vector<std::string> types() const;
  std::set<uint32_t>      resolutions(uint16_t dim) const;
  std::string identity() const {
    boost::unique_lock<boost::mutex> lock(mutex_); return identity_;
  }

  bool changed();

  std::set<Spill> spills() const {
    boost::unique_lock<boost::mutex> lock(mutex_); return spills_;
  }
  
  //get spectra -- may not be thread-safe
  std::shared_ptr<Sink> by_name(std::string name);
  std::list<std::shared_ptr<Sink>> spectra(int32_t dim = -1, int32_t res = -1);
  std::list<std::shared_ptr<Sink>> by_type(std::string reqType);


 private:

  //control
  mutable boost::mutex mutex_;
  boost::condition_variable cond_;
  bool ready_, terminating_, newdata_;

  //data
  std::list<std::shared_ptr<Sink>> my_spectra_;
  std::set<Spill> spills_;

  std::string identity_;
  bool        changed_;

  //helpers
  void clear_helper();
  void write_xml(std::string file_name);

};

}
#endif
