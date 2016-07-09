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

Project::Project(const Qpx::Project& other)
  : Project()
{
  ready_ = true;
  newdata_ = true;
  changed_ = true;
  identity_ = other.identity_;
  current_index_= other.current_index_;
  sinks_ = other.sinks_;
  fitters_1d_ = other.fitters_1d_;
  spills_ = other.spills_;
  for (auto sink : other.sinks_)
    sinks_[sink.first] = SinkFactory::getInstance().create_copy(sink.second);
  DBG << "<Qpx::Project> deep copy performed";
}


std::string Project::identity() const
{
  boost::unique_lock<boost::mutex> lock(mutex_); return identity_;
}

std::set<Spill> Project::spills() const
{
  boost::unique_lock<boost::mutex> lock(mutex_); return spills_;
}

//Project Project::deep_copy() const
//{
//  Project ret;
//  ret.current_index_ = current_index_;
//  ret.spills_ = spills_;
//  ret.fitters_1d_ = fitters_1d_;
//  for (auto sink : sinks_)
//    ret.sinks_[sink.first] = SinkFactory::getInstance().create_copy(sink.second);
//}

void Project::clear() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();
  cond_.notify_all();
}

void Project::clear_helper() {
  //private, no lock needed
  if (!sinks_.empty() || !spills_.empty())
    changed_ = true;

  sinks_.clear();
  spills_.clear();
  fitters_1d_.clear();
  current_index_ = 0;
}

void Project::flush() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (!sinks_.empty())
    for (auto &q: sinks_) {
      //DBG << "closing " << q->name();
      q.second->flush();
    }
}

void Project::activate() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  ready_ = true;
  cond_.notify_all();
}

bool Project::wait_ready() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  while (!ready_)
    cond_.wait(lock);
  ready_ = false;
  return true;
}


bool Project::new_data() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  bool ret = newdata_;
  newdata_ = false;
  return ret;
}

bool Project::changed() const {
  boost::unique_lock<boost::mutex> lock(mutex_);

  for (auto &q : sinks_)
    if (q.second->changed())
      changed_ = true;

  return changed_;
}

void Project::mark_changed() {
  boost::unique_lock<boost::mutex> lock(mutex_);
  changed_ = true;
}

bool Project::empty() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  return sinks_.empty();
}

std::vector<std::string> Project::types() const {
  boost::unique_lock<boost::mutex> lock(mutex_);
  std::set<std::string> my_types;
  for (auto &q: sinks_)
    my_types.insert(q.second->type());
  std::vector<std::string> output(my_types.begin(), my_types.end());
  return output;
}

//std::set<uint32_t> Project::resolutions(uint16_t dim) const {
//  boost::unique_lock<boost::mutex> lock(mutex_);
//  std::set<uint32_t> haveres;
//  for (auto &q: sinks_)
//    if (q.second->dimensions() == dim)
//      haveres.insert(q.second->bits());
//  return haveres;
//}

bool Project::has_fitter(int64_t idx) const
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  return (fitters_1d_.count(idx) > 0);
}

Fitter Project::get_fitter(int64_t idx) const
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (fitters_1d_.count(idx) > 0)
    return fitters_1d_.at(idx);
  else
    return Fitter();
}

void Project::update_fitter(int64_t idx, const Fitter &fit)
{
  boost::unique_lock<boost::mutex> lock(mutex_);
  if (!sinks_.count(idx))
    return;
  fitters_1d_[idx] = fit;
  changed_ = true;
}


SinkPtr Project::get_sink(int64_t idx) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as sink implemented as thread-safe

  if (sinks_.count(idx))
    return sinks_.at(idx);
  else
    return nullptr;
}

std::map<int64_t, SinkPtr> Project::get_sinks(int32_t dimensions) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as sink implemented as thread-safe
  
  if (dimensions == -1)
    return sinks_;

  std::map<int64_t, SinkPtr> ret;
  for (auto &q: sinks_)
    if (q.second->dimensions() == dimensions)
      ret.insert(q);
  return ret;
}

std::map<int64_t, SinkPtr> Project::get_sinks(std::string type) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  //threadsafe so long as sink implemented as thread-safe
  
  std::map<int64_t, SinkPtr> ret;
  for (auto &q: sinks_)
    if (q.second->type() == type)
      ret.insert(q);
  return ret;
}



//client should activate replot after loading all files, as loading multiple
// sink might create a long queue of plot update signals
int64_t Project::add_sink(SinkPtr sink) {
  if (!sink)
    return 0;
  boost::unique_lock<boost::mutex> lock(mutex_);
  sinks_[++current_index_] = sink;
  changed_ = true;
  ready_ = true;
  newdata_ = true;
  // cond_.notify_one();
  return current_index_;
}

int64_t Project::add_sink(Metadata prototype) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  SinkPtr sink = Qpx::SinkFactory::getInstance().create_from_prototype(prototype);
  if (!sink)
    return 0;
  sinks_[++current_index_] = sink;
  changed_ = true;
  ready_ = true;
  newdata_ = false;
  // cond_.notify_one();
  return current_index_;
}

void Project::delete_sink(int64_t idx) {
  boost::unique_lock<boost::mutex> lock(mutex_);

  if (!sinks_.count(idx))
    return;

  sinks_.erase(idx);
  changed_ = true;
  ready_ = true;
  newdata_ = false;
  // cond_.notify_one();

  if (sinks_.empty())
    current_index_ = 0;
}

void Project::set_prototypes(const XMLableDB<Metadata>& prototypes) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();

  for (int i=0; i < prototypes.size(); i++) {
    SinkPtr sink = Qpx::SinkFactory::getInstance().create_from_prototype(prototypes.get(i));
    if (sink)
      sinks_[++current_index_] = sink;
  }

  changed_ = true;
  ready_ = true;
  newdata_ = false;
  cond_.notify_all();
}

void Project::add_spill(Spill* one_spill) {
  boost::unique_lock<boost::mutex> lock(mutex_);

  for (auto &q: sinks_)
    q.second->push_spill(*one_spill);

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
  cond_.notify_all();
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

  to_xml(root);

  doc.save_file(file_name.c_str());

  for (auto &q : sinks_)
    q.second->reset_changed();

  identity_ = file_name;
  cond_.notify_all();
}

void Project::to_xml(pugi::xml_node &root) const {
  root.set_name(this->xml_element_name().c_str());

  if (!spills_.empty()) {
    pugi::xml_node spillsnode = root.append_child("Spills");
    for (auto &s :spills_)
      s.to_xml(spillsnode, true);
  }

  if (!sinks_.empty()) {
    pugi::xml_node sinks_node = root.append_child("Sinks");
    for (auto &q : sinks_) {
      q.second->to_xml(sinks_node);
      sinks_node.last_child().append_attribute("idx").set_value(std::to_string(q.first).c_str());
    }
  }

  if (fitters_1d_.size())
  {
    //    DBG << "Will save fitters";
    pugi::xml_node fits_node = root.append_child("Fits1D");
    for (auto &q : fitters_1d_) {
      //      DBG << "saving fit " << q.first;
      q.second.to_xml(fits_node);
      fits_node.last_child().append_attribute("idx").set_value(std::to_string(q.first).c_str());
    }
  }

  changed_ = false;
  ready_ = true;
  newdata_ = true;
}

void Project::read_xml(std::string file_name, bool with_sinks, bool with_full_sinks) {
  pugi::xml_document doc;

  if (!doc.load_file(file_name.c_str()))
    return;

  pugi::xml_node root;
  if (doc.child(this->xml_element_name().c_str()))
    root = doc.child(this->xml_element_name().c_str());
  else if (doc.child("QpxAcquisition"))  //backwards compat
    root = doc.child("QpxAcquisition");
  if (!root)
    return;

  from_xml(root);

  identity_ = file_name;
  cond_.notify_all();
}


void Project::from_xml(const pugi::xml_node &root, bool with_sinks, bool with_full_sinks) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();


  if (root.child("Spills")) {
    for (auto &s : root.child("Spills").children()) {
      Spill sp;
      sp.from_xml(s);
      if (!sp.empty())
        spills_.insert(sp);
    }
  }


  if (!with_sinks)
    return;

  pugi::xml_node sinksnode;

  if (root.child("Spectra"))
    sinksnode = root.child("Spectra"); //backcompat
  else if (root.child("Sinks"))
    sinksnode = root.child("Sinks");   //current, ok

  for (pugi::xml_node &child : sinksnode.children()) {
    if (child.child("Data") && !with_full_sinks)
      child.remove_child("Data");

    if (child.attribute("idx"))
      current_index_ = child.attribute("idx").as_llong();
    else
      current_index_++; //backwards compat

    SinkPtr sink
        = Qpx::SinkFactory::getInstance().create_from_xml(child);
    if (!sink)
      LINFO << "Could not parse spectrum";
    else {
      for (auto &s : spills_) {
        Spill sp = s;
        if (!sink->metadata().detectors.empty()) //backwards compat
          sp.detectors.clear();
        sink->push_spill(sp);
      }
      sinks_[current_index_] = sink;
    }
  }

  current_index_++;

  if (root.child("Fits1D")) {
    for (pugi::xml_node &child : root.child("Fits1D").children()) {
      int64_t idx = child.attribute("idx").as_llong();
      if (!sinks_.count(idx))
        continue;

      fitters_1d_[idx] = Fitter();
      fitters_1d_[idx].from_xml(child, sinks_.at(idx));
    }
  }

  changed_ = false;
  ready_ = true;
  newdata_ = true;
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
  Setting res = temp.get_attribute("resolution");
  res.value_int = 12;
  temp.set_attribute(res);
  Qpx::Setting pattern;
  pattern = temp.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(std::vector<bool>({1}));
  pattern.value_pattern.set_theshold(1);
  temp.set_attribute(pattern);
  pattern = temp.get_attribute("pattern_add");
  pattern.value_pattern.set_gates(std::vector<bool>({1}));
  pattern.value_pattern.set_theshold(1);
  temp.set_attribute(pattern);

  uint32_t one;
  int spectra_count = 0;
  while (myfile.tellg() != length) {
    //for (int j=0; j<150; ++j) {
    std::vector<uint32_t> data;
    int64_t totalcount = 0;
    //    uint32_t header;
    //    myfile.read ((char*)&header, sizeof(uint32_t));
    //    DBG << "header " << header;
    for (int i=0; i<4096; ++i) {
      myfile.read ((char*)&one, sizeof(uint32_t));
      data.push_back(one);
      totalcount += one;
    }
    if ((totalcount == 0) || data.empty())
      continue;
    Qpx::Setting name;
    name = temp.get_attribute("name");
    name = boost::filesystem::path(file_name).filename().string() + "[" + std::to_string(spectra_count++) + "]";
    temp.set_attribute(name);
    SinkPtr spectrum = Qpx::SinkFactory::getInstance().create_from_prototype(temp);
    spectrum->set_detectors(dets);
    for (int i=0; i < data.size(); ++i) {
      Entry entry;
      entry.first.resize(1, 0);
      entry.first[0] = i;
      entry.second = data[i];
      spectrum->append(entry);
    }
    sinks_[++current_index_] = spectrum;
  }

  changed_ = true;
  //  identity_ = file_name;

  ready_ = true;
  newdata_ = true;
  cond_.notify_all();
}

}
