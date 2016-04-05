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

#include "project.h"
#include "daq_sink_factory.h"
#include "custom_logger.h"
#include "qpx_util.h"

namespace Qpx {


//DEPRECATE!!!
struct RunInfo {
  Qpx::Setting state;
  std::vector<Qpx::Detector> detectors;
  boost::posix_time::ptime time;

  inline RunInfo()
  {}

  void from_xml(const pugi::xml_node &node) {
    if (node.attribute("time"))
      time = from_iso_extended(node.attribute("time").value());
    else if (node.attribute("time_start"))
      time = from_iso_extended(node.attribute("time_start").value());

    if (node.child(state.xml_element_name().c_str()))
      state.from_xml(node.child(state.xml_element_name().c_str()));

    if (node.child("Detectors")) {
      detectors.clear();
      for (auto &q : node.child("Detectors").children()) {
        Qpx::Detector det;
        det.from_xml(q);
        detectors.push_back(det);
      }
    }
  }
};




Project::~Project() {
  terminate();
  clear_helper();
}

void Project::terminate() {
  ready_ = true;
  terminating_ = true;
  cond_.notify_one();
}

void Project::clear() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();
  cond_.notify_one();
}

void Project::clear_helper() {
  //private, no lock needed
  if (!my_spectra_.empty() || !spills_.empty())
    changed_ = true;

  my_spectra_.clear();
  spills_.clear();
}

void Project::flush() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (!my_spectra_.empty())
    for (auto &q: my_spectra_) {
      //PL_DBG << "closing " << q->name();
      q->flush();
    }
}

void Project::activate() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  ready_ = true;  
  cond_.notify_one();
}

bool Project::wait_ready() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_)
    cond_.wait(lock);
  ready_ = false;
  return (!terminating_);
}


bool Project::new_data() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  bool ret = newdata_;
  newdata_ = false;
  return ret;  
}

bool Project::changed() {
  boost::unique_lock<boost::mutex> lock(mutex_);

  for (auto &q : my_spectra_)
    if (q->metadata().changed)
      changed_ = true;

  return changed_;
}


bool Project::empty() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return my_spectra_.empty();  
}

std::vector<std::string> Project::types() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::set<std::string> my_types;
  for (auto &q: my_spectra_)
    my_types.insert(q->type());
  std::vector<std::string> output(my_types.begin(), my_types.end());
  return output;
}

std::set<uint32_t> Project::resolutions(uint16_t dim) const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::set<uint32_t> haveres;
  for (auto &q: my_spectra_)
    if (q->dimensions() == dim)
      haveres.insert(q->bits());
  return haveres;
}

std::shared_ptr<Sink> Project::by_name(std::string name) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe

  if (!my_spectra_.empty())
    for (auto &q: my_spectra_)
      if (q->name() == name)
        return q;
  return nullptr;
}

std::list<std::shared_ptr<Sink>> Project::spectra(int32_t dim, int32_t res) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe
  
  if ((dim == -1) && (res == -1))
    return my_spectra_;

  std::list<std::shared_ptr<Sink>> results;
  for (auto &q: my_spectra_)
    if (((q->dimensions() == dim) && (q->bits() == res)) ||
        ((q->dimensions() == dim) && (res == -1)) ||
        ((dim == -1) && (q->bits() == res)))
      results.push_back(q);
  return results;
}

std::list<std::shared_ptr<Sink>> Project::by_type(std::string reqType) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as Spectrum implemented as thread-safe
  
  std::list<std::shared_ptr<Sink>> results;
  for (auto &q: my_spectra_)
    if (q->type() == reqType)
      results.push_back(q);
  return results;
}



//client should activate replot after loading all files, as loading multiple
// spectra might create a long queue of plot update signals
void Project::add_spectrum(std::shared_ptr<Sink> newSpectrum) {
  if (newSpectrum == nullptr)
    return;
  boost::unique_lock<boost::mutex> lock(mutex_);  
  my_spectra_.push_back(newSpectrum);
  changed_ = true;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  // cond_.notify_one();
}

void Project::add_spectrum(Metadata spectrum) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::shared_ptr<Sink> newSpectrum = Qpx::SinkFactory::getInstance().create_from_prototype(spectrum);
  if (newSpectrum != nullptr)
    my_spectra_.push_back(newSpectrum);
  changed_ = true;
  ready_ = true;
  newdata_ = false;
  terminating_ = false;
  // cond_.notify_one();
}

void Project::delete_spectrum(std::string name) {
  std::list<std::shared_ptr<Sink>>::iterator it = my_spectra_.begin();

  while (it != my_spectra_.end()) {
    if ((*it)->name() == name) {
      my_spectra_.erase(it);
      changed_ = true;
      return;
    }
    it++;
  }
}

void Project::set_spectra(const XMLableDB<Metadata>& newdb) {
  boost::unique_lock<boost::mutex> lock(mutex_);  
  clear_helper();
  int numofspectra = newdb.size();

  for (int i=0; i < numofspectra; i++) {
    std::shared_ptr<Sink> newSpectrum = Qpx::SinkFactory::getInstance().create_from_prototype(newdb.get(i));
    if (newSpectrum != nullptr) {
      my_spectra_.push_back(newSpectrum);
    }
  }

  changed_ = true;

  ready_ = true; terminating_ = false; newdata_ = false;
  cond_.notify_one();
}

void Project::add_spill(Spill* one_spill) {
  boost::unique_lock<boost::mutex> lock(mutex_);  

  for (auto &q: my_spectra_)
    q->push_spill(*one_spill);

  if (!one_spill->detectors.empty()
      || !one_spill->state.branches.empty())
    spills_.insert(*one_spill);

  if ((!one_spill->stats.empty())
       || (!one_spill->hits.empty())
       || (!one_spill->data.empty())
       || (!one_spill->state.branches.empty())
       || (!one_spill->detectors.empty()))
      changed_ = true;

  
  ready_ = true;
  newdata_ = true;
  cond_.notify_one();
}


void Project::save() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (/*changed_ && */(identity_ != "New project"))
    write_xml(identity_);
}


void Project::save_as(std::string file_name) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  write_xml(file_name);
}

void Project::write_xml(std::string file_name) {

  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child();
  root.set_name("QpxProject");

  if (!spills_.empty()) {
    pugi::xml_node spillsnode = root.append_child("Spills");
    for (auto &s :spills_)
      s.to_xml(spillsnode, true);
  }

  if (!my_spectra_.empty()) {
    pugi::xml_node spectranode = root.append_child("Spectra");
    for (auto &q : my_spectra_)
      q->to_xml(spectranode);
  }

  doc.save_file(file_name.c_str());

  for (auto &q : my_spectra_)
    q->reset_changed();

  changed_ = false;
  identity_ = file_name;

  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();

}

void Project::read_xml(std::string file_name, bool with_spectra, bool with_full_spectra) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();

  pugi::xml_document doc;

  if (!doc.load_file(file_name.c_str()))
    return;

  pugi::xml_node root;
  if (doc.child("QpxProject"))
   root = doc.child("QpxProject");
  else if (doc.child("QpxAcquisition"))  //backwards compat
   root = doc.child("QpxAcquisition");
  if (!root)
    return;

  if (root.child("Spills")) {
    for (auto &s : root.child("Spills").children()) {
      Spill sp;
      sp.from_xml(s);
      if (!sp.empty())
        spills_.insert(sp);
    }
  }

  //backwards compat
  RunInfo ri;
  if (root.child("RunInfo")) {
    ri.from_xml(root.child("RunInfo"));
    Spill sp;
    if (!ri.time.is_not_a_date_time()) {
      sp.time = ri.time;
      sp.detectors = ri.detectors;
      sp.state = ri.state;
      if (sp.detectors.empty())
        sp.detectors.resize(16, Detector("unknown"));
      spills_.insert(sp);
    }
  } else if (root.child("Run")) {
    ri.from_xml(root.child("Run"));
    Spill sp;
    if (!ri.time.is_not_a_date_time()) {
      sp.time = ri.time;
      sp.detectors = ri.detectors;
      sp.state = ri.state;
      if (sp.detectors.empty())
        sp.detectors.resize(16, Detector("unknown"));
      spills_.insert(sp);
    }
  }

  if (spills_.empty()) { //backwrds compat
    Spill sp;
    sp.detectors.resize(16, Detector("unknown"));
    spills_.insert(sp);
  }

  if (!with_spectra)
    return;

  if (root.child("Spectra")) {

    for (pugi::xml_node &child : root.child("Spectra").children()) {
      if (child.child("ChannelData") && !with_full_spectra)
        child.remove_child("ChannelData");

      std::shared_ptr<Sink> new_spectrum
          = Qpx::SinkFactory::getInstance().create_from_xml(child);
      if (!new_spectrum)
        PL_INFO << "Could not parse spectrum";
      else {
        for (auto &s : spills_) {
          Spill sp = s;
          if (!new_spectrum->metadata().detectors.empty()) //backwards compat
            sp.detectors.clear();
          new_spectrum->push_spill(sp);
        }
        my_spectra_.push_back(new_spectrum);
      }
    }
  }

  changed_ = false;
  identity_ = file_name;

  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

void Project::import_spn(std::string file_name) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //clear_helper();

  std::ifstream myfile(file_name, std::ios::in | std::ios::binary);

  if (!myfile)
    return;

  myfile.seekg (0, myfile.end);
  int length = myfile.tellg();

//  if (length < 13)
//    return;

  myfile.seekg (0, myfile.beg);


  Qpx::Detector det("unknown");
  det.energy_calibrations_.add(Qpx::Calibration("Energy", 12));

  std::vector<Detector> dets;
  dets.push_back(det);

  Qpx::Metadata temp = Qpx::SinkFactory::getInstance().create_prototype("1D");
  temp.bits = 12;
  Qpx::Setting pattern;
  pattern = temp.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(std::vector<bool>({1}));
  pattern.value_pattern.set_theshold(1);
  temp.attributes.branches.replace(pattern);
  pattern = temp.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(std::vector<bool>({1}));
  pattern.value_pattern.set_theshold(1);
  temp.attributes.branches.replace(pattern);

  uint32_t one;
  int spectra_count = 0;
  while (myfile.tellg() != length) {
  //for (int j=0; j<150; ++j) {
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
    temp.name = boost::filesystem::path(file_name).filename().string() + "[" + std::to_string(spectra_count) + "]";
    std::shared_ptr<Sink> spectrum = Qpx::SinkFactory::getInstance().create_from_prototype(temp);
    spectrum->set_detectors(dets);
    for (int i=0; i < data.size(); ++i) {
      Entry entry;
      entry.first.resize(1, 0);
      entry.first[0] = i;
      entry.second = data[i];
      spectrum->append(entry);
    }
    my_spectra_.push_back(spectrum);
    spectra_count++;
  }

  changed_ = true;
//  identity_ = file_name;

  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

}
