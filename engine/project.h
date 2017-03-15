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

#pragma once

#include "daq_sink.h"
#include "fitter.h"

#ifdef H5_ENABLED
#include "H5CC_Group.h"
#endif

namespace Qpx {

class Project {
private:
  //control
  mutable boost::mutex mutex_;
  boost::condition_variable cond_;
  mutable bool ready_ {false};
  mutable bool newdata_ {false};
  int64_t current_index_ {0};

  //data
  std::map<int64_t, SinkPtr> sinks_;
  std::map<int64_t, Fitter> fitters_1d_;
  std::set<Spill> spills_;

  //saveability
  std::string   identity_ {"New project"};
  mutable bool  changed_  {false};

public:
  Project() {}
  Project(const Qpx::Project&);

  ////control//////
  void clear();
  void activate();    //force release of cond var

  //populate one of these ways
  void set_prototypes(const XMLableDB<Metadata>&);
  int64_t add_sink(SinkPtr sink);
  int64_t add_sink(Metadata prototype);

  void import_spn(std::string file_name);

  void save();
  void save_as(std::string file_name);

  void open(std::string file_name, bool with_sinks = true, bool with_full_sinks = true);

  void delete_sink(int64_t idx);

  //acquisition feeds events to all sinks
  void add_spill(Spill* one_spill);
  void flush();

  //status inquiry
  bool wait_ready();  //wait for cond variable
  bool new_data();    //any new since last readout?
  bool empty() const;

  //report on contents
  std::vector<std::string> types() const;
  std::string identity() const;

  bool changed() const;
  void mark_changed();

  std::set<Spill> spills() const;
  
  //get sinks
  SinkPtr get_sink(int64_t idx);
  std::map<int64_t, SinkPtr> get_sinks(int32_t dimensions = -1);
  std::map<int64_t, SinkPtr> get_sinks(std::string type);

  //Fitters
  bool has_fitter(int64_t) const;
  Fitter get_fitter(int64_t) const;
  void update_fitter(int64_t idx, const Fitter &fit);

  std::string xml_element_name() const {return "QpxProject";}
  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node, bool with_sinks = true, bool with_full_sinks = true);

  #ifdef H5_ENABLED
  void to_h5(H5CC::Group &group) const;
  void from_h5(H5CC::Group &group, bool with_sinks, bool with_full_sinks);
  #endif

private:
  //helpers
  void clear_helper();
  void write_xml(std::string file_name);
  void read_xml(std::string file_name, bool with_sinks = true, bool with_full_sinks = true);

  #ifdef H5_ENABLED
  void write_h5(std::string file_name);
  void read_h5(std::string file_name, bool with_sinks = true, bool with_full_sinks = true);
  #endif

};

typedef std::shared_ptr<Project> ProjectPtr;

}
