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
 *      Qpx::SpectraSet container class for managing simultaneous
 *      acquisition and output of spectra. Thread-safe, contingent
 *      upon stored spectra types being thread-safe.
 *
 ******************************************************************************/

#include <boost/date_time.hpp>
#include <boost/date_time/date_facet.hpp>
#include <set>
#include "spectra_set.h"
#include "custom_logger.h"

namespace Qpx {

SpectraSet::~SpectraSet() {
  terminate();
  clear_helper();
}

void SpectraSet::terminate() {
  ready_ = true;
  terminating_ = true;
  cond_.notify_one();
}

void SpectraSet::clear() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();
  cond_.notify_one();
}

void SpectraSet::clear_helper() {
  //private, no lock needed
  if (!my_spectra_.empty())
    for (auto &q: my_spectra_) {
      if (q != nullptr)
        delete q;
    }
  my_spectra_.clear();
  status_ = "empty";
  run_info_ = RunInfo();
}

void SpectraSet::closeAcquisition() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (!my_spectra_.empty())
    for (auto &q: my_spectra_) {
      //PL_DBG << "closing " << q->name();
      q->closeAcquisition();
    }
}

void SpectraSet::activate() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  ready_ = true;  
  cond_.notify_one();
}

bool SpectraSet::wait_ready() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_)
    cond_.wait(lock);
  ready_ = false;
  return (!terminating_);
}


bool SpectraSet::new_data() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  bool ret = newdata_;
  newdata_ = false;
  return ret;  
}

void SpectraSet::setRunInfo(const RunInfo &ri) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  run_info_ = ri;
  //notify?
}


bool SpectraSet::empty() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return my_spectra_.empty();  
}

std::vector<std::string> SpectraSet::types() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::set<std::string> my_types;
  for (auto &q: my_spectra_)
    my_types.insert(q->type());
  std::vector<std::string> output(my_types.begin(), my_types.end());
  return output;
}

std::set<uint32_t> SpectraSet::resolutions(uint16_t dim) const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::set<uint32_t> haveres;
  for (auto &q: my_spectra_)
    if (q->dimensions() == dim)
      haveres.insert(q->bits());
  return haveres;
}

Spectrum::Spectrum* SpectraSet::by_name(std::string name) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe

  if (!my_spectra_.empty())
    for (auto &q: my_spectra_)
      if (q->name() == name)
        return q;
  return nullptr;
}

std::list<Spectrum::Spectrum*> SpectraSet::spectra(int32_t dim, int32_t res) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe
  
  if ((dim == -1) && (res == -1))
    return my_spectra_;

  std::list<Spectrum::Spectrum*> results;
  for (auto &q: my_spectra_)
    if (((q->dimensions() == dim) && (q->bits() == res)) ||
        ((q->dimensions() == dim) && (res == -1)) ||
        ((dim == -1) && (q->bits() == res)))
      results.push_back(q);
  return results;
}

std::list<Spectrum::Spectrum*> SpectraSet::by_type(std::string reqType) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe
  
  std::list<Spectrum::Spectrum*> results;
  for (auto &q: my_spectra_)
    if (q->type() == reqType)
      results.push_back(q);
  return results;
}



//client should activate replot after loading all files, as loading multiple
// spectra might create a long queue of plot update signals
void SpectraSet::add_spectrum(Spectrum::Spectrum* newSpectrum) {
  if (newSpectrum == nullptr)
    return;
  boost::unique_lock<boost::mutex> lock(mutex_);  
  my_spectra_.push_back(newSpectrum);
  status_ = "imported from file(s)";
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  // cond_.notify_one();
}

void SpectraSet::delete_spectrum(std::string name) {
  std::list<Spectrum::Spectrum*>::iterator it = my_spectra_.begin();

  while (it != my_spectra_.end()) {
    if ((*it)->name() == name) {
      my_spectra_.erase(it);
      return;
    }
    it++;
  }
}

void SpectraSet::set_spectra(const XMLableDB<Spectrum::Template>& newdb) {
  boost::unique_lock<boost::mutex> lock(mutex_);  
  clear_helper();
  int numofspectra = newdb.size();

  for (int i=0; i < numofspectra; i++) {
    Spectrum::Spectrum* newSpectrum = Spectrum::Factory::getInstance().create_from_template(newdb.get(i));
    if (newSpectrum != nullptr) {
      my_spectra_.push_back(newSpectrum);
    }
  }

  ready_ = true; terminating_ = false; newdata_ = false;
  cond_.notify_one();
}

void SpectraSet::add_spill(Spill* one_spill) {
  boost::unique_lock<boost::mutex> lock(mutex_);  

  for (auto &q: my_spectra_)
    q->addSpill(*one_spill);

  if (!one_spill->stats.empty())
      status_ = "Live at " +  boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) +
          " with " + std::to_string(one_spill->stats.front().event_rate) + " events/sec";

  if (!one_spill->run.time_start.is_not_a_date_time()) {
    run_info_ = one_spill->run;
    if (one_spill->run.total_events > 0) {
      status_ = "Complete at " + boost::posix_time::to_simple_string(one_spill->run.time_stop);
//      PL_INFO << "**Spectra building complete";
    }
  }
  
  ready_ = true;
  newdata_ = true;
  cond_.notify_one();
}


void SpectraSet::write_xml(std::string file_name) {
  boost::unique_lock<boost::mutex> lock(mutex_);

  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child();
  root.set_name("QpxAcquisition");

  run_info_.state.strip_metadata();
  run_info_.to_xml(root, true);

  pugi::xml_node spectranode = root.append_child("Spectra");

  for (auto &q : my_spectra_)
    q->to_xml(spectranode);

  doc.save_file(file_name.c_str());

  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();

}

void SpectraSet::read_xml(std::string file_name, bool with_spectra, bool with_full_spectra) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();

  pugi::xml_document doc;

  if (!doc.load_file(file_name.c_str()))
    return;

  pugi::xml_node root = doc.child("QpxAcquisition");
  if (!root)
    return;

  run_info_.from_xml(root.child(run_info_.xml_element_name().c_str()));

  if (!with_spectra)
    return;

  if (root.child("Spectra")) {

    Spill fake_spill;
    fake_spill.run = run_info_;

    for (pugi::xml_node &child : root.child("Spectra").children()) {
      if (child.child("ChannelData") && !with_full_spectra)
        child.remove_child("ChannelData");

      Spectrum::Spectrum* new_spectrum
          = Spectrum::Factory::getInstance().create_from_xml(child);
      if (new_spectrum == nullptr)
        PL_INFO << "Could not parse spectrum";
      else {
        new_spectrum->addSpill(fake_spill);
        my_spectra_.push_back(new_spectrum);
      }
    }
  }

  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

void SpectraSet::read_spn(std::string file_name) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();

  std::ifstream myfile(file_name, std::ios::in | std::ios::binary);

  if (!myfile)
    return;

  myfile.seekg (0, myfile.end);
  int length = myfile.tellg();

//  if (length < 13)
//    return;

  myfile.seekg (0, myfile.beg);


  Gamma::Detector det("unknown");
  det.energy_calibrations_.add(Gamma::Calibration("Energy", 12));

  Qpx::Spill spill;
  spill.run.detectors.push_back(det);

  Qpx::Spectrum::Template *temp = Qpx::Spectrum::Factory::getInstance().create_template("1D");
  temp->visible = false;
  temp->bits = 12;
  temp->match_pattern = std::vector<int16_t>({1});
  temp->add_pattern = std::vector<int16_t>({1});

  uint32_t one;
  int spectra_count = 0;
  while (myfile.tellg() != length) {
  //for (int j=0; j<150; ++j) {
    spectra_count++;
    std::vector<uint32_t> data;
    uint64_t totalcount = 0;
//    uint32_t header;
//    myfile.read ((char*)&header, sizeof(uint32_t));
//    PL_DBG << "header " << header;
    for (int i=0; i<4096; ++i) {
      myfile.read ((char*)&one, sizeof(uint32_t));
      data.push_back(one);
      totalcount += one;
    }
    if ((totalcount == 0) || data.empty())
      continue;
    temp->name_ = boost::filesystem::path(file_name).filename().string() + "[" + std::to_string(spectra_count) + "]";
    Qpx::Spectrum::Spectrum *spectrum = Qpx::Spectrum::Factory::getInstance().create_from_template(*temp);
    for (int i=0; i < data.size(); ++i) {
      Spectrum::Entry entry;
      entry.first.resize(1, 0);
      entry.first[0] = i;
      entry.second = data[i];
      spectrum->add_bulk(entry);
    }
    spectrum->addSpill(spill);
    my_spectra_.push_back(spectrum);
  }

  delete temp;

  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

}
