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
 *      Qpx::Spectrum::Spectrum1D one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/filesystem.hpp>
#include "spectrum1D.h"
#include "xylib.h"

namespace Qpx {
namespace Spectrum {

static Registrar<Spectrum1D> registrar("1D");

void Spectrum1D::_set_detectors(const std::vector<Gamma::Detector>& dets) {
  metadata_.detectors.clear();

  if (dets.size() == 1)
    metadata_.detectors = dets;
  if (dets.size() >= 1) {
    int total = metadata_.add_pattern.size();
    if (dets.size() < total)
      total = dets.size();
    metadata_.detectors.resize(1, Gamma::Detector());

    for (int i=0; i < total; ++i) {
      if (metadata_.add_pattern[i]) {
        metadata_.detectors[0] = dets[i];
      }
    }
  }

  this->recalc_energies();
}

PreciseFloat Spectrum1D::_get_count(std::initializer_list<uint16_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= metadata_.resolution)
    return 0;
  else
    return spectrum_[chan];
}

std::unique_ptr<std::list<Entry>> Spectrum1D::_get_spectrum(std::initializer_list<Pair> list) {
  int min, max;
  if (list.size() != 1) {
    min = 0;
    max = metadata_.resolution;
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

void Spectrum1D::_add_bulk(const Entry& e) {
  int sz = metadata_.add_pattern.size();
  if (e.first.size() < sz)
    sz = e.first.size();
  for (int i = 0; i < sz; ++i)
    if (metadata_.add_pattern[i] && (e.first[i] < spectrum_.size())) {
      PreciseFloat nr = (spectrum_[e.first[i]] += e.second);

      if (nr > metadata_.max_count)
        metadata_.max_count = nr;

      metadata_.total_count += e.second;
    }
}

void Spectrum1D::addHit(const Hit& newHit) {
  uint16_t en = newHit.energy >> shift_by_;
  PreciseFloat nr = ++spectrum_[en];
  if (nr > metadata_.max_count)
    metadata_.max_count = nr;

  if (en > metadata_.max_chan)
    metadata_.max_chan = en;
}

void Spectrum1D::addEvent(const Event& newEvent) {
  for (int16_t i = 0; i < metadata_.add_pattern.size(); i++)
    if ((metadata_.add_pattern[i]) && (newEvent.hit.count(i) > 0))
      this->addHit(newEvent.hit.at(i));
}

bool Spectrum1D::_write_file(std::string dir, std::string format) const {
  if (format == "tka") {
    write_tka(dir + "/" + metadata_.name + ".tka");
    return true;
  } else if (format == "n42") {
    write_n42(dir + "/" + metadata_.name + ".n42");
    return true;
  } else
    return false;
}

bool Spectrum1D::_read_file(std::string name, std::string format) {
  metadata_.attributes = get_template().generic_attributes;

  if (format == "cnf")
    return read_cnf(name);
  else if (format == "tka")
    return read_tka(name);
  else if (format == "n42")
    return read_n42(name);
  else if (format == "ava")
    return read_ava(name);
  else if (format == "spe")
    return read_spe(name);
  else
    return false;
}

void Spectrum1D::init_from_file(std::string filename) { 
  metadata_.match_pattern.resize(1, 1);
  metadata_.add_pattern.resize(1, 1);
  metadata_.match_pattern[0] = 1;
  metadata_.add_pattern[0] = 1;
  metadata_.name = boost::filesystem::path(filename).filename().string();
  std::replace( metadata_.name.begin(), metadata_.name.end(), '.', '_');
  metadata_.visible = true;
  metadata_.appearance = 4278190335;  //randomize?

  initialize();
  recalc_energies();
}

std::string Spectrum1D::_channels_to_xml() const {
  std::stringstream channeldata;

  PreciseFloat z_count = 0;
  for (uint32_t i = 0; i < metadata_.resolution; i++)
    if (spectrum_[i])
      if (z_count == 0)
        channeldata << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";
      else {
        channeldata << "0 " << z_count << " "
                    << std::setprecision(std::numeric_limits<PreciseFloat>::max_digits10) << spectrum_[i] << " ";
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
  spectrum_.resize(metadata_.resolution, 0);

  metadata_.max_count = 0;

  uint16_t i = 0;
  std::string numero, numero_z;
  while (channeldata.rdbuf()->in_avail()) {
    channeldata >> numero;
    if (numero == "0") {
      channeldata >> numero_z;
      i += boost::lexical_cast<uint16_t>(numero_z);
    } else {
      PreciseFloat nr(numero);
      spectrum_[i] = nr;
      if (nr > metadata_.max_count)
        metadata_.max_count = nr;
      i++;
    }
  }
  metadata_.max_chan = i;
  return metadata_.max_chan;
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
        i += boost::lexical_cast<uint16_t>(numero_z);
      } else {
        Entry new_entry;
        new_entry.first.resize(1);
        new_entry.first[0] = i;
        PreciseFloat nr(numero);
        new_entry.second = nr;
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
      PreciseFloat nr(numero);
      new_entry.second = nr;
      entry_list.push_back(new_entry);
      i++;
    }
  } 
  
  if (i == 0)
    return false;

  metadata_.bits = log2(i);
  if (pow(2, metadata_.bits) < i)
    metadata_.bits++;
  metadata_.resolution = pow(2, metadata_.bits);
  shift_by_ = 16 - metadata_.bits;
  metadata_.max_chan = i;

  spectrum_.clear();
  spectrum_.resize(metadata_.resolution, 0);
  metadata_.max_count = 0;
      
  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    metadata_.total_count += q.second;
    if (q.second > metadata_.max_count)
      metadata_.max_count = q.second;

  }

  return true;
}


bool Spectrum1D::read_cnf(std::string name) {
  PL_INFO << "about to read cnf";
  xylib::DataSet* newdata = xylib::load_file(name, "canberra_cnf");

  if (newdata == nullptr)
    return false;
  
  PL_DBG << "has " << newdata->get_block_count() << " blocks";
  for (int i=0; i < newdata->get_block_count(); i++) {

    std::vector<double> calibration;
    for (uint32_t j=0; j < newdata->get_block(i)->meta.size(); j++) {
      std::string key = newdata->get_block(i)->meta.get_key(j);
      std::string value = newdata->get_block(i)->meta.get(key);
      if (key.substr(0, 12) == "energy calib")
        calibration.push_back(boost::lexical_cast<double>(value));
      if (key == "description")
        metadata_.description = value;
      if (key == "date and time") {
        std::stringstream iss;
        iss.str(value);
        std::string week;
        iss >> week;
        boost::posix_time::time_input_facet
            *tif(new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));
        iss.imbue(std::locale(std::locale::classic(), tif));
        iss >> metadata_.start_time;
      }
      if (key == "real time (s)")
        metadata_.real_time = boost::posix_time::duration_from_string(value);
      if (key == "live time (s)")
        metadata_.live_time = boost::posix_time::duration_from_string(value);
    }
     
    PL_DBG << "block " << i
           << " has " << newdata->get_block(i)->get_column_count() << " columns"
           << " and " << newdata->get_block(i)->get_point_count() << " points";

    metadata_.resolution = newdata->get_block(i)->get_point_count();
    metadata_.bits = log2(metadata_.resolution);
    if (pow(2, metadata_.bits) < metadata_.resolution)
      metadata_.bits++;
    //metadata_.resolution = pow(2, metadata_.bits);
    spectrum_.resize(metadata_.resolution, 0);
    shift_by_ = 16 - metadata_.bits;
    metadata_.max_count = 0;

    metadata_.detectors.resize(1);
    metadata_.detectors[0] = Gamma::Detector();
    metadata_.detectors[0].name_ = "unknown";
    Gamma::Calibration new_calib("Energy", metadata_.bits);
    new_calib.coefficients_ = calibration;
    metadata_.detectors[0].energy_calibrations_.add(new_calib);
    
    double tempcount = 0.0;
    for (int j = 0; j < newdata->get_block(i)->get_column_count(); j++) {
      std::stringstream blab;
      for (int k = 0; k < newdata->get_block(i)->get_point_count(); k++) {
        double data = newdata->get_block(i)->get_column(j).get_value(k);
        PreciseFloat nr(data);

        spectrum_[k] = PreciseFloat(data);

        if (nr > metadata_.max_count)
          metadata_.max_count = nr;

        blab << data << " ";
        tempcount += data;
      }
      PL_DBG << "column " << blab.str();
    }
    metadata_.total_count = tempcount;
    PL_DBG << "total count " << tempcount;

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
  metadata_.live_time = boost::posix_time::milliseconds(timed);
  myfile >> data;
  timed = boost::lexical_cast<double>(trim_copy(data)) * 1000.0;
  metadata_.real_time = boost::posix_time::milliseconds(timed);
  int i = 0;

  if(!this->channels_from_string(myfile, false))
    return false;

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = Gamma::Detector();
  metadata_.detectors[0].name_ = "unknown";
  
  init_from_file(name);

  return true;
}

bool Spectrum1D::read_spe(std::string name) {
  //radware format
  std::ifstream myfile(name, std::ios::in | std::ios::binary);
  if (!myfile)
    return false;

  myfile.seekg (0, myfile.end);
  int length = myfile.tellg();

  if (length < 13)
    return false;

  myfile.seekg (12, myfile.beg);

  spectrum_.clear();
  metadata_.total_count = 0;
//  metadata_.max_chan = 0;
//  uint16_t max_i =0;

  std::list<Entry> entry_list;
  float one;
  int i=0;
  while (myfile.tellg() != length) {
    myfile.read ((char*)&one, sizeof(float));
    Entry new_entry;
    new_entry.first.resize(1);
    new_entry.first[0] = i;
    new_entry.second = one;
    entry_list.push_back(new_entry);
    i++;
    metadata_.total_count += one;
  }

  if (i == 0)
    return false;

  metadata_.bits = log2(i);
  if (pow(2, metadata_.bits) < i)
    metadata_.bits++;
  metadata_.resolution = pow(2, metadata_.bits);
  shift_by_ = 16 - metadata_.bits;
  metadata_.max_chan = i;

  spectrum_.clear();
  spectrum_.resize(metadata_.resolution, 0);

  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    metadata_.total_count += q.second;
  }

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = Gamma::Detector();
  metadata_.detectors[0].name_ = "unknown";

  init_from_file(name);

  return true;
}

bool Spectrum1D::read_n42(std::string filename) {
  pugi::xml_document doc;

  if (!doc.load_file(filename.c_str()))
    return false;

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "N42InstrumentData"))
    return false;

  pugi::xml_node meas_node = root.child("Measurement");
  if (!meas_node)
    return false;

  //will only read out first spectrum in N42
  pugi::xml_node node = meas_node.child("Spectrum");
  if (!node)
    return false;

  boost::posix_time::time_input_facet *tif = new boost::posix_time::time_input_facet;
  tif->set_iso_extended_format();
  std::stringstream iss;

  std::string time(node.child_value("StartTime"));
  if (!time.empty()) {
    iss << time;
    iss.imbue(std::locale(std::locale::classic(), tif));
    iss >> metadata_.start_time;
  }

  time = std::string(node.child_value("RealTime"));
  if (!time.empty()) {
    boost::algorithm::trim(time);
    if (time.size() > 3)
      time = time.substr(2, time.size()-3); //to trim PTnnnS to nnn
    double rt_ms = boost::lexical_cast<double>(time) * 1000.0;
    metadata_.real_time = boost::posix_time::milliseconds(rt_ms);
  }

  time = std::string(node.child_value("LiveTime"));
  if (!time.empty()) {
    boost::algorithm::trim(time);
    if (time.size() > 3)
      time = time.substr(2, time.size()-3); //to trim PTnnnS to nnn
    double lt_ms = boost::lexical_cast<double>(time) * 1000.0;
    metadata_.live_time = boost::posix_time::milliseconds(lt_ms);
  }

  std::string this_data(node.child_value("ChannelData"));
  
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);

  if (!this->channels_from_string(channeldata, true)) //assume compressed
    return false;

  Gamma::Detector newdet;
  newdet.name_ = "unknown";
  if (node.attribute("Detector"))
    newdet.name_ = std::string(node.attribute("Detector").value());
  
  newdet.type_ = std::string(node.child_value("DetectorType"));

  //have more claibrations? fwhm,etc..
  if (node.child("Calibration")) {
    Gamma::Calibration newcalib;
    newcalib.from_xml(node.child("Calibration"));
    newcalib.bits_ = metadata_.bits;
    newdet.energy_calibrations_.add(newcalib);
  }

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = newdet;

  init_from_file(filename);

  return true;

}

bool Spectrum1D::read_ava(std::string filename) {
  pugi::xml_document doc;

  if (!doc.load_file(filename.c_str()))
    return false;

  pugi::xml_node root = doc.first_child();
  if (!root || (std::string(root.name()) != "mcadata"))
    return false;

  pugi::xml_node node = root.child("spectrum");
  if (!node)
    return false;

  std::string StartData(node.attribute("start").value());
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
    iss >> metadata_.start_time;
  }

  std::string RealTime(node.attribute("elapsed_real").value()); boost::algorithm::trim(RealTime);
  if (!RealTime.empty()) {
    double rt_ms = boost::lexical_cast<double>(RealTime) * 1000.0;
    metadata_.real_time = boost::posix_time::milliseconds(rt_ms);
  }

  std::string LiveTime(node.attribute("elapsed_live").value()); boost::algorithm::trim(LiveTime);
  if (!LiveTime.empty()) {
    double lt_ms = boost::lexical_cast<double>(LiveTime) * 1000.0;
    metadata_.live_time = boost::posix_time::milliseconds(lt_ms);
  }

  std::string this_data(root.child_value("spectrum"));

  std::replace( this_data.begin(), this_data.end(), ',', ' ');
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);

  if(!this->channels_from_string(channeldata, false))
    return false;

  Gamma::Detector newdet;
  for (auto &q : root.children("calibration_details")) {
    if ((std::string(q.attribute("type").value()) != "energy") ||
        !q.child("model"))
      continue;

    Gamma::Calibration newcalib("Energy", metadata_.bits);

    if (std::string(q.child("model").attribute("type").value()) == "polynomial")
      newcalib.model_ = Gamma::CalibrationModel::polynomial;

    std::vector<double> encalib;
    for (auto &p : q.child("model").children("coefficient")) {
      std::string coefvalstr = boost::algorithm::trim_copy(std::string(p.attribute("value").value()));
      encalib.push_back(boost::lexical_cast<double>(coefvalstr));
    }
    newcalib.coefficients_ = encalib;
    newcalib.units_ = "keV";
    newcalib.type_ = "Energy";
    newdet.energy_calibrations_.add(newcalib);
    newdet.name_ = "unknown";
  }

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = newdet;

  init_from_file(filename);
  return true;
}

void Spectrum1D::write_tka(std::string name) const {
  uint32_t range = (metadata_.resolution - 2);
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  //  myfile.precision(2);
  //  myfile << std::fixed;
  myfile << (metadata_.live_time.total_milliseconds() * 0.001) << std::endl
         << (metadata_.real_time.total_milliseconds() * 0.001) << std::endl;
      
  for (uint32_t i = 0; i < range; i++)
    myfile << spectrum_[i] << std::endl;
  myfile.close();
}

void Spectrum1D::write_n42(std::string filename) const {
  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child();

  std::stringstream durationdata;
  Gamma::Calibration myCalibration;
  if (metadata_.detectors[0].energy_calibrations_.has_a(Gamma::Calibration("Energy", metadata_.bits)))
    myCalibration = metadata_.detectors[0].energy_calibrations_.get(Gamma::Calibration("Energy", metadata_.bits));
  else if ((metadata_.detectors[0].energy_calibrations_.size() == 1) &&
           (metadata_.detectors[0].energy_calibrations_.get(0).valid()))
    myCalibration = metadata_.detectors[0].energy_calibrations_.get(0);

  root.set_name("N42InstrumentData");
  root.append_attribute("xmlns").set_value("http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242");
  root.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
  root.append_attribute("xsi:schemaLocation").set_value("http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242 http://physics.nist.gov/Divisions/Div846/Gp4/ANSIN4242/2005/ANSIN4242.xsd");
  
  pugi::xml_node node = root.append_child("Measurement").append_child("Spectrum");
  node.append_attribute("Type").set_value("PHA");

  if (myCalibration.valid()) {
    node.append_attribute("Detector").set_value(metadata_.detectors[0].name_.c_str());
    node.append_child("DetectorType").append_child(pugi::node_pcdata).set_value(metadata_.detectors[0].type_.c_str());
  }

  durationdata << to_iso_extended_string(metadata_.start_time) << "-5:00"; //fix this hack
  node.append_child("StartTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());
      
  durationdata.str(std::string()); //clear it
  durationdata << "PT" << (metadata_.real_time.total_milliseconds() * 0.001) << "S";
  node.append_child("RealTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());

  durationdata.str(std::string()); //clear it
  durationdata << "PT" << (metadata_.live_time.total_milliseconds() * 0.001) << "S";
  node.append_child("LiveTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());

  if (myCalibration.valid())
    myCalibration.to_xml(node);

  if ((metadata_.resolution > 0) && (metadata_.total_count > 0)) {
    node.append_child("ChannelData").append_attribute("Compression").set_value("CountedZeroes");
    node.last_child().append_child(pugi::node_pcdata).set_value(this->_channels_to_xml().c_str());
  }

  if (!doc.save_file(filename.c_str()))
    PL_ERR << "<Spectrum1D> Failed to save " << filename;
}

}}
