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
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include <iomanip>


namespace Qpx {

Engine::Engine() {
  aggregate_status_ = DeviceStatus(0);

  total_det_num_.id_ = "Total detectors";
  total_det_num_.name = "Total detectors";
  total_det_num_.writable = true;
  total_det_num_.visible = true;
  total_det_num_.setting_type = Gamma::SettingType::integer;
  total_det_num_.minimum = 1;
  total_det_num_.maximum = 42;
  total_det_num_.step = 1;
}


void Engine::initialize(std::string path) {
  //don't allow this twice?

  path_ = path;

  std::string settings_file = path + "/qpx_settings.set";

  pugi::xml_document doc;
  if (!doc.load_file(settings_file.c_str()))
    return;
  PL_DBG << "<Engine> Loading device settings " << settings_file;

  pugi::xml_node root = doc.child(Gamma::Setting().xml_element_name().c_str());
  if (!root)
    return;

  Gamma::Setting tree(root);

  tree.metadata.setting_type = Gamma::SettingType::stem;

  for (auto &q : tree.branches.my_data_) {
    if (q.id_ != "Detectors") {
      DaqDevice* device = DeviceFactory::getInstance().create_type(q.id_, path + q.value_text);
      if (device != nullptr) {
        PL_DBG << "<Engine> Success loading " << device->device_name();
        devices_[q.id_] = device;
      }
    }
  }

  push_settings(tree);
  get_all_settings();
  save_optimization();
}

Engine::~Engine() {
  for (auto &q : devices_)
    if (q.second != nullptr) {
      PL_DBG << "<Engine> Destroying " << q.first;
      delete q.second;
    }
}

Gamma::Setting Engine::pull_settings() const {
  return settings_tree_;
}

void Engine::push_settings(const Gamma::Setting& newsettings) {
  //PL_INFO << "settings pushed";

  settings_tree_ = newsettings;
  write_settings_bulk();
}

bool Engine::read_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_) {
    //PL_INFO << "read bulk "  << set.name;
    if (set.id_ == "Detectors") {
      for (auto &q : set.branches.my_data_) {
        if (q.id_ == "Total Detectors")
          q.value_int = detectors_.size();

      }
      //set.metadata.step = 2; //to always save
      Gamma::Setting totaldets = set.get_setting(Gamma::Setting("Total detectors"), Gamma::Match::id);
      totaldets.value_int = detectors_.size();
      totaldets.metadata = total_det_num_;

      Gamma::Setting det = set.get_setting(Gamma::Setting("Detector"), Gamma::Match::id);

      set.branches.clear();
      set.branches.add_a(totaldets);

      for (int i=0; i < detectors_.size(); ++i) {
        det.value_text = detectors_[i].name_;
        det.index = i;
        det.indices.clear();
        det.indices.insert(i);
        det.metadata.writable = true;
        set.branches.add_a(det);
      }

      save_optimization();
    } else if (devices_.count(set.id_)) {
      //PL_DBG << "read settings bulk > " << set.id_;
      devices_[set.id_]->read_settings_bulk(set);
    }

  }
  return true;
}

bool Engine::write_settings_bulk(){
  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.id_ == "Detectors") {
      Gamma::Setting totaldets = set.get_setting(Gamma::Setting("Total detectors"), Gamma::Match::id);
      Gamma::Setting det = set.get_setting(Gamma::Setting("Detector"), Gamma::Match::id);
      int oldtotal = detectors_.size();

      if (oldtotal != totaldets.value_int)
        detectors_.resize(totaldets.value_int);

      for (auto &q : set.branches.my_data_) {
        if (q.index < detectors_.size()) {
          if (detectors_[q.index].name_ != q.value_text)
            detectors_[q.index] = Gamma::Detector(q.value_text);
        }
      }

      if (oldtotal != totaldets.value_int) {
        set.branches.clear();
        set.branches.add_a(totaldets);

        for (int i=0; i < detectors_.size(); ++i) {
          det.value_text = detectors_[i].name_;
          det.index = i;
          det.indices.clear();
          det.indices.insert(i);
          det.metadata.writable = true;
          set.branches.add_a(det);
        }
      }

    } else if (devices_.count(set.id_)) {
      //PL_DBG << "write settings bulk > " << set.id_;
      devices_[set.id_]->write_settings_bulk(set);
    }
  }
  return true;
}

bool Engine::execute_command(){

  bool success = false;
  for (auto &set : settings_tree_.branches.my_data_) {
    if (devices_.count(set.id_)) {
      //PL_DBG << "execute > " << set.id_;
      if ((devices_[set.id_] != nullptr) && (devices_[set.id_]->status() & DeviceStatus::can_exec) &&
          (devices_[set.id_]->execute_command(set)))
        success = true;
    }
  }
  if (success)
    get_all_settings();

  return success;
}

bool Engine::boot() {
  PL_INFO << "<Engine> Booting system...";

  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::can_boot)) {
      success |= q.second->boot();
      //PL_DBG << "daq_start > " << q.second->device_name();
    }

  if (success) {
    PL_INFO << "<Engine> Boot successful.";
    //settings_tree_.value_int = 2;
    get_all_settings();
  }

  return success;
}

bool Engine::die() {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::booted)) {
      success |= q.second->die();
      //PL_DBG << "die > " << q.second->device_name();
    }

  if (success) {
    //settings_tree_.value_int = 0;
    get_all_settings();
  }

  return success;
}

std::vector<Trace> Engine::oscilloscope() {
  std::vector<Trace> traces(detectors_.size());

  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::can_oscil)) {
      //PL_DBG << "oscil > " << q.second->device_name();
      std::map<int, std::vector<uint16_t>> trc = q.second->oscilloscope();
      for (auto &p : trc) {
        if ((p.first >= 0) && (p.first < detectors_.size())) {
          traces[p.first].data = p.second;
          traces[p.first].index = p.first;
          traces[p.first].detector = detectors_[p.first];
        }
      }
    }
  return traces;
}

bool Engine::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::can_run)) {
      success |= q.second->daq_start(out_queue);
      //PL_DBG << "daq_start > " << q.second->device_name();
    }
  return success;
}

bool Engine::daq_stop() {
  bool success = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::can_run)) {
      success |= q.second->daq_stop();
      //PL_DBG << "daq_stop > " << q.second->device_name();
    }
  return success;
}

bool Engine::daq_running() {
  bool running = false;
  for (auto &q : devices_)
    if ((q.second != nullptr) && (q.second->status() & DeviceStatus::can_run)) {
      running |= q.second->daq_running();
      //PL_DBG << "daq_check > " << q.second->device_name();
    }
  return running;
}

bool Engine::write_detector(const Gamma::Setting &set) {
  if (set.metadata.setting_type != Gamma::SettingType::detector)
    return false;

  if ((set.index < 0) || (set.index >= detectors_.size()))
    return false;

  if (detectors_[set.index].name_ != set.value_text)
    detectors_[set.index] = Gamma::Detector(set.value_text);

  return true;
}

void Engine::set_detector(int ch, Gamma::Detector det) {
  if (ch < 0 || ch >= detectors_.size())
    return;
  detectors_[ch] = det;
  //PL_DBG << "set det #" << ch << " to  " << det.name_;

  for (auto &set : settings_tree_.branches.my_data_) {
    if (set.id_ == "Detectors") {
      for (auto &q : set.branches.my_data_) {
        if (q.index == ch) {
          //    PL_DBG << "set det in tree #" << q.index << " to  " << detectors_[q.index].name_;

          q.value_text = detectors_[q.index].name_;
          load_optimization(q.index);
        }
      }
    }
  }
}

void Engine::save_optimization() {
  int start, stop;
  start = 0; stop = detectors_.size() - 1;

  for (int i = start; i <= stop; i++) {
    //PL_DBG << "Saving optimization channel " << i << " settings for " << detectors_[i].name_;
    detectors_[i].settings_ = Gamma::Setting();
    detectors_[i].settings_.index = i;
    save_det_settings(detectors_[i].settings_, settings_tree_, Gamma::Match::indices);
    if (detectors_[i].settings_.branches.size() > 0) {
      detectors_[i].settings_.metadata.setting_type = Gamma::SettingType::stem;
      detectors_[i].settings_.id_ = "Optimization";
    } else {
      detectors_[i].settings_.metadata.setting_type = Gamma::SettingType::none;
      detectors_[i].settings_.id_.clear();
    }


  }
}

void Engine::load_optimization() {
  int start, stop;
  start = 0; stop = detectors_.size() - 1;

  for (int i = start; i <= stop; i++)
    load_optimization(i);
}

void Engine::load_optimization(int i) {
  if ((i < 0) || (i >= detectors_.size()))
    return;
  if (detectors_[i].settings_.metadata.setting_type == Gamma::SettingType::stem) {
    detectors_[i].settings_.index = i;
    for (auto &q : detectors_[i].settings_.branches.my_data_) {
      q.index = i;
      load_det_settings(q, settings_tree_, Gamma::Match::id | Gamma::Match::indices);
    }
  }
}


void Engine::save_det_settings(Gamma::Setting& result, const Gamma::Setting& root, Gamma::Match flags) const {
  if (root.metadata.setting_type == Gamma::SettingType::stem) {
    for (auto &q : root.branches.my_data_)
      save_det_settings(result, q, flags);
    //PL_DBG << "maybe created stem " << stem << "/" << root.name;
  } else if ((root.metadata.setting_type != Gamma::SettingType::detector) && root.compare(result, flags))
  {
    Gamma::Setting set(root);
    set.index = result.index;
    set.indices.clear();
    result.branches.add(set);
    //PL_DBG << "saved setting " << stem << "/" << root.name;
  }
}

void Engine::load_det_settings(Gamma::Setting det, Gamma::Setting& root, Gamma::Match flags) {
  if ((root.metadata.setting_type == Gamma::SettingType::none) || det.id_.empty())
    return;
  if (root.metadata.setting_type == Gamma::SettingType::stem) {
    //PL_DBG << "comparing stem for " << det.name << " vs " << root.name;
    for (auto &q : root.branches.my_data_)
      load_det_settings(det, q, flags);
  } else if (root.compare(det, flags)) {
    //PL_DBG << "applying " << det.name;
    root.value_dbl = det.value_dbl;
    root.value_int = det.value_int;
    root.value_text = det.value_text;
  }
}

void Engine::set_setting(Gamma::Setting address, Gamma::Match flags) {
  if ((address.index < 0) || (address.index >= detectors_.size()))
    return;
  load_det_settings(address, settings_tree_, flags);
  write_settings_bulk();
  read_settings_bulk();
}

void Engine::get_all_settings() {
  aggregate_status_ = DeviceStatus(0);
  for (auto &q : devices_) {
    q.second->get_all_settings();
    aggregate_status_ = aggregate_status_ | q.second->status();
  }
  read_settings_bulk();
}

void Engine::getMca(uint64_t timeout, SpectraSet& spectra, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);

  if (!(aggregate_status_ & DeviceStatus::can_run)) {
    PL_WARN << "<Engine> No devices exist that can perform acquisition";
    return;
  }

  PL_INFO << "<Engine> Multithreaded spectra acquisition scheduled for " << timeout << " seconds";

  CustomTimer *anouncement_timer = nullptr;
  double secs_between_anouncements = 5;

  SynchronizedQueue<Spill*> parsedQueue;

  boost::thread builder(boost::bind(&Qpx::Engine::worker_MCA, this, &parsedQueue, &spectra));

  Spill* spill = new Spill;
  get_all_settings();
  save_optimization();
  spill->run.state = pull_settings();
  spill->run.detectors = get_detectors();
  boost::posix_time::ptime time_start = boost::posix_time::microsec_clock::local_time();
  spill->run.time_start = time_start;
  parsedQueue.enqueue(spill);

  if (daq_start(&parsedQueue))
    PL_DBG << "<Engine> Started device daq threads";

  CustomTimer total_timer(timeout, true);
  anouncement_timer = new CustomTimer(true);

  while (daq_running()) {
    wait_ms(1000);
    if (anouncement_timer->s() > secs_between_anouncements) {
      if (timeout > 0)
        PL_INFO << "  RUNNING Elapsed: " << total_timer.done()
                << "  ETA: " << total_timer.ETA();
      else
        PL_INFO << "  RUNNING Elapsed: " << total_timer.done();

      delete anouncement_timer;
      anouncement_timer = new CustomTimer(true);
    }
    if (interruptor.load() || (timeout && total_timer.timeout())) {
      if (daq_stop())
        PL_DBG << "<Engine> Stopped device daq threads successfully";
      else
        PL_ERR << "<Engine> Failed to stop device daq threads";
    }
  }

  delete anouncement_timer;

  spill = new Spill;
  get_all_settings();
  save_optimization();
  spill->run.state = pull_settings();
  spill->run.detectors = get_detectors();
  //spill->spill_number = -1;
  //  spill->run->total_events = 1; //BAD!!!!
  spill->run.time_stop = boost::posix_time::microsec_clock::local_time();
  parsedQueue.enqueue(spill);

  wait_ms(500);
  while (parsedQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  parsedQueue.stop();
  wait_ms(500);

  builder.join();
}

void Engine::getFakeMca(Simulator& source, SpectraSet& spectra,
                        uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "<Engine> Multithreaded acquisition simulation scheduled for " << timeout << " seconds";

  SynchronizedQueue<Spill*> eventQueue;

  boost::thread builder(boost::bind(&Qpx::Engine::worker_MCA, this, &eventQueue, &spectra));
  worker_fake(&source, &eventQueue, &interruptor);
  while (eventQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  eventQueue.stop();
  wait_ms(500);
  builder.join();
}

void Engine::simulateFromList(Sorter &sorter, SpectraSet &spectra, boost::atomic<bool> &interruptor) {
  boost::unique_lock<boost::mutex> lock(mutex_);
  PL_INFO << "<Engine> Acquisition simulation from saved list data";

  SynchronizedQueue<Spill*> eventQueue;

  boost::thread builder(boost::bind(&Qpx::Engine::worker_MCA, this, &eventQueue, &spectra));
  worker_from_list(&sorter, &eventQueue, &interruptor);
  while (eventQueue.size() > 0)
    wait_ms(1000);
  wait_ms(500);
  eventQueue.stop();
  wait_ms(500);
  builder.join();

}


ListData* Engine::getList(uint64_t timeout, boost::atomic<bool>& interruptor) {

  boost::unique_lock<boost::mutex> lock(mutex_);

  if (!(aggregate_status_ & DeviceStatus::can_run)) {
    PL_WARN << "<Engine> No devices exist that can perform acquisition";
    return nullptr;
  }

  Spill* one_spill;
  ListData* result = new ListData;

  PL_INFO << "<Engine> Multithreaded list mode acquisition scheduled for " << timeout << " seconds";

  CustomTimer *anouncement_timer = nullptr;
  double secs_between_anouncements = 5;

  get_all_settings();
  save_optimization();
  result->run.state = pull_settings();
  result->run.detectors = get_detectors();
  result->run.time_start = boost::posix_time::microsec_clock::local_time();

  SynchronizedQueue<Spill*> parsedQueue;

  if (daq_start(&parsedQueue))
    PL_DBG << "<Engine> Started device daq threads";

  CustomTimer total_timer(timeout, true);
  anouncement_timer = new CustomTimer(true);

  while (daq_running()) {
    wait_ms(1000);
    if (anouncement_timer->s() > secs_between_anouncements) {
      PL_INFO << "  RUNNING Elapsed: " << total_timer.done()
              << "  ETA: " << total_timer.ETA();
      delete anouncement_timer;
      anouncement_timer = new CustomTimer(true);
    }
    if (interruptor.load() || (timeout && total_timer.timeout())) {
      if (daq_stop())
        PL_DBG << "<Engine> Stopped device daq threads successfully";
      else
        PL_ERR << "<Engine> Failed to stop device daq threads";
    }
  }

  delete anouncement_timer;

  result->run.time_stop = boost::posix_time::microsec_clock::local_time();

  wait_ms(500);

  while (parsedQueue.size() > 0) {
    one_spill = parsedQueue.dequeue();
    for (auto &q : one_spill->hits)
      result->hits.push_back(q);
    delete one_spill;
  }

  parsedQueue.stop();
  return result;
}

//////STUFF BELOW SHOULD NOT BE USED DIRECTLY////////////
//////ASSUME YOU KNOW WHAT YOU'RE DOING WITH THREADS/////

void Engine::worker_MCA(SynchronizedQueue<Spill*>* data_queue,
                        SpectraSet* spectra) {

  std::vector<int> queue_status;
  queue_status.resize(detectors_.size(), 0);
  // 0 = unused, 1 = empty, 2 = data

  std::list<Spill*> current_spills;

  PL_DBG << "<Engine> Spectra builder thread initiated";
  Spill* in_spill;
  Spill* out_spill;
  while (true) {
    in_spill = data_queue->dequeue();
    if (in_spill != nullptr) {
      for (auto &q : in_spill->stats) {
        if (q.channel >= queue_status.size())
          queue_status.resize(q.channel);
        if ((q.channel >= 0) && (q.channel < queue_status.size())) {
          if (!in_spill->hits.empty())
            queue_status[q.channel] = 2;
          else
            queue_status[q.channel] = 1;
        }
      }
      current_spills.push_back(in_spill);
    }

    bool empty = false;
    while (!empty) {

      out_spill = new Spill;

      empty = queue_status.empty();
      if (in_spill != nullptr) {
        for (auto &q : queue_status)
          if (q != 0)
            q = 1;
        for (auto i = current_spills.begin(); i != current_spills.end(); i++) {
          if (!(*i)->hits.empty())
            for (auto &q : (*i)->stats) {
              if ((q.channel >= 0) && (q.channel < queue_status.size()))
                queue_status[q.channel] = 2;
            }
        }
      } else {
        for (auto &q : queue_status)
          if (q != 0)
            q = 2;        
      }

      for (auto q : queue_status)
        if (q == 1)
          empty = true;


      while (!empty) {
        Hit oldest;
        for (auto &q : current_spills) {
          if (q->hits.empty()) {
            empty = true;
            break;
          } else if ((oldest == Hit()) || (q->hits.front().timestamp < oldest.timestamp))
            oldest = q->hits.front();
        }
        if (!empty) {
          out_spill->hits.push_back(oldest);
          for (auto &q : current_spills)
            if ((!q->hits.empty()) && (q->hits.front().timestamp == oldest.timestamp)) {
              q->hits.pop_front();
              break;
            }
        }
        if (current_spills.empty())
          empty = true;
      }

      bool noempties = false;
      while (!noempties) {

        noempties = true;
        for (auto i = current_spills.begin(); i != current_spills.end(); i++)
          if ((*i)->hits.empty()) {
//            out_spill->spill_number = (*i)->spill_number;
            out_spill->data = (*i)->data;
            out_spill->stats = (*i)->stats;
            out_spill->run = (*i)->run;
            spectra->add_spill(out_spill);
            //            PL_DBG << "enqueued some spill n=" << out_spill->spill_number;

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

  PL_DBG << "<Engine> Spectra builder thread terminating";

  spectra->closeAcquisition();
}

void Engine::worker_fake(Simulator* source, SynchronizedQueue<Spill*>* data_queue, boost::atomic<bool>* interruptor) {

  PL_DBG << "<Engine> Simulated event generator initiated";

  uint32_t secsperrun = 5;  ///calculate this based on ocr and buf size
  Spill* one_spill;

  uint64_t /*spill_number = 0,*/ event_count = 0;
  bool timeout = false;
  boost::posix_time::ptime session_start_time, block_time;

  uint64_t   rate = source->OCR;
  StatsUpdate moving_stats,
      one_run = source->getBlock(5.05);  ///bit more than secsperrun

  //start of run status update
  //mainly for spectra to have calibration
  one_spill = new Spill;
  one_spill->run.state = source->settings;
  one_spill->run.detectors = source->detectors;
  session_start_time =  boost::posix_time::microsec_clock::local_time();
  moving_stats.lab_time = session_start_time;
  one_spill->stats.push_back(moving_stats);
  data_queue->enqueue(one_spill);

  while (!timeout) {
//    spill_number++;

    one_spill = new Spill;

    for (uint32_t i=0; i<(rate*secsperrun); i++)
      one_spill->hits.push_back(source->getHit());

    block_time =  boost::posix_time::microsec_clock::local_time();

    event_count += (rate*secsperrun);

    moving_stats = moving_stats + one_run;
//    moving_stats.spill_number = spill_number;
    moving_stats.lab_time = block_time;
    moving_stats.event_rate = one_run.event_rate;
    one_spill->stats.push_back(moving_stats);
    data_queue->enqueue(one_spill);

    boost::this_thread::sleep(boost::posix_time::seconds(secsperrun));

    if (*interruptor)
      timeout = true;
  }

  one_spill = new Spill;
  one_spill->run.state = source->settings;
  one_spill->run.time_start = session_start_time;
  one_spill->run.time_stop = block_time;
  one_spill->run.total_events = event_count;
  data_queue->enqueue(one_spill);
}


void Engine::worker_from_list(Sorter* sorter, SynchronizedQueue<Spill*>* data_queue, boost::atomic<bool>* interruptor) {
  Spill one_spill;

  while ((!((one_spill = sorter->get_spill()) == Spill())) && (!(*interruptor))) {
    data_queue->enqueue(new Spill(one_spill));
    boost::this_thread::sleep(boost::posix_time::seconds(2));
  }
  PL_DBG << "<Engine> worker_from_list terminating";
}

}
