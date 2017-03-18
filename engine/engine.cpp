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
 *      Qpx::Engine online and offline setting describing the state of
 *      a device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#include "engine.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "producer_factory.h"
#include <iomanip>


namespace Qpx {

Engine::Engine() {
  aggregate_status_ = ProducerStatus(0);
  intrinsic_status_ = ProducerStatus(0);

  total_det_num_.id_ = "Total detectors";
  total_det_num_.name = "Total detectors";
  total_det_num_.writable = true;
  total_det_num_.visible = true;
  total_det_num_.setting_type = Qpx::SettingType::integer;
  total_det_num_.minimum = 1;
  total_det_num_.maximum = 42;
  total_det_num_.step = 1;

  single_det_.id_ = "Detector";
  single_det_.name = "Detector";
  single_det_.writable = true;
  single_det_.saveworthy = true;
  single_det_.visible = true;
  single_det_.setting_type = Qpx::SettingType::detector;
}


void Engine::initialize(std::string profile_path, std::string settings_path) {
  //don't allow this twice?
  profile_path_ = profile_path;

  DBG << "<Engine> attempting to initialize profile at " << profile_path_;

  Qpx::Setting tree, descr;

  pugi::xml_document doc;
  if (doc.load_file(profile_path_.c_str()))
  {
    DBG << "<Engine> Loading device settings " << profile_path_;
    pugi::xml_node root = doc.child(Qpx::Setting().xml_element_name().c_str());
    if (root) {
      tree = Qpx::Setting(root);
      tree.metadata.saveworthy = true;
      descr = tree.get_setting(Qpx::Setting("Profile description"), Qpx::Match::id);
      descr.metadata.setting_type = Qpx::SettingType::text;
      descr.metadata.writable = true;
      tree.branches.replace(descr);
    }
  }


  boost::filesystem::path path(settings_path);

//  if (!boost::filesystem::is_directory(path)) {
//    DBG << "<Engine> Bad profile root directory. Will not proceed with loading device settings";
//    return;
//  }

  die();
  devices_.clear();

  for (auto &q : tree.branches.my_data_) {
    if (q.id_ != "Detectors") {
      boost::filesystem::path dev_settings = path / q.value_text;
      SourcePtr device = ProducerFactory::getInstance().create_type(q.id_, dev_settings.string());
      if (device) {
        DBG << "<Engine> Success loading " << device->device_name();
        devices_[q.id_] = device;
      }
    }
  }

  push_settings(tree);
  get_all_settings();

  if (!descr.value_text.empty())
    LINFO << "<Engine> Welcome to " << descr.value_text;
}

Engine::~Engine() {
  if (die())
    get_all_settings();

  if (!profile_path_.empty()) {
    get_all_settings();

    Qpx::Setting dev_settings = pull_settings();
    dev_settings.condense();
    dev_settings.strip_metadata();

    pugi::xml_document doc;
    dev_settings.to_xml(doc);

    if (doc.save_file(profile_path_.c_str()))
      ERR << "<Engine> Saved settings to " << profile_path_;
    else
      ERR << "<Engine> Failed to save device settings";
  }
}

Qpx::Setting Engine::pull_settings() const {
  return settings_tree_;
}

void Engine::push_settings(const Qpx::Setting& newsettings) {
  settings_tree_ = newsettings;
  write_settings_bulk();

//  LINFO << "settings pushed branches = " << settings_tree_.branches.size();
}

bool Engine::read_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_)
  {
    if (set.id_ == "Detectors") {

      //set.metadata.step = 2; //to always save
      Qpx::Setting totaldets(total_det_num_);
      totaldets.value_int = detectors_.size();

      Qpx::Setting det(single_det_);

      set.branches.clear();
      set.branches.add_a(totaldets);

      for (size_t i=0; i < detectors_.size(); ++i) {
        det.metadata.name = "Detector " + std::to_string(i);
        det.value_text = detectors_[i].name_;
        det.indices.clear();
        det.indices.insert(i);
        det.metadata.writable = true;
        set.branches.add_a(det);
      }

    } else if (devices_.count(set.id_)) {
      //DBG << "read settings bulk > " << set.id_;
      devices_[set.id_]->read_settings_bulk(set);
    }

  }
  save_optimization();
  return true;
}

bool Engine::write_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.id_ == "Detectors") {
      rebuild_structure(set);
    } else if (devices_.count(set.id_)) {
      //DBG << "write settings bulk > " << set.id_;
      devices_[set.id_]->write_settings_bulk(set);
    }
  }
  return true;
}

void Engine::rebuild_structure(Qpx::Setting &set) {
  Qpx::Setting totaldets = set.get_setting(Qpx::Setting("Total detectors"), Qpx::Match::id);
  int oldtotal = detectors_.size();
  int newtotal = totaldets.value_int;
  if (newtotal < 0)
    newtotal = 0;

  if (oldtotal != newtotal)
    detectors_.resize(newtotal);

}

bool Engine::boot() {
  LINFO << "<Engine> Booting system...";

  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::can_boot)) {
      success |= q.second->boot();
      //DBG << "daq_start > " << q.second->device_name();
    }

  if (success) {
    LINFO << "<Engine> Boot successful";
    //settings_tree_.value_int = 2;
    get_all_settings();
  } else
    LINFO << "<Engine> Boot failed";

  return success;
}

bool Engine::die() {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::booted)) {
      success |= q.second->die();
      //DBG << "die > " << q.second->device_name();
    }

  if (success) {
    //settings_tree_.value_int = 0;
    get_all_settings();
  }

  return success;
}

std::vector<Hit> Engine::oscilloscope() {
  std::vector<Hit> traces;
  traces.resize(detectors_.size());

  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::can_oscil)) {
      //DBG << "oscil > " << q.second->device_name();
      std::list<Hit> trc = q.second->oscilloscope();
      for (auto &p : trc)
        if ((p.source_channel() >= 0) && (p.source_channel() < static_cast<int16_t>(traces.size())))
          traces[p.source_channel()] = p;
    }
  return traces;
}

bool Engine::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::can_run)) {
      success |= q.second->daq_start(out_queue);
      //DBG << "daq_start > " << q.second->device_name();
    }
  return success;
}

bool Engine::daq_stop() {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::can_run)) {
      success |= q.second->daq_stop();
      //DBG << "daq_stop > " << q.second->device_name();
    }
  return success;
}

bool Engine::daq_running() {
  bool running = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & ProducerStatus::can_run)) {
      running |= q.second->daq_running();
      //DBG << "daq_check > " << q.second->device_name();
    }
  return running;
}

void Engine::set_detector(size_t ch, Qpx::Detector det) {
  if (ch >= detectors_.size())
    return;
  detectors_[ch] = det;
  //DBG << "set det #" << ch << " to  " << det.name_;

  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.id_ == "Detectors") {
      for (auto &q : set.branches.my_data_) {
        if (q.indices.count(ch) > 0) {
          //    DBG << "set det in tree #" << ch << " to  " << detectors_[ch].name_;

          q.value_text = detectors_[ch].name_;
          load_optimization(ch);
        }
      }
    }
  }
}

void Engine::save_optimization() {
  for (size_t i = 0; i < detectors_.size(); i++) {
//    DBG << "Saving optimization channel " << i << " settings for " << detectors_[i].name_;
//    detectors_[i].settings_ = Qpx::Setting();
    detectors_[i].settings_.indices.insert(i);
    detectors_[i].settings_.branches.my_data_ =
      settings_tree_.find_all(detectors_[i].settings_, Qpx::Match::indices);
    if (detectors_[i].settings_.branches.size() > 0) {
      detectors_[i].settings_.metadata.setting_type = Qpx::SettingType::stem;
      detectors_[i].settings_.id_ = "Optimization";
      for (auto &q : detectors_[i].settings_.branches.my_data_)
      {
        q.indices.clear();
        q.indices.insert(i);
      }
    } else {
      detectors_[i].settings_.metadata.setting_type = Qpx::SettingType::none;
      detectors_[i].settings_.id_.clear();
    }


  }
}

void Engine::load_optimization() {
  for (size_t i = 0; i < detectors_.size(); i++)
    load_optimization(i);
}

void Engine::load_optimization(size_t i) {
  if (i >= detectors_.size())
    return;
  if (detectors_[i].settings_.metadata.setting_type == Qpx::SettingType::stem) {
    detectors_[i].settings_.indices.clear();
    detectors_[i].settings_.indices.insert(i);
    for (auto &q : detectors_[i].settings_.branches.my_data_) {
      q.indices.clear();
      q.indices.insert(i);
    }
    settings_tree_.set_all(detectors_[i].settings_.branches.my_data_, Qpx::Match::id | Qpx::Match::indices);
  }
}

void Engine::set_setting(Qpx::Setting address, Qpx::Match flags) {
  if (settings_tree_.set_setting_r(address, flags))
  {
//    DBG << "<Engine> Success setting " << address.id_;
  }
  write_settings_bulk();
  read_settings_bulk();
}

void Engine::get_all_settings() {
  aggregate_status_ = ProducerStatus(0);
  for (auto &q : devices_) {
    q.second->get_all_settings();
    aggregate_status_ = aggregate_status_ | q.second->status();
  }
  read_settings_bulk();
}

void Engine::getMca(uint64_t timeout, ProjectPtr spectra, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);

  if (!spectra) {
    WARN << "<Engine> No reference to valid daq project";
    return;
  }

  if (!(aggregate_status_ & ProducerStatus::can_run)) {
    WARN << "<Engine> No devices exist that can perform acquisition";
    return;
  }

  if (timeout > 0)
    LINFO << "<Engine> Starting acquisition scheduled for " << timeout << " seconds";
  else
    LINFO << "<Engine> Starting acquisition for indefinite run";

  CustomTimer *anouncement_timer = nullptr;
  double secs_between_anouncements = 5;

  SynchronizedQueue<Spill*> parsedQueue;

  boost::thread builder(boost::bind(&Qpx::Engine::worker_MCA, this, &parsedQueue, spectra));

  Spill* spill = new Spill;
  get_all_settings();
  spill->state = pull_settings();
  spill->detectors = get_detectors();
  parsedQueue.enqueue(spill);

  if (daq_start(&parsedQueue))
    DBG << "<Engine> Started device daq threads";

  CustomTimer total_timer(timeout, true);
  anouncement_timer = new CustomTimer(true);

  while (daq_running()) {
    wait_ms(1000);
    if (anouncement_timer->s() > secs_between_anouncements) {
      if (timeout > 0)
        LINFO << "  RUNNING Elapsed: " << total_timer.done()
                << "  ETA: " << total_timer.ETA();
      else
        LINFO << "  RUNNING Elapsed: " << total_timer.done();

      delete anouncement_timer;
      anouncement_timer = new CustomTimer(true);
    }
    if (interruptor.load() || (timeout && total_timer.timeout())) {
      if (daq_stop())
        DBG << "<Engine> Stopped device daq threads successfully";
      else
        ERR << "<Engine> Failed to stop device daq threads";
    }
  }

  delete anouncement_timer;

  spill = new Spill;
  get_all_settings();
  spill->state = pull_settings();
  parsedQueue.enqueue(spill);

  wait_ms(500);
  while (parsedQueue.size() > 0)
    wait_ms(500);
  parsedQueue.stop();
  wait_ms(500);

  builder.join();
  LINFO << "<Engine> Acquisition finished";
}

ListData Engine::getList(uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);

  if (!(aggregate_status_ & ProducerStatus::can_run)) {
    WARN << "<Engine> No devices exist that can perform acquisition";
    return ListData();
  }

  if (timeout > 0)
    LINFO << "<Engine> List mode acquisition scheduled for " << timeout << " seconds";
  else
    LINFO << "<Engine> List mode acquisition indefinite run";

  Spill* one_spill;
  ListData result;

  CustomTimer *anouncement_timer = nullptr;
  double secs_between_anouncements = 5;

  one_spill = new Spill;
  get_all_settings();
  one_spill->state = pull_settings();
  one_spill->detectors = get_detectors();
  result.push_back(SpillPtr(one_spill));

  SynchronizedQueue<Spill*> parsedQueue;

  if (daq_start(&parsedQueue))
    DBG << "<Engine> Started device daq threads";

  CustomTimer total_timer(timeout, true);
  anouncement_timer = new CustomTimer(true);

  while (daq_running()) {
    wait_ms(1000);
    if (anouncement_timer->s() > secs_between_anouncements) {
      LINFO << "  RUNNING Elapsed: " << total_timer.done()
              << "  ETA: " << total_timer.ETA();
      delete anouncement_timer;
      anouncement_timer = new CustomTimer(true);
    }
    if (interruptor.load() || (timeout && total_timer.timeout())) {
      if (daq_stop())
        DBG << "<Engine> Stopped device daq threads successfully";
      else
        ERR << "<Engine> Failed to stop device daq threads";
    }
  }

  delete anouncement_timer;

  one_spill = new Spill;
  get_all_settings();
  one_spill->state = pull_settings();
  parsedQueue.enqueue(one_spill);
//  result.push_back(SpillPtr(one_spill));

  wait_ms(500);

  while (parsedQueue.size() > 0)
    result.push_back(SpillPtr(parsedQueue.dequeue()));


  parsedQueue.stop();
  return result;
}

//////STUFF BELOW SHOULD NOT BE USED DIRECTLY////////////
//////ASSUME YOU KNOW WHAT YOU'RE DOING WITH THREADS/////

void Engine::worker_MCA(SynchronizedQueue<Spill*>* data_queue,
                        ProjectPtr spectra) {

  CustomTimer presort_timer;
  uint64_t presort_compares(0), presort_hits(0), presort_cycles(0);

  std::map<int16_t, bool> queue_status;
  // for each input channel (detector) false = empty, true = data

  std::multiset<Spill*> current_spills;

  DBG << "<Engine> Spectra builder thread initiated";
  Spill* in_spill  = nullptr;
  Spill* out_spill = nullptr;
  while (true) {
    in_spill = data_queue->dequeue();
    if (in_spill != nullptr) {
      for (auto &q : in_spill->stats) {
        if (q.second.source_channel >= 0) {
          queue_status[q.second.source_channel] = (!in_spill->hits.empty() || (q.second.stats_type == StatsType::stop));
        }
      }
      current_spills.insert(in_spill);
    }

//    if (in_spill)
//      DBG << "<Engine: worker_MCA> spill backlog " << current_spills.size()
//             << " at arrival of " << boost::posix_time::to_iso_extended_string(in_spill->time);

    bool empty = false;
    while (!empty) {

      out_spill = new Spill;

      empty = queue_status.empty();
      if (in_spill != nullptr) {
        for (auto &q : queue_status)
          q.second = false;
        for (auto i = current_spills.begin(); i != current_spills.end(); i++) {
          for (auto &q : (*i)->stats) {
            if ((q.second.source_channel >= 0) &&
                (!(*i)->hits.empty() || (q.second.stats_type == StatsType::stop) ))
              queue_status[q.second.source_channel] = true;
          }
        }
      } else {
        for (auto &q : queue_status)
          q.second = true;
      }

      for (auto q : queue_status) {
        if (!q.second)
          empty = true;
      }


      presort_cycles++;
      presort_timer.start();
      while (!empty) {
        Hit oldest;
        for (auto &q : current_spills) {
          if (q->hits.empty()) {
            empty = true;
            break;
          } else if ((oldest == Hit()) || (q->hits.front().timestamp() < oldest.timestamp())) {
            oldest = q->hits.front();
            presort_compares++;
          }
        }
        if (!empty) {
          presort_hits++;
          out_spill->hits.push_back(oldest);
          for (auto &q : current_spills)
            if ((!q->hits.empty()) && (q->hits.front().timestamp() == oldest.timestamp())) {
              q->hits.pop_front();
              presort_compares++;
              break;
            }
        }
        if (current_spills.empty())
          empty = true;
      }
      presort_timer.stop();

//      DBG << "<Engine> Presort pushed " << presort_hits << " hits, ";
//                               " time/hit: " << presort_timer.us()/presort_hits << "us, "
//                               " time/cycle: " << presort_timer.us()/presort_cycles  << "us, "
//                               " compares/hit: " << double(presort_compares)/double(presort_hits) << ", "
//                               " hits/cyle: " << double(presort_hits)/double(presort_cycles) << ", "
//                               " compares/cycle: " << double(presort_compares)/double(presort_cycles);

      bool noempties = false;
      while (!noempties) {

        noempties = true;
        for (auto i = current_spills.begin(); i != current_spills.end(); i++)
          if ((*i)->hits.empty()) {
            out_spill->time = (*i)->time;
            out_spill->data = (*i)->data;
            out_spill->stats = (*i)->stats;
            out_spill->detectors = (*i)->detectors;
            out_spill->state = (*i)->state;
            spectra->add_spill(out_spill);

            delete (*i);
            current_spills.erase(i);

            delete out_spill;
            out_spill = new Spill;
            noempties = false;
            break;
          }
      }
      delete out_spill;
    }

    if ((in_spill == nullptr) && current_spills.empty())
      break;
  }

  DBG << "<Engine> Spectra builder terminating";

  spectra->flush();
}


}
