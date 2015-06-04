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
 *      Pixie::Settings online and offline setting describing the state of
 *      a Pixie-4 device.
 *      Not thread safe, mostly for speed concerns (getting stats during run)
 *
 ******************************************************************************/

#include "px_settings.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "common_xia.h"
#include "custom_logger.h"

namespace Pixie {

const int N_SYSTEM_PAR = 1;
const int N_MODULE_PAR = 1;
const int N_CHANNEL_PAR = 1;

Settings::Settings() : dev(nullptr), udev(nullptr) {
  DevFound = 0;

  num_chans_ = 32;  //This might need to be changed

  live_ = LiveStatus::dead;
  detectors_.resize(num_chans_);

  initialize();
}

Settings::Settings(const Settings& other):
  live_(LiveStatus::history), //this is why we have this function, deactivate if copied
  num_chans_(other.num_chans_), detectors_(other.detectors_),
  settings_tree_(other.settings_tree_) {}

Settings::Settings(tinyxml2::XMLElement* root):
  Settings()
{
  from_xml(root);
}

bool Settings::boot() {
  if (live_ == LiveStatus::history)
    return false;

  if ((DevFound=xxusb_devices_find(xxusbDev))>0)
  {
    for (int i=0; i < DevFound; i++)
    {
      dev = xxusbDev[i].usbdev;
      PL_INFO << "[" << (i+1) << "]"
              << " ProductID=" << dev->descriptor.idProduct
              << " Serial=" << xxusbDev[i].SerialString;
    }
    udev = xxusb_device_open(dev);
    if (udev)
      if (dev->descriptor.idProduct==XXUSB_CCUSB_PRODUCT_ID)
        PL_WARN << "Wrong device: CC-USB Found with Serial Number " << xxusbDev[0].SerialString;
      else if (dev->descriptor.idProduct==XXUSB_VMUSB_PRODUCT_ID)
        PL_INFO << "VM-USB Found with Serial Number " << xxusbDev[0].SerialString;
  }

  // Make sure VM_USB opened OK
  if (!udev) {
    PL_WARN << "Failedto Open VM_USB";
    live_ = LiveStatus::offline;
    return true;
  }

  // configure VM-USB
  PL_INFO << "set_vm_usb";
  set_vm_usb(udev);
  PL_INFO << "set_vm_usb finished";

  // configure MADC-32
  PL_INFO << "Threshold will be " << threshold_;
  set_madc(udev, threshold_);

  // write stacks IRQ=0 and Slot=0 for external trigger
  set_stack(udev, 0, 0xcd, 0);
  // configure MSCF-16
  //  shaper_configure(udev);

#ifdef SCALER_MODE
  set_scaler(udev);
#endif

  // print all settings
  print_xxusb(udev);
  print_madc(udev);
  // reset and enable VM-USB scaler
  //  long Data;
  //  VME_register_read(udev, XXUSB_DEV_REGISTER, &Data);
  //  VME_register_write(udev, XXUSB_DEV_REGISTER, ((Data & ~XXUSB_DEV_SCLRA_RESET) | \
  XXUSB_DEV_SCLRA_ENABLE));

#ifdef SCALER_MODE
  VME_write_32(udev, 0xd, SCLR_ADDR(SIS3820_KEY_OPERATION_ENABLE),
               SIS3820_KEY_OPERATION_ENABLE);
#endif

  // set DGG 1 to create ADC gate, I1 -> O1 delay:0, gate 0x400 = 12us
  int ret = VME_DGG(udev,0,1,0,0x0,0x100,0,0);

  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_FIFO_RESET), 0);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_READ_RESET), 0);

  live_ = LiveStatus::online;
  return true;
}

void Settings::reset() {
  //set_vm_usb(udev);
  //set_madc(udev, threshold_);

  /*  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_MULTIEVENT), MADC_MULTIEVENT_LIMIT);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_MULTIEVENT), MADC_MULTIEVENT_ON);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_ADC_RES), MADC_ADC_RES_4K_HI);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_IN_RANGE), MADC_IN_RANGE_4V);*/

  set_stack(udev, 0, 0xcd, 0);
  print_xxusb(udev);
  print_madc(udev);
  //VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_FIFO_RESET), 0);
  //VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_READ_RESET), 0);
}


Detector Settings::get_detector(Channel ch) const {
  if (ch == Channel::current)
    return detectors_[static_cast<int>(current_channel_)];
  else if ((static_cast<int>(ch) > -1) && (static_cast<int>(ch) < num_chans_))
    return detectors_[static_cast<int>(ch)];
  else
    return Detector();
}

void Settings::set_detector(Channel ch, const Detector& det) {
  if ((static_cast<int>(ch) > -1) && (static_cast<int>(ch) < num_chans_)) {
    PL_INFO << "Setting detector [" << det.name_ << "] for channel " << static_cast<int>(ch);
    detectors_[static_cast<int>(ch)] = det;
  }

  //set for all?
}

void Settings::save_optimization(Channel chan) {
  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = num_chans_;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < num_chans_){
    start = stop = static_cast<int>(chan);
  }

  //current module only

  /*for (int i = start; i < stop; i++) {
    detectors_[i].setting_names_.resize(N_CHANNEL_PAR);
    detectors_[i].setting_values_.resize(N_CHANNEL_PAR);
    for (int j = 0; j < N_CHANNEL_PAR; j++) {
      if (chan_set_meta_[j].writable) {
        detectors_[i].setting_names_[j] = chan_set_meta_[j].name;
        detectors_[i].setting_values_[j] = get_chan(j, Channel(i), Module::current);
      }
    }
    }*/
}

void Settings::load_optimization(Channel chan) {
  int start, stop;
  if (chan == Channel::all) {
    start = 0; stop = num_chans_;
  } else if (static_cast<int>(chan) < 0)
    return;
  else if (static_cast<int>(chan) < num_chans_){
    start = stop = static_cast<int>(chan);
  }

  //current module only
  
  /*for (int i = start; i < stop; i++) {
    if (detectors_[i].setting_values_.size()) {
      PL_INFO << "Optimizing channel " << i << " settings for " << detectors_[i].name_;
      for (std::size_t j = 0; j < detectors_[i].setting_values_.size(); j++) {
        if (chan_set_meta_[j].writable)
          set_chan(detectors_[i].setting_names_[j], detectors_[i].setting_values_[j], Channel(i), Module::current);
      }
    }
    }*/
}

////////////////////////////////////////
///////////////Settings/////////////////
////////////////////////////////////////

void Settings::get_all_settings() {
  read_settings_bulk();
  /*get_sys_all();
  get_mod_all(); //might want this for more modules
  for (int i=0; i < num_chans_; i++) {
    get_chan_all(Channel(i));
    }*/
};

void Settings::set_threshold(uint32_t nt) {
  threshold_ = nt;
}

bool Settings::read_setting_MADC(Setting &set) {
  if (live_ != LiveStatus::online)
    return true; //should be false
  
  if (set.node_type == NodeType::stem) {
    PL_DBG << "will traverse branches of " << set.name;
    for (int i=0; i < set.branches.size(); ++i) {
      Setting newstem = set.branches.get(i);
      bool ret = read_setting_MADC(newstem);
      if (ret) {
        set.branches.add_a(newstem);
        PL_DBG << "success reading " << set.branches.get(i).name;
      } else
        PL_DBG << "failed reading " << set.branches.get(i).name;
    }
  } else if (set.node_type == NodeType::setting) {
    PL_DBG << "reading " << set.name;    
    long data = 0;
    VME_read_16(udev, AM_MADC, MADC_ADDR(set.address), &data);
    if (set.setting_type == SettingType::floating)
      set.value = data;
    else if ((set.setting_type == SettingType::integer) ||
             (set.setting_type == SettingType::boolean))
      set.value_int = data;
  }
  return true;  
}

bool Settings::write_setting_MADC(const Setting &set) {
  if (live_ != LiveStatus::online)
    return true; //should be false
  
  if (set.node_type == NodeType::stem) {
    PL_DBG << "will traverse branches of " << set.name;
    for (int i=0; i < set.branches.size(); ++i)
      if (write_setting_MADC(set.branches.get(i)))
        PL_DBG << "success writing " << set.branches.get(i).name;
      else
        PL_DBG << "failed writing " << set.branches.get(i).name;
  } else if (set.node_type == NodeType::setting) {
    PL_DBG << "writing " << set.name;    
    long data = 0;
    if (set.setting_type == SettingType::floating)
      data = set.value;
    else if ((set.setting_type == SettingType::integer) ||
             (set.setting_type == SettingType::boolean))
      data = set.value_int;
    VME_write_16(udev, AM_MADC, MADC_ADDR(set.address), data);
  }
  return true;  
}

bool Settings::read_settings_bulk(){
  if (settings_tree_.name == "MADC-32") {
    Setting newstem = settings_tree_;
    bool ret = read_setting_MADC(newstem);
    if (ret)
      settings_tree_ = newstem;
    return true;
  }
  return false;
}

bool Settings::write_settings_bulk(){
  if (settings_tree_.name == "MADC-32")
    return write_setting_MADC(settings_tree_);
}


void Settings::initialize() {
  Setting threshold;
  threshold.node_type = NodeType::setting;
  threshold.name = "threshold";
  threshold.setting_type = SettingType::integer;
  threshold.minimum = 0;
  threshold.maximum = 8191;
  threshold.step = 1;
  threshold.writable = true;
  threshold.address = MADC_THRESH_CH0;

  Setting threshold_tree;
  threshold_tree.node_type = NodeType::stem;
  threshold_tree.index = 0;
  threshold_tree.writable = false;
  threshold_tree.unit = "channels";
  threshold_tree.name = "threshold";
  threshold_tree.description = "8191 to disable";
  for (int i=0; i < 32; ++i) {
    threshold.name = "channel " + std::to_string(i);
    threshold_tree.branches.add(threshold);
    threshold.address++;
    threshold.index = ( MADC_THRESH_CH0 | (i << 1) );
  }

  Setting multi_event;
  multi_event.node_type = NodeType::setting;
  multi_event.name = "multi event";
  multi_event.setting_type = SettingType::integer;
  //  threshold.unit = "channels";
  multi_event.minimum = 0;
  multi_event.maximum = 3;
  multi_event.step = 1;
  multi_event.writable = true;
  multi_event.address = MADC_MULTIEVENT;
  multi_event.description = "0->no, 1->yes, 3->limited";

  
  Setting adc_resolution;
  adc_resolution.node_type = NodeType::setting;
  adc_resolution.name = "ADC resolution";
  adc_resolution.setting_type = SettingType::integer;
  //  threshold.unit = "channels";
  adc_resolution.minimum = 0;
  adc_resolution.maximum = 4;
  adc_resolution.step = 1;
  adc_resolution.writable = true;
  adc_resolution.address = MADC_ADC_RES;
  adc_resolution.description = "0->2k, 1->4k, 2->4k hires, 3->8k, 4->8k hires";

  Setting input_range;
  input_range.node_type = NodeType::setting;
  input_range.name = "input range";
  input_range.setting_type = SettingType::integer;
  //  threshold.unit = "channels";
  input_range.minimum = 0;
  input_range.maximum = 2;
  input_range.step = 1;
  input_range.writable = true;
  input_range.address = MADC_IN_RANGE;
  input_range.description = "0->4V, 1->10V, 2->8V";
  
  settings_tree_.node_type = NodeType::stem;
  settings_tree_.index = 0;
  settings_tree_.writable = false;
  settings_tree_.name = "MADC-32";
  settings_tree_.branches.add(threshold_tree);
  settings_tree_.branches.add(multi_event);
  settings_tree_.branches.add(adc_resolution);
  settings_tree_.branches.add(input_range);
}

Setting Settings::pull_settings() {
  return settings_tree_;
}

void Settings::push_settings(const Setting& newsettings) {
  settings_tree_ = newsettings;
  write_settings_bulk();
  PL_INFO << "settings pushed";
}

void Settings::to_xml(tinyxml2::XMLPrinter& printer) {
  printer.OpenElement("PixieSettings");
  settings_tree_.to_xml(printer);
  printer.CloseElement(); //Settings
}

void Settings::from_xml(tinyxml2::XMLElement* root) {
  live_ = LiveStatus::history;
  detectors_.resize(num_chans_); //hardcoded

  tinyxml2::XMLElement* TopElement = root->FirstChildElement();
  while (TopElement != nullptr) {
    std::string topElementName(TopElement->Name());
    if (topElementName == settings_tree_.name)
      settings_tree_.from_xml(TopElement);
    TopElement = dynamic_cast<tinyxml2::XMLElement*>(TopElement->NextSibling());
  }
}


}
