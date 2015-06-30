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
 *      Pixie::Spectrum::Spectrum1D one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/filesystem.hpp>
#include "spectrum1D.h"
#include "xylib.h"

namespace Pixie {
namespace Spectrum {

static Registrar<Spectrum1D> registrar("1D");

uint64_t Spectrum1D::_get_count(std::initializer_list<uint16_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= resolution_)
    return 0;
  else
    return spectrum_[chan];
}

std::unique_ptr<std::list<Entry>> Spectrum1D::_get_spectrum(std::initializer_list<Pair> list) {
  int min, max;
  if (list.size() != 1) {
    min = 0;
    max = resolution_;
  } else {
    Pair range = *list.begin();
    min = range.first;
    max = range.second;
  }

  //in range?
  
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  for (int i=min; i < max; i++) {
    Entry newentry;
    newentry.first.resize(1, 0);
    newentry.first[0] = i;
    newentry.second = spectrum_[i];
    result->push_back(newentry);
  }
  return result;
}

void Spectrum1D::addHit(const Hit& newHit, int chan) {
  uint16_t en = newHit.energy[chan] >> shift_by_;
  spectrum_[en]++;
  if (en > max_chan_)
    max_chan_ = en;
  count_++;
}

void Spectrum1D::addHit(const Hit& newHit) {
  for (int i = 0; i < kNumChans; i++)
    if ((add_pattern_[i]) && (newHit.pattern[i]))
      this->addHit(newHit, i);
}

bool Spectrum1D::_write_file(std::string dir, std::string format) const {
  if (format == "tka") {
    write_tka(dir + "/" + name_ + ".tka");
    return true;
  } else if (format == "n42") {
    write_n42(dir + "/" + name_ + ".n42");
    return true;
  } else
    return false;
}

bool Spectrum1D::_read_file(std::string name, std::string format) {
  if (format == "cnf")
    return read_cnf(name);
  else if (format == "tka")
    return read_tka(name);
  else if (format == "n42")
    return read_n42(name);
  else if (format == "ava")
    return read_ava(name);
  else
    return false;
}

void Spectrum1D::init_from_file(std::string filename) { 
  match_pattern_.resize(kNumChans, 0);
  add_pattern_.resize(kNumChans, 0);
  match_pattern_[0] = 1;
  add_pattern_[0] = 1;
  name_ = boost::filesystem::path(filename).filename().string();
  std::replace( name_.begin(), name_.end(), '.', '_');
  visible_ = true;
  appearance_ = 4278190335;  //randomize?
  initialize();
  recalc_energies();
}

std::string Spectrum1D::_channels_to_xml() const {
  std::stringstream channeldata;

  uint64_t z_count = 0;
  for (uint32_t i = 0; i < resolution_; i++)
    if (spectrum_[i])
      if (z_count == 0)
        channeldata << spectrum_[i] << " ";
      else {
        channeldata << "0 " << z_count << " "
                    << spectrum_[i] << " ";
        z_count = 0;
      }
    else
      z_count++;

  return channeldata.str();
}

uint16_t Spectrum1D::_channels_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  spectrum_.clear();
  spectrum_.resize(resolution_, 0);

  uint64_t i = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "0") {
      channeldata >> numero_z;
      i += boost::lexical_cast<uint64_t>(numero_z);
    } else {
      spectrum_[i] = boost::lexical_cast<uint64_t>(numero);
      i++;
    }
  }
  max_chan_ = i;
  return max_chan_;
}


bool Spectrum1D::channels_from_string(std::istream &data_stream, bool compression) {
  std::list<Entry> entry_list;

  int i = 0;
  std::string numero, numero_z;
  if (compression) {
    while (data_stream.rdbuf()->in_avail()) {
      data_stream >> numero;
      if (numero == "0") {
        data_stream >> numero_z;
        i += boost::lexical_cast<uint64_t>(numero_z);
      } else {
        Entry new_entry;
        new_entry.first.resize(1);
        new_entry.first[0] = i;
        new_entry.second = boost::lexical_cast<uint64_t>(numero);
        entry_list.push_back(new_entry);
        i++;
      }
    }    
  } else {
    while (data_stream.rdbuf()->in_avail()) {
      data_stream >> numero;
      Entry new_entry;
      new_entry.first.resize(1);
      new_entry.first[0] = i;
      new_entry.second = boost::lexical_cast<uint64_t>(numero);
      entry_list.push_back(new_entry);
      i++;
    }
  } 
  
  if (i == 0)
    return false;

  bits_ = log2(i);
  if (pow(2, bits_) < i)
    bits_++;
  resolution_ = pow(2, bits_);
  shift_by_ = 16 - bits_;
  max_chan_ = i;

  spectrum_.clear();
  spectrum_.resize(resolution_, 0);
      
  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    count_ += q.second;    
  }

  return true;
}


bool Spectrum1D::read_cnf(std::string name) {
  PL_INFO << "about to read cnf";
  xylib::DataSet* newdata = xylib::load_file(name, "canberra_cnf");

  if (newdata == nullptr)
    return false;
  
  PL_TRC << "has " << newdata->get_block_count() << " blocks";
  for (int i=0; i < newdata->get_block_count(); i++) {

    std::vector<double> calibration;
    for (uint32_t j=0; j < newdata->get_block(i)->meta.size(); j++) {
      std::string key = newdata->get_block(i)->meta.get_key(j);
      std::string value = newdata->get_block(i)->meta.get(key);
      if (key.substr(0, 12) == "energy calib")
        calibration.push_back(boost::lexical_cast<double>(value));
      if (key == "description")
        description_ = value;
      if (key == "date and time") {
        std::stringstream iss;
        iss.str(value);
        std::string week;
        iss >> week;
        boost::posix_time::time_input_facet
            *tif(new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));
        iss.imbue(std::locale(std::locale::classic(), tif));
        iss >> start_time_;
      }
      if (key == "real time (s)")
        real_time_ = boost::posix_time::duration_from_string(value);
      if (key == "live time (s)")
        live_time_ = boost::posix_time::duration_from_string(value);
    }
     
    PL_INFO << "block " << i
            << " has " << newdata->get_block(i)->get_column_count() << " columns"
            << " and " << newdata->get_block(i)->get_point_count() << " points";

    resolution_ = newdata->get_block(i)->get_point_count();
    bits_ = log2(resolution_);
    if (pow(2, bits_) < resolution_)
      bits_++;
    //resolution_ = pow(2, bits_);
    spectrum_.resize(resolution_, 0);
    shift_by_ = 16 - bits_;

    detectors_.resize(kNumChans);
    detectors_[0] = Gamma::Detector();
    detectors_[0].name_ = "default";
    Gamma::Calibration new_calib("Energy", bits_);
    new_calib.coefficients_ = calibration;
    detectors_[0].energy_calibrations_.add(new_calib);
    
    double tempcount = 0.0;
    for (int j = 0; j < newdata->get_block(i)->get_column_count(); j++) {
      //std::stringstream blab;
      for (int k = 0; k < newdata->get_block(i)->get_point_count(); k++) {
        double data = newdata->get_block(i)->get_column(j).get_value(k);

        spectrum_[k] = static_cast<uint64_t>(round(data));
        //blab << data << " ";
        tempcount += data;
      }
      //PL_DBG << "column " << blab.str();
    }
    count_ = tempcount;
    //PL_INFO << "total count " << tempcount;

  }

  init_from_file(name);
  delete newdata;  
    
  return true;
}

bool Spectrum1D::read_tka(std::string name) {
  using namespace boost::algorithm;
  std::ifstream myfile(name, std::ios::in);
  std::string data;
  double timed;
  
  myfile >> data;
  timed = boost::lexical_cast<double>(trim_copy(data)) * 1000.0;
  live_time_ = boost::posix_time::milliseconds(timed);
  myfile >> data;
  timed = boost::lexical_cast<double>(trim_copy(data)) * 1000.0;
  real_time_ = boost::posix_time::milliseconds(timed);
  int i = 0;

  if(!this->channels_from_string(myfile, false))
    return false;

  detectors_.resize(kNumChans);
  detectors_[0] = Gamma::Detector();
  detectors_[0].name_ = "default";
  
  init_from_file(name);

  return true;
}

bool Spectrum1D::read_n42(std::string name) {
  FILE* myfile;
  myfile = fopen (name.c_str(), "r");
  if (myfile == nullptr)
    return false;

  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;
  
  tinyxml2::XMLDocument docx;
  docx.LoadFile(myfile);

  tinyxml2::XMLElement* root;
  tinyxml2::XMLElement* branch;
  
  if ((root = docx.FirstChildElement("N42InstrumentData")) == nullptr) {
    fclose(myfile);
    return false;
  }
  
  if ((root = root->FirstChildElement("Measurement")) == nullptr) {
    fclose(myfile);
    return false;
  }
  
  if ((root = root->FirstChildElement("Spectrum")) == nullptr) {
    fclose(myfile);
    return false;
  }
  
  branch = root->FirstChildElement("StartTime");
  if (branch != nullptr) {
    iss << branch->GetText();
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> start_time_;
  }

  branch = root->FirstChildElement("RealTime");
  if (branch != nullptr) {
    std::string rt_str = std::string(branch->GetText());
    boost::algorithm::trim(rt_str);
    if (rt_str.size() > 3)
      rt_str = rt_str.substr(2, rt_str.size()-3); //to trim PTnnnS to nnn
    double rt_ms = boost::lexical_cast<double>(rt_str) * 1000.0;
    real_time_ = boost::posix_time::milliseconds(rt_ms);
  }

  branch = root->FirstChildElement("LiveTime");
  if (branch != nullptr) {
    std::string lt_str = std::string(branch->GetText());
    boost::algorithm::trim(lt_str);
    if (lt_str.size() > 3)
      lt_str = lt_str.substr(2, lt_str.size()-3); //to trim PTnnnS to nnn
    double lt_ms = boost::lexical_cast<double>(lt_str) * 1000.0;
    live_time_ = boost::posix_time::milliseconds(lt_ms);
  }

  if ((branch = root->FirstChildElement("ChannelData")) == nullptr) {
    fclose(myfile);
    return false;
  }

  std::string this_data = branch->GetText();
  
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);

  if (this->channels_from_string(channeldata, true)) {//assume compressed

    Gamma::Detector newdet;
    newdet.name_ = "default";
    if (root->Attribute("Detector"))
      newdet.name_ = std::string(root->Attribute("Detector"));
  
    branch = root->FirstChildElement("DetectorType");
    if (branch != nullptr)
      newdet.type_ = std::string(branch->GetText());

    branch = root->FirstChildElement("Calibration");
    if (branch != nullptr) {
      Gamma::Calibration newcalib("Energy", bits_);
      newcalib.from_xml(branch);
      newdet.energy_calibrations_.add(newcalib);
    }

    detectors_.resize(kNumChans);
    detectors_[0] = newdet;

    fclose(myfile);

    init_from_file(name);
    
    return true;
  } else {
    fclose(myfile);
    return false;
  }
  
}

bool Spectrum1D::read_ava(std::string name) {
  tinyxml2::XMLElement* root;
  tinyxml2::XMLElement* branch;
  
  FILE* myfile;
  myfile = fopen (name.c_str(), "r");
  if (myfile == nullptr)
    return false;

  tinyxml2::XMLDocument docx;
  docx.LoadFile(myfile);

  if ((root = docx.FirstChildElement("mcadata")) == nullptr) {
    fclose(myfile);
    return false;
  }

  if ((branch = root->FirstChildElement("spectrum")) == nullptr) {
    fclose(myfile);
    return false;
  }
  
  std::string StartData(branch->Attribute("start"));
  boost::algorithm::trim(StartData);
  if (!StartData.empty()) {
    std::stringstream oss, iss;
    oss.str(StartData);
    std::string week, month, day, time, zone, year;
    oss >> week >> month >> day >> time >> zone >> year;
    if (day.size() == 1)
      day = "0" + day; 
    std::string inp = year + "-" + month + "-" + day + " " + time;
    iss.str(inp);  
    iss >> start_time_;
  }

  std::string RealTime(branch->Attribute("elapsed_real")); boost::algorithm::trim(RealTime);
  if (!RealTime.empty()) {
    double rt_ms = boost::lexical_cast<double>(RealTime) * 1000.0;
    real_time_ = boost::posix_time::milliseconds(rt_ms);
  }

  std::string LiveTime(branch->Attribute("elapsed_live")); boost::algorithm::trim(LiveTime);
  if (!LiveTime.empty()) {
    double lt_ms = boost::lexical_cast<double>(LiveTime) * 1000.0;
    live_time_ = boost::posix_time::milliseconds(lt_ms);
  }

  std::string this_data = branch->GetText();

  std::replace( this_data.begin(), this_data.end(), ',', ' ');
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);
  if(!this->channels_from_string(channeldata, false)) {
    fclose(myfile);
    return false;
  }

  branch = root->FirstChildElement("calibration_details");
  while ((branch != nullptr) && (std::string(branch->Attribute("type")) != "energy"))
    branch = dynamic_cast<tinyxml2::XMLElement*>(branch->NextSibling());

  Gamma::Detector newdet; Gamma::Calibration newcalib("Energy", bits_);
  if ((branch != nullptr) &&
      ((branch = branch->FirstChildElement("model")) != nullptr)) {
    if (std::string(branch->Attribute("type")) == "polynomial")
      newcalib.model_ = Gamma::CalibrationModel::polynomial;
    std::vector<double> encalib;
    branch = branch->FirstChildElement("coefficient");
    while ((branch != nullptr) && (branch->Attribute("value")) != nullptr) {
      std::string coefvalstr =
          boost::algorithm::trim_copy(std::string(branch->Attribute("value")));
      encalib.push_back(boost::lexical_cast<double>(coefvalstr));
      branch = dynamic_cast<tinyxml2::XMLElement*>(branch->NextSibling());
    }
    newcalib.coefficients_ = encalib;
    newcalib.units_ = "keV";
    newcalib.type_ = "Energy";
    newdet.energy_calibrations_.add(newcalib);
    newdet.name_ = "default";
  }
  detectors_.resize(kNumChans);
  detectors_[0] = newdet;

  fclose(myfile);
  
 
  init_from_file(name);
  return true;
}

void Spectrum1D::write_tka(std::string name) const {
  uint32_t range = (resolution_ - 2);
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  //  myfile.precision(2);
  //  myfile << std::fixed;
  myfile << (live_time_.total_milliseconds() * 0.001) << std::endl
         << (real_time_.total_milliseconds() * 0.001) << std::endl;
      
  for (uint32_t i = 0; i < range; i++)
    myfile << spectrum_[i] << std::endl;
  myfile.close();
}

void Spectrum1D::write_n42(std::string name) const {
  FILE* myfile;
  myfile = fopen (name.c_str(), "w");
  if (myfile == nullptr)
    return;
 
  tinyxml2::XMLDocument newDoc;
  tinyxml2::XMLPrinter printer(myfile);
  printer.PushDeclaration(newDoc.NewDeclaration()->Value());

  std::stringstream durationdata;
  int myDetector = -1;
  for (int i = 0; i < kNumChans; i++)
    if (add_pattern_[i] == 1)
      myDetector = i;
  Gamma::Calibration myCalibration;
  if (myDetector != -1) {
    if (detectors_[myDetector].energy_calibrations_.has_a(Gamma::Calibration("Energy", bits_)))
      myCalibration = detectors_[myDetector].energy_calibrations_.get(Gamma::Calibration("Energy", bits_));
    else if ((detectors_[myDetector].energy_calibrations_.size() == 1) &&
             (detectors_[myDetector].energy_calibrations_.get(0).units_ != "channels"))
      myCalibration = detectors_[myDetector].energy_calibrations_.get(0);
  }
  
  printer.OpenElement("N42InstrumentData");
  printer.PushAttribute("xmlns",
                        "http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242");
  printer.PushAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
  printer.PushAttribute("xsi:schemaLocation",
                        "http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242 http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242.xsd");
  
  printer.OpenElement("Measurement");
  printer.OpenElement("Spectrum");
  printer.PushAttribute("Type","PHA");
  if ((myDetector > -1) &&
      (myCalibration.units_ != "channels")) {
    printer.PushAttribute("Detector", detectors_[myDetector].name_.c_str());
    printer.OpenElement("DetectorType");
    printer.PushText(detectors_[myDetector].type_.c_str());
    printer.CloseElement();
  }

  
  printer.OpenElement("StartTime");
  durationdata << to_iso_extended_string(start_time_) << "-5:00"; //fix this hack
  printer.PushText(durationdata.str().c_str());
  printer.CloseElement();
      
  durationdata.str(std::string()); //clear it
  printer.OpenElement("RealTime");
  durationdata << "PT" << (real_time_.total_milliseconds() * 0.001) << "S";
  printer.PushText(durationdata.str().c_str());
  printer.CloseElement();

  durationdata.str(std::string()); //clear it
  printer.OpenElement("LiveTime");
  durationdata << "PT" << (live_time_.total_milliseconds() * 0.001) << "S";
  printer.PushText(durationdata.str().c_str());
  printer.CloseElement();

  if ((myDetector > -1) &&
      (myCalibration.units_ != "channels"))
    myCalibration.to_xml(printer);    

  printer.OpenElement("ChannelData");
  printer.PushAttribute("Compression", "CountedZeroes");
  if ((resolution_ > 0) && (count_ > 0))
    printer.PushText(this->_channels_to_xml().c_str());
  printer.CloseElement();

  printer.CloseElement(); //Spectrum
  printer.CloseElement(); //Measurement
  printer.CloseElement(); //N42InstrumentData

  fclose(myfile);
}

}}
