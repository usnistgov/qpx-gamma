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
 *      Qpx::Settings online and offline setting describing the state of
 *      a device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#include "qpx_settings.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common_xia.h"
#include "custom_logger.h"

namespace Qpx {

Settings::Settings() {
  settings_tree_.metadata.setting_type = Gamma::SettingType::stem;
  settings_tree_.id_ = "QpxSettings";
  pixie_plugin_ = new Plugin("/home/mgs/qpxdata/XIA/qpx_plugin.set");
}

Settings::Settings(const Settings& other) :
  settings_tree_(other.settings_tree_),
  pixie_plugin_(other.pixie_plugin_),
  detectors_(other.detectors_)
{}

Settings::Settings(tinyxml2::XMLElement* root, Gamma::Setting tree) :
  Settings()
{
  settings_tree_ = tree;
  from_xml(root);
}

Settings::~Settings() {
  if (pixie_plugin_ != nullptr)
    delete pixie_plugin_;
}

void Settings::to_xml(tinyxml2::XMLPrinter& printer) const {
  settings_tree_.to_xml(printer);
}

void Settings::from_xml(tinyxml2::XMLElement* root) {
  std::string rootname(root->Name());
  if (rootname == "PixieSettings") {
    pixie_plugin_->from_xml_legacy(root, detectors_);
    write_settings_bulk();
    read_settings_bulk();
  } else if (rootname == "Setting") {
    settings_tree_.from_xml(root);
  }
}


Gamma::Setting Settings::pull_settings() const {
  return settings_tree_;
}

void Settings::push_settings(const Gamma::Setting& newsettings) {
  //PL_INFO << "settings pushed";
  settings_tree_ = newsettings;
  write_settings_bulk();
}

bool Settings::read_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_) {
    //PL_INFO << "read bulk "  << set.name;
    if (set.id_ == "Pixie4")
      pixie_plugin_->read_settings_bulk(set);
    else if (set.id_ == "Detectors") {
      set.metadata.step = 2; //to always save
      if (detectors_.size() != set.branches.size()) {
        detectors_.resize(set.branches.size(), Gamma::Detector());
        for (auto &q : set.branches.my_data_) {
          write_detector(q);
          q.metadata.writable = true;
        }
      }
      save_optimization();
    }
  }
  return true;
}

bool Settings::write_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_) {
    //PL_INFO << "write bulk "  << set.name;
    if (set.id_ == "Pixie4")
      pixie_plugin_->write_settings_bulk(set);
  }
  return true;
}

bool Settings::execute_command(){
  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    if ((set.id_ == "Pixie4") && (pixie_plugin_->execute_command(set))) {
      success = true;
    }
  }

  if (success)
    get_all_settings();

  return success;
}

bool Settings::boot() {
  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Boot") {
        if ((set.id_ == "Pixie4") && pixie_plugin_->boot()) {
          success = true;
        }
      }
    }
  }

  if (success) {
    settings_tree_.value_int = 2;
    get_all_settings();
  }

  return success;
}

bool Settings::die() {
  //Must all shutdown?

  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Boot") { //Pixie4/Die
        if ((set.id_ == "Pixie4") && pixie_plugin_->die()) {
          success = true;
        }
      }
    }
  }

  if (success) {
    settings_tree_.value_int = 0;
    get_all_settings();
  }

  return success;
}

std::vector<Trace> Settings::oscilloscope() {
  std::vector<Trace> traces(detectors_.size());

  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Oscilloscope") {
        if (set.id_ == "Pixie4") {
          std::map<int, std::vector<uint16_t>> trc = pixie_plugin_->oscilloscope();
          for (auto &q : trc) {
            if ((q.first >= 0) && (q.first < detectors_.size())) {
              traces[q.first].data = q.second;
              traces[q.first].index = q.first;
              traces[q.first].detector = detectors_[q.first];
            }
          }
        }
      }
    }
  }

  return traces;
}

bool Settings::daq_start(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue) {
  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Acquisition Start") {
        if ((set.id_ == "Pixie4") && pixie_plugin_->daq_start(timeout, out_queue)) {
          success = true;
        }
      }
    }
  }
  return success;
}

bool Settings::daq_stop() {
  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Acquisition Stop") {
        if ((set.id_ == "Pixie4") && pixie_plugin_->daq_stop()) {
          success = true;
        }
      }
    }
  }

  if (success)
    get_all_settings();

  return success;
}

bool Settings::daq_running() {
  bool running = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    for (auto &q : set.branches.my_data_) {
      if (q.id_ == "Pixie4/Acquisition Start") {
        if ((set.id_ == "Pixie4") && pixie_plugin_->daq_running()) {
          running = true;
        }
      }
    }
  }

  return running;
}





bool Settings::write_detector(const Gamma::Setting &set) {
  if (set.metadata.setting_type != Gamma::SettingType::detector)
    return false;

  if ((set.index < 0) || (set.index >= detectors_.size()))
    return false;

  if (detectors_[set.index].name_ != set.value_text)
    detectors_[set.index] = Gamma::Detector(set.value_text);

  return true;
}

void Settings::set_detector(int ch, Gamma::Detector det) {
  if (ch < 0 || ch >= detectors_.size())
    return;
  detectors_[ch] = det;

  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.id_ == "Detectors") {
      if (detectors_.size() == set.branches.size()) {
        for (auto &q : set.branches.my_data_) {
          if (q.index == ch) {
            q.value_text = detectors_[q.index].name_;
            load_optimization(q.index);
          }
        }
      }
    }
  }
}

void Settings::save_optimization() {
  int start, stop;
  start = 0; stop = detectors_.size() - 1;

  for (int i = start; i <= stop; i++) {
    //PL_DBG << "Saving optimization channel " << i << " settings for " << detectors_[i].name_;
    detectors_[i].settings_ = Gamma::Setting();
    detectors_[i].settings_.index = i;
    save_det_settings(detectors_[i].settings_, settings_tree_);
    if (detectors_[i].settings_.branches.size() > 0) {
      detectors_[i].settings_.metadata.setting_type = Gamma::SettingType::stem;
      detectors_[i].settings_.id_ = "Optimization";
    } else {
      detectors_[i].settings_.metadata.setting_type = Gamma::SettingType::none;
      detectors_[i].settings_.id_.clear();
    }


  }
}

void Settings::load_optimization() {
  int start, stop;
  start = 0; stop = detectors_.size() - 1;

  for (int i = start; i <= stop; i++)
    load_optimization(i);
}

void Settings::load_optimization(int i) {
  if ((i < 0) || (i >= detectors_.size()))
    return;
  if (detectors_[i].settings_.metadata.setting_type == Gamma::SettingType::stem) {
    detectors_[i].settings_.index = i;
    for (auto &q : detectors_[i].settings_.branches.my_data_) {
      q.index = i;
      load_det_settings(q, settings_tree_);
    }
  }
}


void Settings::save_det_settings(Gamma::Setting& result, const Gamma::Setting& root, bool precise_index) const {
  if (root.metadata.setting_type == Gamma::SettingType::stem) {
    for (auto &q : root.branches.my_data_)
      save_det_settings(result, q, precise_index);
    //PL_DBG << "maybe created stem " << stem << "/" << root.name;
  } else if ((root.metadata.setting_type != Gamma::SettingType::detector) &&
             ((precise_index && (root.index == result.index)) ||
              (!precise_index && (root.indices.count(result.index) > 0)))
             )
  {
    Gamma::Setting set(root);
    set.index = result.index;
    set.indices.clear();
    result.branches.add(set);
    //PL_DBG << "saved setting " << stem << "/" << root.name;
  }
}

void Settings::load_det_settings(Gamma::Setting det, Gamma::Setting& root, bool precise_index) {
  if ((root.metadata.setting_type == Gamma::SettingType::none) || det.id_.empty())
    return;
  if (root.metadata.setting_type == Gamma::SettingType::stem) {
    //PL_DBG << "comparing stem for " << det.name << " vs " << root.name;
    for (auto &q : root.branches.my_data_)
      load_det_settings(det, q, precise_index);
  } else if ((det.id_ == root.id_) && root.metadata.writable &&
             ((precise_index && (root.index == det.index)) ||
              (!precise_index && (root.indices.count(det.index) > 0)))
             ) {
    //PL_DBG << "applying " << det.name;
    root.value_dbl = det.value_dbl;
    root.value_int = det.value_int;
    root.value_text = det.value_text;
  }
}

void Settings::set_setting(Gamma::Setting address, bool precise_index) {
  if ((address.index < 0) || (address.index >= detectors_.size()))
    return;
  load_det_settings(address, settings_tree_, precise_index);
  write_settings_bulk();
  read_settings_bulk();
}

void Settings::get_all_settings() {
  pixie_plugin_->get_all_settings();
  read_settings_bulk();
}

}
