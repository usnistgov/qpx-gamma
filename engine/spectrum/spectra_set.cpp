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
#include "qpx_settings.h"

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
      PL_DBG << "closing " << q->name();
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

void SpectraSet::set_spectra(const XMLable2DB<Spectrum::Template>& newdb) {
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

  if ((!one_spill->stats.empty()) && (one_spill->stats.front().spill_number > 0))
      status_ = "Live at " +  boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) +
          " with " + std::to_string(one_spill->stats.front().event_rate) + " events/sec";

  if (one_spill->run != nullptr) {
    run_info_ = *one_spill->run;
    if (one_spill->run->total_events > 0) {
      status_ = "Complete at " + boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time());
      PL_INFO << "**Spectra building complete";
    }
  }
  
  ready_ = true;
  newdata_ = true;
  cond_.notify_one();
}

void SpectraSet::write_xml(std::string file_name) {
  boost::unique_lock<boost::mutex> lock(mutex_);

  FILE* myfile;
  myfile = fopen (file_name.c_str(), "w");
  if (myfile == nullptr)
    return;

  tinyxml2::XMLPrinter printer(myfile);
  printer.PushHeader(true, true);
  
  printer.OpenElement("QpxAcquisition");
  
  printer.OpenElement("Run");
  printer.PushAttribute("time_start",                       boost::posix_time::to_iso_extended_string(run_info_.time_start).c_str());
  printer.PushAttribute("time_stop",                       boost::posix_time::to_iso_extended_string(run_info_.time_stop).c_str());
  printer.PushAttribute("total_events", std::to_string(run_info_.total_events).c_str());
  printer.CloseElement();  

  run_info_.state.to_xml(printer);

  printer.OpenElement("Spectra");
  for (auto &q : my_spectra_)
    q->to_xml(printer);
  printer.CloseElement();

  printer.CloseElement(); //QpxAcquisition

  fclose(myfile);

  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();

}

void SpectraSet::write_xml2(std::string file_name) {
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

void SpectraSet::read_xml(std::string file_name, bool with_spectra, Gamma::Setting tree) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  clear_helper();
  
  FILE* myfile;
  myfile = fopen (file_name.c_str(), "r");
  if (myfile == nullptr)
    return;
  
  tinyxml2::XMLDocument docx;
  docx.LoadFile(myfile);

  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  tinyxml2::XMLElement* root = docx.FirstChildElement("QpxAcquisition");
  if (root == nullptr) return;

  tinyxml2::XMLElement* elem;

  if ((elem = root->FirstChildElement("Run")) != nullptr) {
    if (elem->Attribute("time_start")) {
      iss << elem->Attribute("time_start");
      iss.imbue(std::locale(std::locale::classic(), tif));
      iss >> run_info_.time_start;
    }
    if (elem->Attribute("time_stop")) {
      iss << elem->Attribute("time_stop");
      iss.imbue(std::locale(std::locale::classic(), tif));
      iss >> run_info_.time_stop;
    }
    if (elem->Attribute("total_events"))
      run_info_.total_events = boost::lexical_cast<uint64_t>(elem->Attribute("total_events"));
  }

  if ((elem = root->FirstChildElement("PixieSettings")) != nullptr) { //backwards compatibility
    PL_DBG << "Loading PixieSettings";
    Qpx::Settings sets(elem, tree);
    sets.save_optimization();
    run_info_.state = sets.pull_settings();
    run_info_.detectors = sets.get_detectors();
  } else if ((elem = root->FirstChildElement("Setting")) != nullptr) {
    run_info_.state = Gamma::Setting(elem);
    //run_info_.state.save_optimization();
  }

  if (!with_spectra) {
    fclose(myfile);
    return;
  }
  
  if ((elem = root->FirstChildElement("Spectra")) != nullptr) {
    Spill fake_spill;
    fake_spill.run = new RunInfo(run_info_);

    tinyxml2::XMLElement* one_spectrum = elem->FirstChildElement("PixieSpectrum");
    while (one_spectrum != nullptr) {
      Spectrum::Spectrum* new_spectrum
          = Spectrum::Factory::getInstance().create_from_xml(one_spectrum);
      if (new_spectrum == nullptr)
        PL_INFO << "Could not parse spectrum";
      else {
        new_spectrum->addSpill(fake_spill);
        my_spectra_.push_back(new_spectrum);
      }
      one_spectrum = dynamic_cast<tinyxml2::XMLElement*>(one_spectrum->NextSibling());
    }
    delete fake_spill.run;
  }
  
  fclose(myfile);
  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

void SpectraSet::read_xml2(std::string file_name, bool with_spectra) {
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
    fake_spill.run = new RunInfo(run_info_);

    for (pugi::xml_node &child : root.child("Spectra").children()) {
      PL_DBG << "child name " << child.name();
      Spectrum::Spectrum* new_spectrum
          = Spectrum::Factory::getInstance().create_from_xml(child);
      if (new_spectrum == nullptr)
        PL_INFO << "Could not parse spectrum";
      else {
        new_spectrum->addSpill(fake_spill);
        my_spectra_.push_back(new_spectrum);
        PL_DBG << "loaded " << new_spectrum->name() << " with dets " << new_spectrum->metadata().detectors.size() << " and total "
               << new_spectrum->metadata().total_count;
      }
    }
    delete fake_spill.run;
  }

  status_ = file_name;
  ready_ = true;
  newdata_ = true;
  terminating_ = false;
  cond_.notify_one();
}

}
