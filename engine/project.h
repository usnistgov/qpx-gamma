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

#ifndef QPX_PROJECT_H
#define QPX_PROJECT_H

#include "daq_sink.h"

namespace Qpx {

class Project {
 public:
  Project()
    : ready_(false), newdata_(false), terminating_(false), changed_(false)
    , identity_("New project")
    , current_index_(0)
  {}

  ~Project();

  ////control//////
  void clear();
  void activate();    //force release of cond var
  void terminate();   //explicitly in case other threads waiting for cond var

  //populate one of these ways
  void set_prototypes(const XMLableDB<Metadata>&);
  int64_t add_sink(SinkPtr sink);
  int64_t add_sink(Metadata prototype);

  void import_spn(std::string file_name);

  void save();
  void save_as(std::string file_name);

  void read_xml(std::string file_name, bool with_sinks = true, bool with_full_sinks = true);

  void delete_sink(int64_t idx);

  //acquisition feeds events to all sinks
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
  
  //get sinks
  SinkPtr get_sink(int64_t idx);
  std::map<int64_t, SinkPtr> get_sinks(int32_t dimensions = -1, int32_t bits = -1);
  std::map<int64_t, SinkPtr> get_sinks(std::string type);


 private:

  //control
  mutable boost::mutex mutex_;
  boost::condition_variable cond_;
  bool ready_, terminating_, newdata_;
  int64_t current_index_;

  //data
  std::map<int64_t, SinkPtr> sinks_;
  std::set<Spill> spills_;

  std::string identity_;
  bool        changed_;

  //helpers
  void clear_helper();
  void write_xml(std::string file_name);

};

}
#endif
