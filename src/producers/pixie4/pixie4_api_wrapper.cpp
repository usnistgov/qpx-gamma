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
 ******************************************************************************/

#include "pixie4_api_wrapper.h"
#include "custom_logger.h"
#include <bitset>

//XIA stuff:
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "PlxTypes.h"
#include "Plx.h"
#include "globals.h"
#include "sharedfiles.h"
#include "utilities.h"

namespace Qpx {

Pixie4Wrapper::Pixie4Wrapper()
{
  system_parameter_values_.resize(N_SYSTEM_PAR, 0.0);
  module_parameter_values_.resize(PRESET_MAX_MODULES*N_MODULE_PAR, 0.0);
  channel_parameter_values_.resize(PRESET_MAX_MODULES*N_CHANNEL_PAR*NUMBER_OF_CHANNELS, 0.0);
}

void Pixie4Wrapper::set_num_modules(uint16_t nmod)
{
  num_modules_ = nmod;
  set_sys("NUMBER_MODULES", nmod);
}

void Pixie4Wrapper::get_all_settings()
{
  //  if (status_ & ProducerStatus::booted)
  get_sys_all();
  for (uint16_t i=0; i< num_modules_; ++i)
  {
    get_mod_all(i);
    get_chan_all(i);
  }
}

void Pixie4Wrapper::get_all_stats()
{
  //  if (status_ & ProducerStatus::booted)
  for (uint16_t i=0; i< num_modules_; ++i)
  {
    get_mod_stats(i);
    get_chan_stats(i);
  }
}

void Pixie4Wrapper::get_all_settings(std::set<uint16_t> mods)
{
  for (auto m : mods)
  {
    get_mod_all(m);
    get_chan_all(m);
  }
}

void Pixie4Wrapper::get_all_stats(std::set<uint16_t> mods)
{
  for (auto m : mods)
  {
    get_mod_stats(m);
    get_chan_stats(m);
  }
}

void Pixie4Wrapper::reset_counters_next_run()
{
  for (size_t i=0; i < num_modules_; ++i)
  {
    set_mod(i, "SYNCH_WAIT", 1);
    set_mod(i, "IN_SYNCH", 0);
  }
}

bool Pixie4Wrapper::boot(const std::vector<std::string> &boot_files)
{
  if (boot_files.size() != 7)
  {
    ERR << "<Pixie4> boot_files.size() != 7";
    return false;
  }

  if (!get_sys("NUMBER_MODULES"))
  {
    ERR << "<Pixie4> No valid module slots.";
    return false;
  }

  set_sys("OFFLINE_ANALYSIS", 0);  //live boot
  set_sys("AUTO_PROCESSLMDATA", 0);  //do not write XIA datafile

  //  else
  //  {
  //    //DBG << "Number of modules to boot: " << max;
  //    PixieAPI.read_sys("SLOT_WAVE");
  //    for (int i=0; i < get_sys("NUMBER_MODULES"); i++)
  //      DBG << "<Pixie4> Booting module " << i << " in slot "
  //          << PixieAPI.system_parameter_values_[PixieAPI.i_sys("SLOT_WAVE") + i];
  //  }

  for (int i=0; i < 7; i++)
  {
    memcpy(Boot_File_Name_List[i], (S8*)boot_files[i].c_str(), boot_files[i].size());
    Boot_File_Name_List[i][boot_files[i].size()] = '\0';
  }

  return (boot_err(Pixie_Boot_System(0x1F)));
}


//////////////////////////////
//////////////////////////////
/// U T I L I T I E S/////////
//////////////////////////////
//////////////////////////////

bool Pixie4Wrapper::start_run(uint16_t type)
{
  for (size_t i=0; i< num_modules_; ++i)
    if (!start_run(i, type))
      return false;
  return true;
}

bool Pixie4Wrapper::resume_run(uint16_t type)
{
  for (size_t i=0; i< num_modules_; ++i)
    if (!resume_run(i, type))
      return false;
  return true;
}

bool Pixie4Wrapper::stop_run(uint16_t type)
{
  for (size_t i=0; i< num_modules_; ++i)
    if (!stop_run(i, type))
      return false;
  return true;
}

bool Pixie4Wrapper::start_run(uint8_t mod, uint16_t runtype)
{
  uint16_t type = (runtype | 0x1000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret)
  {
  case 0x10:
    return true;
  case -0x11:
    ERR << "Start run failed: Invalid Pixie module number";
    return false;
  case -0x12:
    ERR << "Start run failed. Try rebooting";
    return false;
  default:
    ERR << "Start run failed. Unknown error";
    return false;
  }
}

bool Pixie4Wrapper::resume_run(uint8_t mod, uint16_t runtype)
{
  uint16_t type = (runtype | 0x2000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret)
  {
  case 0x20:
    return true;
  case -0x21:
    ERR << "Resume run failed: Invalid Pixie module number";
    return false;
  case -0x22:
    ERR << "Resume run failed. Try rebooting";
    return false;
  default:
    ERR << "Resume run failed. Unknown error";
    return false;
  }
}

bool Pixie4Wrapper::stop_run(uint8_t mod, uint16_t runtype)
{
  uint16_t type = (runtype | 0x3000);
  int32_t ret = Pixie_Acquire_Data(type, NULL, 0, mod);
  switch (ret)
  {
  case 0x30:
    return true;
  case -0x31:
    ERR << "Stop run failed: Invalid Pixie module number";
    return false;
  case -0x32:
    ERR << "Stop run failed. Try rebooting";
    return false;
  default:
    ERR << "Stop run failed. Unknown error";
    return false;
  }
}


uint32_t Pixie4Wrapper::poll_run(uint8_t mod, uint16_t runtype)
{
  return Pixie_Acquire_Data(uint16_t(runtype | 0x4000), NULL, 0, mod);
}

uint32_t Pixie4Wrapper::poll_run_dbl(uint8_t mod)
{
  return Pixie_Acquire_Data(0x40FF, NULL, 0, mod);
}

bool Pixie4Wrapper::read_EM(uint32_t* data, uint8_t mod)
{
  S32 retval = Pixie_Acquire_Data(0x9003, (U32*)data, NULL, mod);
  switch (retval) {
  case -0x93:
    ERR << "Failure to read list mode section of external memory. Reboot recommended.";
    return false;
  case -0x95:
    ERR << "Invalid external memory I/O request. Check run type.";
    return false;
  case 0x90:
    return true;
  case 0x0:
    return true; //undocumented by XIA, returns this rather than 0x90 upon success
  default:
    ERR << "Unexpected error " << std::hex << retval;
    return false;
  }
}

bool Pixie4Wrapper::write_EM(uint32_t* data, uint8_t mod)
{
  return (Pixie_Acquire_Data(0x9004, (U32*)data, NULL, mod) == 0x90);
}


uint16_t Pixie4Wrapper::i_dsp(const char* setting_name)
{
  return Find_Xact_Match((S8*)setting_name, DSP_Parameter_Names, N_DSP_PAR);
}

//this function taken from XIA's code and modified, original comments remain
bool Pixie4Wrapper::read_EM_double_buffered(uint32_t* data, uint8_t mod)
{
  U16 j;
  U32 Aoffset[2], WordCountPP[2];
  U32 WordCount, NumWordsToRead, CSR;
  U32 dsp_word[2];

  Aoffset[0] = 0;
  Aoffset[1] = LM_DBLBUF_BLOCK_LENGTH;

  Pixie_ReadCSR((U8)mod, &CSR);
  // A read of Pixie's word count register
  // This also indicates to the DSP that a readout has begun
  Pixie_RdWrdCnt((U8)mod, &WordCount);

  // The number of 16-bit words to read is in EMwords or EMwords2
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp("EMWORDS"), MOD_READ, 2, dsp_word);
  WordCountPP[0] = dsp_word[0] * 65536 + dsp_word[1];
  Pixie_IODM((U8)mod, (U16)DATA_MEMORY_ADDRESS + i_dsp("EMWORDS2"), MOD_READ, 2, dsp_word);
  WordCountPP[1] = dsp_word[0] * 65536 + dsp_word[1];

  if(TstBit(CSR_128K_FIRST, (U16)CSR) == 1)
    j=0;
  else		// block at 128K+64K was first
    j=1;

  if  (TstBit(CSR_DATAREADY, (U16)CSR) == 0 )
  {
    // function called after a readout that cleared WCR => run stopped => read other block
    j=1-j;
    WARN << "<Pixie4Wrapper> read_EM_double_buffered: module " << mod <<
            " csr reports both memory blocks full (block " << 1-j << " older). Run paused (or finished).";
  }

  if (WordCountPP[j] >0)
  {
    // Check if it is an odd or even number
    if(fmod(WordCountPP[j], 2.0) == 0.0)
      NumWordsToRead = WordCountPP[j] / 2;
    else
      NumWordsToRead = WordCountPP[j] / 2 + 1;

    if(NumWordsToRead > LIST_MEMORY_LENGTH)
    {
      ERR << "<Pixie4Wrapper> read_EM_double_buffered: invalid word count " << NumWordsToRead;
      return false;
    }

    // Read out the list mode data
    Pixie_IOEM((U8)mod, LIST_MEMORY_ADDRESS+Aoffset[j], MOD_READ, NumWordsToRead, (U32*)data);
  }

  // A second read of Pixie's word count register to clear the DBUF_1FULL bit
  // indicating to the DSP that the read is complete
  Pixie_RdWrdCnt((U8)mod, &WordCount);
  return true;
}

bool Pixie4Wrapper::clear_EM(uint8_t mod)
{
  std::vector<uint32_t> my_data(list_mem_len32, 0);
  return (write_EM(my_data.data(), mod) >= 0);
}


//////////////////////////
/////System Settings//////
//////////////////////////

void Pixie4Wrapper::set_sys(const std::string& setting, double val)
{
  TRC << "Setting " << setting << " to " << val << " for system";
  //check bounds?
  system_parameter_values_[i_sys(setting.c_str())] = val;
  write_sys(setting.c_str());
}

double Pixie4Wrapper::get_sys(const std::string& setting)
{
  return get_sys(i_sys(setting.c_str()));;
}

double Pixie4Wrapper::get_sys(uint16_t idx)
{
  //check bounds?
  //  read_sys(setting.c_str());
  return system_parameter_values_[idx];
}

void Pixie4Wrapper::get_sys_all()
{
  TRC << "Getting all system";
  read_sys("ALL_SYSTEM_PARAMETERS");
}


///////////////////////////
//////Module Settings//////
///////////////////////////

uint16_t Pixie4Wrapper::hard_max_modules()
{
  return N_SYSTEM_PAR - 7;
}

bool Pixie4Wrapper::module_valid(int16_t mod) const
{
  return (num_modules_ && (mod >= 0) && (mod < static_cast<int16_t>(num_modules_)));
}

void Pixie4Wrapper::set_mod(uint16_t mod, const std::string& name, double val)
{
  module_parameter_values_[mod_val_idx(mod, i_mod(name.c_str()))] = val;
  write_mod(name.c_str(), mod);
}

double Pixie4Wrapper::get_mod(uint16_t mod, const std::string& name) const
{
  return get_mod(mod, i_mod(name.c_str()));
}

double Pixie4Wrapper::get_mod(uint16_t mod, uint16_t idx) const
{
  return module_parameter_values_[mod_val_idx(mod, idx)];
}

void Pixie4Wrapper::get_mod_all(uint16_t mod)
{
  read_mod("ALL_MODULE_PARAMETERS", mod);
}

void Pixie4Wrapper::get_mod_stats(uint16_t mod)
{
  read_mod("MODULE_RUN_STATISTICS", mod);
}

void Pixie4Wrapper::get_mod_all(std::set<uint16_t> mods)
{
  for (auto m : mods)
    get_mod_all(m);
}

void Pixie4Wrapper::get_mod_stats(std::set<uint16_t> mods)
{
  for (auto m : mods)
    get_mod_stats(m);
}


////////////////////////////
////////Channels////////////
////////////////////////////

bool Pixie4Wrapper::channel_valid(int16_t chan)
{
  return ((chan >= 0) && (chan < NUMBER_OF_CHANNELS));
}

double Pixie4Wrapper::set_chan(uint16_t mod, uint16_t chan, const std::string& name,
                               double val)
{
  channel_parameter_values_[chan_val_idx(mod, chan, i_chan(name.c_str()))] = val;
  write_chan(name.c_str(), mod, chan);
}

double Pixie4Wrapper::get_chan(uint16_t mod, uint16_t chan, const std::string& name) const
{
  return get_chan(mod, chan, i_chan(name.c_str()));
}

double Pixie4Wrapper::get_chan(uint16_t mod, uint16_t chan, uint16_t idx) const
{
  return channel_parameter_values_[chan_val_idx(mod, chan, idx)];
}

void Pixie4Wrapper::get_chan_all(uint16_t mod, uint16_t chan)
{
  read_chan("ALL_CHANNEL_PARAMETERS", mod, chan);
}

void Pixie4Wrapper::get_chan_stats(uint16_t mod, uint16_t chan)
{
  read_chan("CHANNEL_RUN_STATISTICS", mod, chan);
}

void Pixie4Wrapper::get_chan_all(uint16_t mod)
{
  for (size_t i=0; i < NUMBER_OF_CHANNELS; ++i)
    get_chan_all(mod, i);
}

void Pixie4Wrapper::get_chan_stats(uint16_t mod)
{
  for (size_t i=0; i < NUMBER_OF_CHANNELS; ++i)
    get_chan_stats(mod, i);
}

uint16_t Pixie4Wrapper::i_sys(const char* setting)
{
  return Find_Xact_Match((S8*)setting, System_Parameter_Names, N_SYSTEM_PAR);
}

uint16_t Pixie4Wrapper::i_mod(const char* setting)
{
  return Find_Xact_Match((S8*)setting, Module_Parameter_Names, N_MODULE_PAR);
}

uint16_t Pixie4Wrapper::i_chan(const char* setting)
{
  return Find_Xact_Match((S8*)setting, Channel_Parameter_Names, N_CHANNEL_PAR);
}

uint16_t Pixie4Wrapper::mod_val_idx(uint16_t mod, uint16_t idx)
{
  return mod * N_MODULE_PAR + idx;
}

uint16_t Pixie4Wrapper::chan_val_idx(uint16_t mod, uint16_t chan, uint16_t idx)
{
  return idx + mod * N_CHANNEL_PAR * NUMBER_OF_CHANNELS + chan * N_CHANNEL_PAR;
}

bool Pixie4Wrapper::write_sys(const char* setting)
{
  S8 sysstr[8] = "SYSTEM\0";
  return set_err(Pixie_User_Par_IO(system_parameter_values_.data(),
                                   (S8*) setting, sysstr, MOD_WRITE, 0, 0));
}

bool Pixie4Wrapper::read_sys(const char* setting)
{
  S8 sysstr[8] = "SYSTEM\0";
  return set_err(Pixie_User_Par_IO(system_parameter_values_.data(),
                                   (S8*) setting, sysstr, MOD_READ, 0, 0));
}

bool Pixie4Wrapper::write_mod(const char* setting, uint8_t mod)
{
  S8 modstr[8] = "MODULE\0";
  return set_err(Pixie_User_Par_IO(module_parameter_values_.data(),
                                   (S8*) setting, modstr, MOD_WRITE, mod, 0));
}

bool Pixie4Wrapper::read_mod(const char* setting, uint8_t mod)
{
  S8 modstr[8] = "MODULE\0";
  return set_err(Pixie_User_Par_IO(module_parameter_values_.data(),
                                   (S8*) setting, modstr, MOD_READ, mod, 0));
}

bool Pixie4Wrapper::write_chan(const char* setting, uint8_t mod, uint8_t chan)
{
  S8 chnstr[9] = "CHANNEL\0";
  return set_err(Pixie_User_Par_IO(channel_parameter_values_.data(),
                                   (S8*) setting, chnstr, MOD_WRITE, mod, chan));
}

bool Pixie4Wrapper::read_chan(const char* setting, uint8_t mod, uint8_t chan)
{
  S8 chnstr[9] = "CHANNEL\0";
  return set_err(Pixie_User_Par_IO(channel_parameter_values_.data(),
                                   (S8*) setting, chnstr, MOD_READ, mod, chan));
}

uint32_t* Pixie4Wrapper::control_collect_ADC(uint8_t module)
{
  TRC << "<Pixie4Wrapper> get ADC (oscilloscope) traces";
  ///why is NUMBER_OF_CHANNELS used? Same for multi-module?
  //  if (status_ & ProducerStatus::can_oscil)

  uint32_t* adc_data = new uint32_t[NUMBER_OF_CHANNELS * max_buf_len];
  int32_t retval = Pixie_Acquire_Data(0x0084, (U32*)adc_data, NULL, module);
  if (retval < 0)
  {
    delete[] adc_data;
    control_err(retval);
    return nullptr;
  }
  return adc_data;
}

bool Pixie4Wrapper::control_set_DAC(uint8_t module)
{
  return control_err(Pixie_Acquire_Data(0x0000, NULL, NULL, module));
}

bool Pixie4Wrapper::control_connect(uint8_t module)
{
  return control_err(Pixie_Acquire_Data(0x0001, NULL, NULL, module));
}

bool Pixie4Wrapper::control_disconnect(uint8_t module)
{
  return control_err(Pixie_Acquire_Data(0x0002, NULL, NULL, module));
}

bool Pixie4Wrapper::control_program_Fippi(uint8_t module)
{
  return control_err(Pixie_Acquire_Data(0x0005, NULL, NULL, module));
}

bool Pixie4Wrapper::control_measure_baselines(uint8_t mod)
{
  return control_err(Pixie_Acquire_Data(0x0006, NULL, NULL, mod));
}

bool Pixie4Wrapper::control_measure_baselines()
{
  bool success = false;
  for (size_t i=0; i< num_modules_; ++i)
    if (control_measure_baselines(i))
      success = true;
  return success;
}

bool Pixie4Wrapper::control_test_EM_write(uint8_t module)
{
  TRC << "<Pixie4Wrapper> test EM write";
  return control_err(Pixie_Acquire_Data(0x0016, NULL, NULL, module));
}

bool Pixie4Wrapper::control_test_HM_write(uint8_t module)
{
  TRC << "<Pixie4Wrapper> test HM write";
  return control_err(Pixie_Acquire_Data(0x001A, NULL, NULL, module));
}

bool Pixie4Wrapper::control_compute_BLcut()
{
  TRC << "<Pixie4Wrapper> compute BLcut";
  return control_err(Pixie_Acquire_Data(0x0080, NULL, NULL, 0));
}

bool Pixie4Wrapper::control_find_tau(uint8_t mod)
{
  return control_err(Pixie_Acquire_Data(0x0081, NULL, NULL, mod));
}

bool Pixie4Wrapper::control_find_tau()
{
  bool success = false;
  for (size_t i=0; i< num_modules_; ++i)
    if (control_find_tau(i))
      success = true;
  return success;
}

bool Pixie4Wrapper::control_adjust_offsets(uint8_t mod)
{
  return control_err(Pixie_Acquire_Data(0x0083, NULL, NULL, mod));
}

bool Pixie4Wrapper::control_adjust_offsets()
{
  bool success = false;
  for (size_t i=0; i< num_modules_; ++i)
    if (control_adjust_offsets(i))
      success = true;
  return success;
}

bool Pixie4Wrapper::control_err(int32_t err_val)
{
  switch (err_val)
  {
  case 0:
    return true;
  case -1:
    ERR << "<Pixie4Wrapper> Control command failed: Invalid Pixie modules number. Check ModNum";
    break;
  case -2:
    ERR << "<Pixie4Wrapper> Control command failed: Failure to adjust offsets. Reboot recommended";
    break;
  case -3:
    ERR << "<Pixie4Wrapper> Control command failed: Failure to acquire ADC traces. Reboot recommended";
    break;
  case -4:
    ERR << "<Pixie4Wrapper> Control command failed: Failure to start the control task run. Reboot recommended";
    break;
  default:
    ERR << "<Pixie4Wrapper> Control comman failed: Unknown error " << err_val;
  }
  return false;
}

bool Pixie4Wrapper::set_err(int32_t err_val)
{
  switch (err_val)
  {
  case 0:
    return true;
  case -1:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Null pointer for User_Par_Values";
    break;
  case -2:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Invalid user parameter name";
    break;
  case -3:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Invalid user parameter type";
    break;
  case -4:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Invalid I/O direction";
    break;
  case -5:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Invalid module number";
    break;
  case -6:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Invalid channel number";
    break;
  default:
    ERR << "<Pixie4Wrapper> Set/get parameter failed: Unknown error " << err_val;
  }
  return false;
}

bool Pixie4Wrapper::boot_err(int32_t err_val)
{
  if (err_val >= 0)
    return true;

  switch (err_val)
  {
  case -1:
    ERR << "<Pixie4Wrapper> Boot failed: unable to scan crate slots. Check PXI slot map.";
    break;
  case -2:
    ERR << "<Pixie4Wrapper> Boot failed: unable to read communication FPGA (rev. B). Check comFPGA file.";
    break;
  case -3:
    ERR << "<Pixie4Wrapper> Boot failed: unable to read communication FPGA (rev. C). Check comFPGA file.";
    break;
  case -4:
    ERR << "<Pixie4Wrapper> Boot failed: unable to read signal processing FPGA config. Check SPFPGA file.";
    break;
  case -5:
    ERR << "<Pixie4Wrapper> Boot failed: unable to read DSP executable code. Check DSP code file.";
    break;
  case -6:
    ERR << "<Pixie4Wrapper> Boot failed: unable to read DSP parameter values. Check DSP parameter file.";
    break;
  case -7:
    ERR << "<Pixie4Wrapper> Boot failed: unable to initialize DSP parameter names. Check DSP .var file.";
    break;
  case -8:
    ERR << "<Pixie4Wrapper> Boot failed: failed to boot all modules in the system. Check Pixie modules.";
    break;
  default:
    ERR << "<Pixie4Wrapper> Boot failed, undefined error " << err_val;
  }
  return false;
}

std::set<uint16_t> Pixie4Wrapper::get_triggered_modules() const
{
  std::set<uint16_t> triggered_modules;
  for (size_t i=0; i < num_modules_; ++i)
  {
    auto csr = std::bitset<32>(poll_run_dbl(i));
    if (csr[14])
      triggered_modules.insert(i);
  }
  return triggered_modules;
}

}
