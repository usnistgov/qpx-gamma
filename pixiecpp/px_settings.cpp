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

  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*num_chans_, 0.0);

  sys_set_meta_.resize(N_SYSTEM_PAR);
  mod_set_meta_.resize(N_MODULE_PAR);
  chan_set_meta_.resize(N_CHANNEL_PAR);
  
  current_module_ = Module::none;
  current_channel_ = Channel::none;
  live_ = LiveStatus::dead;
  detectors_.resize(num_chans_);

  current_module_ = Module(0);
  current_channel_ = Channel(0);
}

Settings::Settings(const Settings& other):
  live_(LiveStatus::history), //this is why we have this function, deactivate if copied
  num_chans_(other.num_chans_), detectors_(other.detectors_),
  current_module_(other.current_module_), current_channel_(other.current_channel_),
  system_parameter_values_(other.system_parameter_values_),
  module_parameter_values_(other.module_parameter_values_),
  channel_parameter_values_(other.channel_parameter_values_),
  sys_set_meta_(other.sys_set_meta_),
  mod_set_meta_(other.mod_set_meta_),
  chan_set_meta_(other.chan_set_meta_) {}

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
    return false;
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
  initialize();
  return true;
}

void Settings::reset() {
  //set_vm_usb(udev);
  //set_madc(udev, threshold_);

  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_MULTIEVENT), MADC_MULTIEVENT_LIMIT);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_MULTIEVENT), MADC_MULTIEVENT_ON);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_ADC_RES), MADC_ADC_RES_4K_HI);
  VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_IN_RANGE), MADC_IN_RANGE_4V);

  set_stack(udev, 0, 0xcd, 0);
  print_xxusb(udev);
  print_madc(udev);
  //VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_FIFO_RESET), 0);
  //VME_write_16(udev, AM_MADC, MADC_ADDR(MADC_READ_RESET), 0);
}


Module Settings::current_module() const {
  return current_module_;
}

void Settings::set_current_module(Module mod) {
  current_module_ = mod;
}

Channel Settings::current_channel() const {
  return current_channel_;
}

void Settings::set_current_channel(Channel chan) {
  current_channel_ = chan;
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
  if (ch == Channel::current)
    ch = current_channel_;
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

  for (int i = start; i < stop; i++) {
    detectors_[i].setting_names_.resize(N_CHANNEL_PAR);
    detectors_[i].setting_values_.resize(N_CHANNEL_PAR);
    for (int j = 0; j < N_CHANNEL_PAR; j++) {
      if (chan_set_meta_[j].writable) {
        detectors_[i].setting_names_[j] = chan_set_meta_[j].name;
        detectors_[i].setting_values_[j] = get_chan(j, Channel(i), Module::current);
      }
    }
  }
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
  
  for (int i = start; i < stop; i++) {
    if (detectors_[i].setting_values_.size()) {
      PL_INFO << "Optimizing channel " << i << " settings for " << detectors_[i].name_;
      for (std::size_t j = 0; j < detectors_[i].setting_values_.size(); j++) {
        if (chan_set_meta_[j].writable)
          set_chan(detectors_[i].setting_names_[j], detectors_[i].setting_values_[j], Channel(i), Module::current);
      }
    }
  }
}

uint8_t Settings::chan_param_num() const
{
  if (live_ == LiveStatus::dead)
    return 0;
  else
    return N_CHANNEL_PAR;
}

////////////////////////////////////////
///////////////Settings/////////////////
////////////////////////////////////////

void Settings::get_all_settings() {
  get_sys_all();
  get_mod_all(); //might want this for more modules
  for (int i=0; i < num_chans_; i++) {
    get_chan_all(Channel(i));
  }
};

void Settings::set_threshold(uint32_t nt) {
  threshold_ = nt;
}

/////System Settings//////
//////////////////////////

void Settings::set_sys(const std::string& setting, double val) {
  if (live_ == LiveStatus::history)
    return;

  PL_DBG << "Setting " << setting << " to " << val << " for system";

  //check bounds
  int ret = i_sys(setting);
  if (ret >= 0) {
    system_parameter_values_[ret] = val;
    write_sys(ret);
  }
}

void Settings::set_slots(const std::vector<uint8_t>& slots) {
}

double Settings::get_sys(const std::string& setting) {
  PL_TRC << "Getting " << setting << " for system";

  //check bounds
  int ret = i_sys(setting);
  if (ret >= 0) {
    if (live_ != LiveStatus::history)
      read_sys(ret);
    return system_parameter_values_[ret];
  } else
    return 0;
}

void Settings::get_sys_all() {
  if (live_ == LiveStatus::history)
    return;

  PL_TRC << "Getting all system settings";
  for (int i=0; i < N_SYSTEM_PAR; ++i)
    read_sys(i);
}

void Settings::set_boot_files(std::vector<std::string>& files) {
}


//////Module Settings//////
///////////////////////////

void Settings::set_mod(const std::string& setting, double val, Module mod) {
  if (live_ == LiveStatus::history)
    return;

  switch (mod) {
  case Module::current:
    mod = current_module_;
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_INFO << "Setting " << setting << " to " << val << " for module " << module;
      int ret = i_mod(setting);
      if (ret >= 0) {
        module_parameter_values_[module * N_MODULE_PAR + ret] = val;
        write_mod(ret, module);
      }
    }
  }
}

double Settings::get_mod(const std::string& setting,
                         Module mod) const {
  switch (mod) {
  case Module::current:
    mod = current_module_;
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_TRC << "Getting " << setting << " for module " << module;
      int ret = i_mod(setting);
      if (ret >= 0) {
        return module_parameter_values_[module * N_MODULE_PAR + ret];
      }
    }
    else
      return -1;
  }
}

double Settings::get_mod(const std::string& setting,
                         Module mod,
                         LiveStatus force) {
  switch (mod) {
  case Module::current:
    mod = current_module_;
  default:
    int module = static_cast<int>(mod);
    int ret = i_mod(setting);
    if ((module > -1)  && (ret > -1)) {
      PL_TRC << "Getting " << setting << " for module " << module;
      if ((force == LiveStatus::online) && (live_ == force))
        read_mod(ret, module);
      return module_parameter_values_[module * N_MODULE_PAR + ret];
    }
    else
      return -1;
  }
}

void Settings::get_mod_all(Module mod) {
  if (live_ == LiveStatus::history)
    return;
  
  switch (mod) {
  case Module::all: {
    /*loop through all*/
    break;
  }
  case Module::current:
    mod = current_module_;
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_TRC << "Getting all parameters for module " << module;
      //read_mod("ALL_MODULE_PARAMETERS", module);
    }
  }
}

void Settings::get_mod_stats(Module mod) {
  if (live_ == LiveStatus::history)
    return;
  
  switch (mod) {
  case Module::all: {
    /*loop through all*/
    break;
  }
  case Module::current:
    mod = current_module_;
  default:
    int module = static_cast<int>(mod);
    if (module > -1) {
      PL_DBG << "Getting run statistics for module " << module;
      //read_mod("MODULE_RUN_STATISTICS", module);
    }
  }
}


////////Channels////////////
////////////////////////////

void Settings::set_chan(const std::string& setting, double val,
                        Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);
  int ret = i_chan(setting);
  
  PL_DBG << "Setting " << setting << " to " << val
         << " for module " << mod << " chan "<< chan;

  if ((mod > -1) && (chan > -1) && (ret > -1)) {
    channel_parameter_values_[ret + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR] = val;
    write_chan(ret, mod, chan);
  }
}

void Settings::set_chan(uint8_t setting, double val,
                        Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);
  
  PL_DBG << "Setting " << chan_set_meta_[setting].name << " to " << val
         << " for module " << mod << " chan "<< chan;

  if ((mod > -1) && (chan > -1)) {
    channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR] = val;
    write_chan(setting, mod, chan);
  }
}

double Settings::get_chan(uint8_t setting, Channel channel, Module module) const {
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << chan_set_meta_[setting].name
         << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1)) {
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Settings::get_chan(uint8_t setting, Channel channel,
                          Module module, LiveStatus force) {
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting " << chan_set_meta_[setting].name
         << " for module " << mod << " channel " << chan;
  
  if ((mod > -1) && (chan > -1)) {
    if ((force == LiveStatus::online) && (live_ == force))
      read_chan(setting, mod, chan);
    return channel_parameter_values_[setting + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Settings::get_chan(const std::string& setting,
                          Channel channel, Module module) const {
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);
  int ret = i_chan(setting);

  PL_TRC << "Getting " << setting << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1) && (ret > -1)) {
    return channel_parameter_values_[ret + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

double Settings::get_chan(const std::string& setting,
                          Channel channel, Module module,
                          LiveStatus force) {
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);
  int ret = i_chan(setting);
  
  PL_TRC << "Getting " << setting << " for module " << mod << " channel " << chan;

  if ((mod > -1) && (chan > -1) && (ret > -1)) {
    if ((force == LiveStatus::online) && (live_ == force))
      read_chan(ret, mod, chan);
    return channel_parameter_values_[ret + mod * N_CHANNEL_PAR *
        NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR];
  } else
    return -1;
}

void Settings::get_chan_all(Channel channel, Module module) {
  if (live_ == LiveStatus::history)
    return;
  
  if (module == Module::current)
    module = current_module_;
  if (channel == Channel::current)
    channel = current_channel_;

  int mod = static_cast<int>(module);
  int chan = static_cast<int>(channel);

  PL_TRC << "Getting all parameters for module " << mod << " channel " << chan;

  for (int i=0; i < N_CHANNEL_PAR; ++i)
    read_chan(i, mod, chan);
}

void Settings::get_chan_stats(Module module) {
  if (live_ == LiveStatus::history)
    return;
  
  if (module == Module::current)
    module = current_module_;

  int mod = static_cast<int>(module);

  PL_TRC << "Getting channel run statistics for module " << mod;

  for (int i=0; i < NUMBER_OF_CHANNELS; ++i)
    get_chan_all(Channel(i), module);
}

int16_t Settings::i_sys(std::string setting) const {
  uint16_t ret = -1;
  for (int i=0; i < N_SYSTEM_PAR; ++i)
    if (sys_set_meta_[i].name == setting)
      ret = i;
  return ret;
}

int16_t Settings::i_mod(std::string setting) const {
  uint16_t ret = -1;
  for (int i=0; i < N_MODULE_PAR; ++i)
    if (mod_set_meta_[i].name == setting)
      ret = i;
  return ret;
}

int16_t Settings::i_chan(std::string setting) const {
  uint16_t ret = -1;
  for (int i=0; i < N_CHANNEL_PAR; ++i)
    if (chan_set_meta_[i].name == setting)
      ret = i;
  return ret;
}


bool Settings::write_sys(int) {
  return true;
}

bool Settings::write_mod(int, uint8_t) {
  return true;
}

bool Settings::write_chan(int idx, uint8_t mod, uint8_t chan) {
  PL_INFO << "write chan";
  if (live_ == LiveStatus::online)
      VME_write_16(udev, AM_MADC, MADC_ADDR(chan_set_meta_[idx].address | (chan << 1)),
                   channel_parameter_values_[idx + mod * N_CHANNEL_PAR *
                           NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR]);
  return true;
}

bool Settings::read_sys(int) {
  return true;
}

bool Settings::read_mod(int, uint8_t) {
  return true;
}

bool Settings::read_chan(int idx, uint8_t mod, uint8_t chan) {
  PL_DBG << "read chn try";

  long data = 0;
  if (live_ == LiveStatus::online) {
    PL_DBG << "read chn do";
    VME_read_16(udev, AM_MADC, MADC_ADDR(chan_set_meta_[idx].address | (chan << 1)), &data);
  }

  channel_parameter_values_[idx + mod * N_CHANNEL_PAR *
            NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR] = data;
  return true;
}


void Settings::initialize() {
  sys_set_meta_[0].name = "sys1";

  mod_set_meta_[0].name = "mod1";

  chan_set_meta_[0].name = "threshold";
  chan_set_meta_[0].unit = "channels";
  chan_set_meta_[0].writable = true;  //move this to ctor
  chan_set_meta_[0].address = MADC_THRESH_CH0;
}

void Settings::to_xml(tinyxml2::XMLPrinter& printer) {
  printer.OpenElement("PixieSettings");
  printer.OpenElement("System");
  for (int i=0; i < N_SYSTEM_PAR; i++) {
    if (!sys_set_meta_[i].name.empty()) {  //use metadata structure!!!!
      printer.OpenElement("Setting");
      printer.PushAttribute("key", std::to_string(i).c_str());
      printer.PushAttribute("name", sys_set_meta_[i].name.c_str()); //not here?
      printer.PushAttribute("value", std::to_string(system_parameter_values_[i]).c_str());
      printer.CloseElement();
    }
  }
  printer.CloseElement(); //System
  for (int i=0; i < 1; i++) { //hardcoded. Make for multiple modules...
    printer.OpenElement("Module");
    printer.PushAttribute("number", std::to_string(i).c_str());
    for (int j=0; j < N_MODULE_PAR; j++) {
      if (!mod_set_meta_[j].name.empty()) {
        printer.OpenElement("Setting");
        printer.PushAttribute("key", std::to_string(j).c_str());
        printer.PushAttribute("name", mod_set_meta_[j].name.c_str());
        printer.PushAttribute("value", std::to_string(module_parameter_values_[j]).c_str());
        printer.CloseElement();
      }
    }
    for (int j=0; j<num_chans_; j++) {
      printer.OpenElement("Channel");
      printer.PushAttribute("number", std::to_string(j).c_str());
      detectors_[j].to_xml(printer);
      for (int k=0; k < N_CHANNEL_PAR; k++) {
        if (!chan_set_meta_[k].name.empty()) {
          printer.OpenElement("Setting");
          printer.PushAttribute("key", std::to_string(k).c_str());
          printer.PushAttribute("name", chan_set_meta_[k].name.c_str());
          printer.PushAttribute("value",
                                std::to_string(get_chan(k,Channel(j))).c_str());
          printer.CloseElement();
        }
      }
      printer.CloseElement(); //Channel
    }
    printer.CloseElement(); //Module
  }
  printer.CloseElement(); //Settings
}

void Settings::from_xml(tinyxml2::XMLElement* root) {
  live_ = LiveStatus::history;
  detectors_.resize(num_chans_); //hardcoded

  tinyxml2::XMLElement* TopElement = root->FirstChildElement();
  while (TopElement != nullptr) {
    std::string topElementName(TopElement->Name());
    if (topElementName == "System") {
      tinyxml2::XMLElement* SysSetting = TopElement->FirstChildElement("Setting");
      while (SysSetting != nullptr) {
        int thisKey = boost::lexical_cast<short>(SysSetting->Attribute("key"));
        double thisVal = boost::lexical_cast<double>(SysSetting->Attribute("value"));
        system_parameter_values_[thisKey] = thisVal;
        SysSetting = dynamic_cast<tinyxml2::XMLElement*>(SysSetting->NextSibling());
      }
    }
    if (topElementName == "Module") {
      int thisModule = boost::lexical_cast<short>(TopElement->Attribute("number"));
      tinyxml2::XMLElement* ModElement = TopElement->FirstChildElement();
      while (ModElement != nullptr) {
        std::string modElementName(ModElement->Name());
        if (modElementName == "Setting") {
          int thisKey =  boost::lexical_cast<short>(ModElement->Attribute("key"));
          double thisVal = boost::lexical_cast<double>(ModElement->Attribute("value"));
          module_parameter_values_[thisModule * N_MODULE_PAR + thisKey] = thisVal;
        } else if (modElementName == "Channel") {
          int thisChan = boost::lexical_cast<short>(ModElement->Attribute("number"));
          tinyxml2::XMLElement* ChanElement = ModElement->FirstChildElement();
          while (ChanElement != nullptr) {
            if (std::string(ChanElement->Name()) == "Detector") {
              detectors_[thisChan].from_xml(ChanElement);
            } else if (std::string(ChanElement->Name()) == "Setting") {
              int thisKey =  boost::lexical_cast<short>(ChanElement->Attribute("key"));
              double thisVal = boost::lexical_cast<double>(ChanElement->Attribute("value"));
              std::string thisName = std::string(ChanElement->Attribute("name"));
              channel_parameter_values_[thisModule * N_CHANNEL_PAR * NUMBER_OF_CHANNELS
                  + thisChan * N_CHANNEL_PAR + thisKey] = thisVal;
              chan_set_meta_[thisKey].name = thisName;
            }
            ChanElement = dynamic_cast<tinyxml2::XMLElement*>(ChanElement->NextSibling());
          }
        }
        ModElement = dynamic_cast<tinyxml2::XMLElement*>(ModElement->NextSibling());
      }
    }
    TopElement = dynamic_cast<tinyxml2::XMLElement*>(TopElement->NextSibling());
  }
}


}
