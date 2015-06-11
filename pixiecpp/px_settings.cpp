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
#include "common_xia.h"
#include "custom_logger.h"

namespace Pixie {

Settings::Settings() : udev(nullptr) {
  num_chans_ = 32;  //This might need to be changed

  live_ = LiveStatus::dead;
  detectors_.resize(num_chans_);

  settings_tree_.node_type = NodeType::stem;
  settings_tree_.name = "MADC-32";
}

Settings::Settings(const Settings& other):
  live_(LiveStatus::history), //this is why we have this function, deactivate if copied
  num_chans_(other.num_chans_), detectors_(other.detectors_),
  settings_tree_(other.settings_tree_) {}

bool Settings::boot() {
  if (live_ == LiveStatus::history)
    return false;

  if (settings_tree_.branches.empty()) {
    PL_ERR << "No settings tree. Cannot boot";
    return false;
  }

  struct usb_device *dev = nullptr;
  xxusb_device_type xxusbDev[32];
  int DevFound = 0;

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
    PL_WARN << "Failedto Open VM_USB. Offline functions only";
    live_ = LiveStatus::offline;
    return true;
  }

  // configure VM-USB
  PL_INFO << "Configuring VM USB";
//  settings_tree_ = Setting();
  set_vm_usb(udev);

  // configure MADC-32
  PL_INFO << "Configuring MADC-32";
  write_settings_bulk();

  // configure MSCF-16
  //  shaper_configure(udev);

  // set DGG 1 to create ADC gate, I1 -> O1 delay:0, gate 0x400 = 12us
  PL_INFO << "Configuring VME DGG";
  int ret = VME_DGG(udev,0,1,0,0,1120,0,0);

  //reset();

  live_ = LiveStatus::online;
  return true;
}

void Settings::reset() {

  // write stacks IRQ=0 and Slot=0 for external trigger
  set_stack(udev, 0, 0xcd, 0);

  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_FIFO_RESET), 0);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_READ_RESET), 0);

  print_xxusb(udev);
  print_madc(udev);
}

Setting Settings::pull_settings() {
  return settings_tree_;
}

void Settings::push_settings(const Setting& newsettings) {
  settings_tree_ = newsettings;
  write_settings_bulk();
  PL_INFO << "settings pushed";
}

void Settings::get_all_settings() {
  read_settings_bulk();
};

bool Settings::read_settings_bulk(){
  if ((live_ == LiveStatus::online) && (settings_tree_.name == "MADC-32")) {
    Setting newstem = settings_tree_;
    bool ret = read_setting_MADC(newstem);
    if (ret)
      settings_tree_ = newstem;
    return true;
  }
  return false;
}

bool Settings::write_settings_bulk(){
  if ((live_ == LiveStatus::online) && (settings_tree_.name == "MADC-32")) {
    long Data;
    VME_read_16(udev, AM_MADC, MADC_ADDR(MADC_FIRMWARE), &Data);
    PL_INFO << "MADC-32 Firmware version = 0x" << std::hex << Data;
    VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_SOFT_RESET), 0);
    
    return write_setting_MADC(settings_tree_);
  }
}


bool Settings::read_setting_MADC(Setting &set) {
  if (live_ != LiveStatus::online)
    return true; //should be false
  
  if (set.node_type == NodeType::stem) {
    //PL_DBG << "will traverse branches of " << set.name;
    Setting newstem = set; newstem.branches.clear();
    for (int i=0; i < set.branches.size(); ++i) {
      Setting newbranch = set.branches.get(i);
      bool ret = read_setting_MADC(newbranch);
      newstem.branches.add(newbranch);
    }
    set = newstem;
  } else if (set.node_type == NodeType::setting) {
    //PL_DBG << "reading " << set.name;
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
    //PL_DBG << "will traverse branches of " << set.name;
    for (int i=0; i < set.branches.size(); ++i)
      write_setting_MADC(set.branches.get(i));
  } else if ((set.node_type == NodeType::setting) && (set.writable)){
    //PL_DBG << "writing " << set.name;
    long data = 0;
    if (set.setting_type == SettingType::floating)
      data = set.value;
    else if ((set.setting_type == SettingType::integer) ||
             (set.setting_type == SettingType::boolean))
      data = set.value_int;
    //PL_TRC << "will write " << set.name << " << " << data;
    VME_write_16(udev, AM_MADC, MADC_ADDR(set.address), data);
  }
  return true;  
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

Settings::Settings(tinyxml2::XMLElement* root):
  Settings()
{
  from_xml(root);
}

Detector Settings::get_detector(Channel ch) const {
  return Detector();
}

void Settings::set_detector(Channel ch, const Detector& det) {
}

void Settings::save_optimization(Channel chan) {
}

void Settings::load_optimization(Channel chan) {
}

void Settings::initialize() {

}


}
