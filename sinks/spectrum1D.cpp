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
 *      Qpx::Sink1D one-dimensional spectrum
 *
 ******************************************************************************/


#include <fstream>
#include <boost/algorithm/string.hpp>
#include "spectrum1D.h"
#include "daq_sink_factory.h"
#include "xylib.h"
#include "qpx_util.h"

namespace Qpx {

static SinkRegistrar<Spectrum1D> registrar("1D");

Spectrum1D::Spectrum1D()
  : cutoff_bin_(0)
  , maxchan_(0)
{
  Setting base_options = metadata_.attributes();
  metadata_ = Metadata("1D", "Traditional MCA spectrum", 1,
  {"cnf", "tka", "n42", "ava", "spe", "Spe", "CNF", "N42", "mca", "dat"},
  {"n42", "tka", "spe"});

  Qpx::Setting cutoff_bin;
  cutoff_bin.id_ = "cutoff_bin";
  cutoff_bin.metadata.setting_type = Qpx::SettingType::integer;
  cutoff_bin.metadata.description = "Hits rejected below minimum energy (affects binning only)";
  cutoff_bin.metadata.writable = true;
  cutoff_bin.metadata.minimum = 0;
  cutoff_bin.metadata.step = 1;
  cutoff_bin.metadata.maximum = 1000000;
  cutoff_bin.metadata.flags.insert("preset");
  base_options.branches.add(cutoff_bin);

  metadata_.overwrite_all_attributes(base_options);
}

void Spectrum1D::_set_detectors(const std::vector<Qpx::Detector>& dets) {
  metadata_.detectors.resize(metadata_.dimensions(), Qpx::Detector());

  if (dets.size() == metadata_.dimensions())
    metadata_.detectors = dets;
  if (dets.size() >= metadata_.dimensions()) {

    for (int i=0; i < dets.size(); ++i) {
      if (pattern_add_.relevant(i)) {
        metadata_.detectors[0] = dets[i];
        break;
      }
    }
  }

  this->_recalc_axes();
}

bool Spectrum1D::_initialize()
{
  Spectrum::_initialize();

  cutoff_bin_ = metadata_.get_attribute("cutoff_bin").value_int;

  spectrum_.resize(pow(2, bits_), 0);

  return true;
}

PreciseFloat Spectrum1D::_data(std::initializer_list<uint16_t> list) const {
  if (list.size() != 1)
    return 0;
  
  uint32_t chan = *list.begin();
  if (chan >= spectrum_.size())
    return 0;
  else
    return spectrum_[chan];
}

std::unique_ptr<std::list<Entry>> Spectrum1D::_data_range(std::initializer_list<Pair> list) {
  int min, max;
  if (list.size() != 1) {
    min = 0;
    max = pow(2, bits_);
  } else {
    Pair range = *list.begin();
    min = range.first;
    max = range.second;
  }

  if (max >= spectrum_.size())
    max = spectrum_.size() - 1;

  //in range?
  
  std::unique_ptr<std::list<Entry>> result(new std::list<Entry>);
  for (int i=min; i <= max; i++) {
    Entry newentry;
    newentry.first.resize(1, 0);
    newentry.first[0] = i;
    newentry.second = spectrum_[i];
    result->push_back(newentry);
  }
  return result;
}

void Spectrum1D::_append(const Entry& e) {
  for (int i = 0; i < e.first.size(); ++i)
    if (pattern_add_.relevant(i) && (e.first[i] < spectrum_.size())) {
      spectrum_[e.first[i]] += e.second;
//      metadata_.total_count += e.second;
      total_hits_ += e.second;

      //total events??? HACK!!
    }
}

void Spectrum1D::addHit(const Hit& newHit) {
  uint16_t en = newHit.value(energy_idx_.at(newHit.source_channel())).val(bits_);
  if (en < cutoff_bin_)
    return;

  ++spectrum_[en];

  if (en > maxchan_)
    maxchan_ = en;
}

void Spectrum1D::addEvent(const Event& newEvent) {
  for (auto &h : newEvent.hits)
    if (pattern_add_.relevant(h.first))
      this->addHit(h.second);
}

bool Spectrum1D::_write_file(std::string dir, std::string format) const {
  std::string name = metadata_.get_attribute("name").value_text;
  //change illegal characters
  if (format == "tka") {
    write_tka(dir + "/" + name + ".tka");
    return true;
  } else if (format == "n42") {
    write_n42(dir + "/" + name + ".n42");
    return true;
  } else if (format == "spe") {
    write_spe(dir + "/" + name + ".spe");
    return true;
  } else
    return false;
}

bool Spectrum1D::_read_file(std::string name, std::string format) {
//  DBG << "Will try to make " << format;

  if (format == "tka")
    return read_tka(name);
  else if (format == "n42")
    return read_n42(name);
  else if (format == "ava")
    return read_ava(name);
  else if (format == "dat")
    return read_dat(name);
  else if (format == "spe") {
    if (read_spe_radware(name))
      return true;
    else if (read_spe_gammavision(name))
      return true;
    else
      return read_xylib(name, format);
  }
  else if ((format == "mca") || (format == "cnf"))
    return read_xylib(name, format);
  else
    return false;
}

void Spectrum1D::init_from_file(std::string filename) {

  pattern_coinc_.resize(1);
  pattern_coinc_.set_gates(std::vector<bool>({true}));

  pattern_anti_.resize(1);
  pattern_anti_.set_gates(std::vector<bool>({false}));

  pattern_add_.resize(1);
  pattern_add_.set_gates(std::vector<bool>({true}));

  Qpx::Setting pattern;
  pattern = metadata_.get_attribute("pattern_coinc");
  pattern.value_pattern = pattern_coinc_;
  metadata_.set_attribute(pattern);

  pattern = metadata_.get_attribute("pattern_anti");
  pattern.value_pattern = pattern_anti_;
  metadata_.set_attribute(pattern);

  pattern = metadata_.get_attribute("pattern_add");
  pattern.value_pattern = pattern_add_;
  metadata_.set_attribute(pattern);

  Setting name = metadata_.get_attribute("name");
  name.value_text = boost::filesystem::path(filename).filename().string();
  std::replace( name.value_text.begin(), name.value_text.end(), '.', '_');
  metadata_.set_attribute(name);

  _initialize();
  _recalc_axes();
  _flush();

  Qpx::Setting cts;
  cts = metadata_.get_attribute("total_hits");
  cts.value_precise = total_hits_;
  metadata_.set_attribute(cts);

  cts = metadata_.get_attribute("total_events");
  cts.value_precise = total_events_;
  metadata_.set_attribute(cts);
}

std::string Spectrum1D::_data_to_xml() const {
  std::stringstream channeldata;

  PreciseFloat z_count = 0;
  for (uint32_t i = 0; i < maxchan_; i++)
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

uint16_t Spectrum1D::_data_from_xml(const std::string& thisData){
  std::stringstream channeldata;
  channeldata.str(thisData);

  bits_ = metadata_.get_attribute("resolution").value_int;

  spectrum_.clear();
  spectrum_.resize(pow(2, bits_), 0);

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
      i++;
    }
  }
  maxchan_ = i;
  return maxchan_;
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

  bits_ = log2(i);
  if (pow(2, bits_) < i)
    bits_++;
  maxchan_ = i;

  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  spectrum_.clear();
  spectrum_.resize(pow(2, bits_), 0);
      
  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    total_hits_ += q.second;
  }

  return true;
}


bool Spectrum1D::read_xylib(std::string name, std::string ext) {
  xylib::DataSet* newdata;

  if (ext == "mca") {
    newdata = xylib::load_file(name, "canberra_mca");
  } else if (ext == "cnf") {
    newdata = xylib::load_file(name, "canberra_cnf");
  } else if (ext == "spe") {
    newdata = xylib::load_file(name, "spe");
  } else
    return false;

  if (newdata == nullptr)
    return false;

  std::vector<double> calibration;

//  DBG << "xylib.blocks =  " << newdata->get_block_count();
  for (int i=0; i < newdata->get_block_count(); i++) {
    calibration.clear();

    for (uint32_t j=0; j < newdata->get_block(i)->meta.size(); j++) {
      std::string key = newdata->get_block(i)->meta.get_key(j);
      std::string value = newdata->get_block(i)->meta.get(key);
//      DBG << "xylib.meta " << key << " = " << value;
      if (key.substr(0, 12) == "energy calib")
        calibration.push_back(boost::lexical_cast<double>(value));
      if (key == "description") {
        Setting descr = metadata_.get_attribute("description");
        descr.value_text = value;
        metadata_.set_attribute(descr);
      }
      if (key == "date and time") {
        std::stringstream iss;
        iss.str(value);
        std::string week;
        iss >> week;
        Setting start_time = metadata_.get_attribute("start_time");
        start_time.value_time = from_custom_format(iss.str(), "%Y-%m-%d %H:%M:%S");
        metadata_.set_attribute(start_time);
      }
      if (key == "real time (s)") {
        double RT = boost::lexical_cast<double>(value) * 1000;
        Setting real_time = metadata_.get_attribute("real_time");
        real_time.value_duration = boost::posix_time::milliseconds(RT);
        metadata_.set_attribute(real_time);
      }
      if (key == "live time (s)") {
        Setting live_time = metadata_.get_attribute("live_time");
        double LT = boost::lexical_cast<double>(value) * 1000;
        live_time.value_duration = boost::posix_time::milliseconds(LT);
        metadata_.set_attribute(live_time);
      }
    }

    double tempcount = 0.0;
    int column = newdata->get_block(i)->get_column_count();
//    DBG << "xylib.columns = " << column;
//    if (ext == "vms")
//      column--;

    if (column == 2) {

//      DBG << "xylib.points = " << newdata->get_block(i)->get_point_count();
      for (int k = 0; k < newdata->get_block(i)->get_point_count(); k++) {
        double data = newdata->get_block(i)->get_column(column).get_value(k);
        PreciseFloat nr(data);

        spectrum_.push_back(PreciseFloat(data));

        tempcount += data;
      }
    }
    total_events_ = total_hits_ = tempcount;
  }

  uint32_t resolution = spectrum_.size();
  bits_ = log2(resolution);
  if (pow(2, bits_) < resolution)
    bits_++;
  spectrum_.resize(pow(2, bits_), 0);

  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = Qpx::Detector();
  metadata_.detectors[0].name_ = "unknown";
  Qpx::Calibration new_calib("Energy", bits_);
  new_calib.coefficients_ = calibration;
  new_calib.units_ = "keV";
  metadata_.detectors[0].energy_calibrations_.add(new_calib);

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
  Setting live_time = metadata_.get_attribute("live_time");
  timed = boost::lexical_cast<double>(trim_copy(data)) * 1000.0;
  live_time.value_duration = boost::posix_time::milliseconds(timed);
  metadata_.set_attribute(live_time);

  myfile >> data;
  Setting real_time = metadata_.get_attribute("real_time");
  timed = boost::lexical_cast<double>(trim_copy(data)) * 1000.0;
  real_time.value_duration = boost::posix_time::milliseconds(timed);
  metadata_.set_attribute(real_time);

  if(!this->channels_from_string(myfile, false))
    return false;

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = Qpx::Detector();
  metadata_.detectors[0].name_ = "unknown";
  
  init_from_file(name);

  return true;
}

bool Spectrum1D::read_spe_radware(std::string name) {
  //radware spe format
  std::ifstream myfile(name, std::ios::in | std::ios::binary);
  if (!myfile)
    return false;

  myfile.seekg (0, myfile.end);
  int length = myfile.tellg();

  if (length < 36)
    return false;

  myfile.seekg (0, myfile.beg);

  uint32_t val;
  myfile.read ((char*)&val, sizeof(uint32_t));
  if (val != 24) {
    myfile.close();
    return false;
  }

  char cname[9];
  myfile.read ((char*)&cname, 8*sizeof(char));
  cname[8] = '\0';
  Setting pname = metadata_.get_attribute("name");
  pname.value_text = std::string(cname);
  metadata_.set_attribute(pname);

  uint32_t dim1, dim2;
  myfile.read ((char*)&dim1, sizeof(uint32_t));
  myfile.read ((char*)&dim2, sizeof(uint32_t));

  if ((dim1*dim2) > length)
    return false;

  uint32_t resolution = dim1*dim2;

  myfile.seekg (28, myfile.beg);

  myfile.read ((char*)&val, sizeof(uint32_t));
  if (val != 24)
    return false;

  myfile.read ((char*)&val, sizeof(uint32_t));
  if (val != resolution *4)
    return false;

  spectrum_.clear();
  total_hits_ = 0;
  maxchan_ = 0;

  std::list<Entry> entry_list;
  float one;
  int i=0;
  while ((myfile.tellg() != length) && (i < resolution)) {
    myfile.read ((char*)&one, sizeof(float));
    Entry new_entry;
    new_entry.first.resize(1);
    new_entry.first[0] = i;
    new_entry.second = one;
    entry_list.push_back(new_entry);
    if (one > 0)
      maxchan_ = i;
    i++;
  }

  myfile.read ((char*)&val, sizeof(uint32_t));
//  DBG << "bytes again = " << val;

  myfile.close();

  bits_ = log2(i);
  if (pow(2, bits_) < i)
    bits_++;
  resolution = pow(2, bits_);
  maxchan_ = i;

  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  spectrum_.clear();
  spectrum_.resize(pow(2, bits_), 0);

  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    total_hits_ += q.second;
  }
  total_events_ = total_hits_;

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = Qpx::Detector();
  metadata_.detectors[0].name_ = "unknown";

  init_from_file(name);

  return true;
}


bool Spectrum1D::read_spe_gammavision(std::string name) {
  //gammavision plain text
  std::ifstream myfile(name, std::ios::in);
  if (!myfile)
    return false;

  std::string line, token;
  std::list<Entry> entry_list;
  int detnum = 0;
  std::string detname = "unknown";
  std::string enerfit, mcacal, shapecal;
  double time;

  myfile >> line;
  boost::algorithm::trim(line);
  if (line != "$SPEC_ID:") {
    myfile.close();
    return false;
  }

  myfile.seekg (0, myfile.beg);
  line.clear();

  DBG << "<Spectrum1D> Reading spe as gammavision";

  while (myfile.rdbuf()->in_avail()) {
    if (line.empty()) {
      std::getline(myfile, line);
      boost::algorithm::trim(line);
    }
    //DBG << "Line = " << line;
    if (line == "$SPEC_ID:")
    {
      Setting name = metadata_.get_attribute("name");
      std::getline(myfile, name.value_text);
      metadata_.set_attribute(name);
      //DBG << "name: " << metadata_.name;
      line.clear();
    }
    else if (line == "$SPEC_REM:")
    {
      while (myfile.rdbuf()->in_avail()) {
        std::getline(myfile, line);
        boost::algorithm::trim(line);
        std::stringstream ss(line);
        ss >> token;
        if (token == "DET#")
          ss >> detnum;
        else if (token == "DETDESC#") {
          detname.clear();
          while (ss.rdbuf()->in_avail()) {
            ss >> token;
            detname += token + " ";
          }
          boost::algorithm::trim(detname);
        }
        else if (token == "AP#")
          ss >> token; //do nothing
        else
          break;
      }
    } else if (line == "$MEAS_TIM:") {
      std::getline(myfile, line);
      boost::algorithm::trim(line);
      std::stringstream ss(line);

      ss >> time;
      Setting live_time = metadata_.get_attribute("live_time");
      live_time.value_duration = boost::posix_time::seconds(time);
      metadata_.set_attribute(live_time);

      ss >> time;
      Setting real_time = metadata_.get_attribute("real_time");
      real_time.value_duration = boost::posix_time::seconds(time);
      metadata_.set_attribute(real_time);

      line.clear();
    } else if (line == "$DATE_MEA:") {
      std::getline(myfile, line);
      Setting start_time = metadata_.get_attribute("start_time");
      start_time.value_time = from_custom_format(line, "%m/%d/%Y %H:%M:%S");
      metadata_.set_attribute(start_time);
      line.clear();
    } else if (line == "$ENER_FIT:") {
      std::getline(myfile, enerfit);
      line.clear();
    } else if (line == "$MCA_CAL:") {
      std::getline(myfile, line);
      std::getline(myfile, mcacal);
      line.clear();
    } else if (line == "$SHAPE_CAL:") {
      std::getline(myfile, line);
      std::getline(myfile, shapecal);
      line.clear();
    } else if (line == "$DATA:") {
      std::getline(myfile, line);
      std::stringstream ss(line);
      int k1, k2;
      ss >> k1 >> k2;

      //DBG << "should be " << k2;
      for (int i=0; i < k2; ++i) {
        myfile >> token;
        Entry new_entry;
        new_entry.first.resize(1);
        new_entry.first[0] = i;
        PreciseFloat nr(token);
        new_entry.second = nr;
        entry_list.push_back(new_entry);
        if (nr > 0)
          maxchan_ = i;
      }
      line.clear();
    } else
      line.clear();
  }

  //DBG << "entry list is " << entry_list.size();

  bits_ = log2(entry_list.size());
  if (pow(2, bits_) < entry_list.size())
    bits_++;
  maxchan_ = entry_list.size() - 1;

  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  spectrum_.clear();
  spectrum_.resize(pow(2, bits_), 0);

  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    total_hits_ += q.second;
  }
  total_events_ = total_hits_;

  Qpx::Calibration enc("Energy", bits_, "keV");
  enc.coef_from_string(mcacal);

  Qpx::Calibration fwc("FWHM", bits_, "keV");
  fwc.coef_from_string(shapecal);


  Qpx::Detector det(detname);
  det.energy_calibrations_.add(enc);
  det.fwhm_calibration_ = fwc;

  metadata_.detectors.resize(1);
  metadata_.detectors[0] = det;

  init_from_file(name);

//  metadata_.match_pattern.resize(detnum+1);
//  metadata_.match_pattern[detnum] = 1;

//  metadata_.add_pattern.resize(detnum+1);
//  metadata_.add_pattern[detnum] = 1;

  return true;
}

bool Spectrum1D::read_dat(std::string name) {
  std::ifstream myfile(name, std::ios::in);
  if (!myfile)
    return false;

  std::string line, token;
  std::list<Entry> entry_list;

  while (myfile.rdbuf()->in_avail()) {
    std::getline(myfile, line);
    std::stringstream ss(line);
    uint32_t k1, k2;
    ss >> k1 >> k2;

    Entry new_entry;
    new_entry.first.resize(1);
    new_entry.first[0] = k1;
    new_entry.second = k2;
    entry_list.push_back(new_entry);
    if ((k2 > 0) && (k1 > maxchan_))
       maxchan_ = k1;
  }

  //DBG << "entry list is " << entry_list.size();

  bits_ = log2(maxchan_);
  if (pow(2, bits_) < entry_list.size())
    bits_++;
  maxchan_ = entry_list.size() - 1;

  Setting res = metadata_.get_attribute("resolution");
  res.value_int = bits_;
  metadata_.set_attribute(res);

  spectrum_.clear();
  spectrum_.resize(pow(2, bits_), 0);

  for (auto &q : entry_list) {
    spectrum_[q.first[0]] = q.second;
    total_hits_ += q.second;
  }
  total_events_ = total_hits_;

  metadata_.detectors.resize(1);

  init_from_file(name);

//  metadata_.match_pattern.resize(detnum+1);
//  metadata_.match_pattern[detnum] = 1;

//  metadata_.add_pattern.resize(detnum+1);
//  metadata_.add_pattern[detnum] = 1;

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
    Setting start_time = metadata_.get_attribute("start_time");
    start_time.value_time = from_iso_extended(time);
    metadata_.set_attribute(start_time);
  }

  time = std::string(node.child_value("RealTime"));
  if (!time.empty()) {
    boost::algorithm::trim(time);
    if (time.size() > 3)
      time = time.substr(2, time.size()-3); //to trim PTnnnS to nnn
    double rt_ms = boost::lexical_cast<double>(time) * 1000.0;
    Setting real_time = metadata_.get_attribute("real_time");
    real_time.value_duration = boost::posix_time::milliseconds(rt_ms);
    metadata_.set_attribute(real_time);
  }

  time = std::string(node.child_value("LiveTime"));
  if (!time.empty()) {
    boost::algorithm::trim(time);
    if (time.size() > 3)
      time = time.substr(2, time.size()-3); //to trim PTnnnS to nnn
    double lt_ms = boost::lexical_cast<double>(time) * 1000.0;
    Setting live_time = metadata_.get_attribute("live_time");
    live_time.value_duration = boost::posix_time::milliseconds(lt_ms);
    metadata_.set_attribute(live_time);
  }

  std::string this_data(node.child_value("ChannelData"));
  
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);

  if (!this->channels_from_string(channeldata, true)) //assume compressed
    return false;

  Qpx::Detector newdet;
  newdet.name_ = "unknown";
  if (node.attribute("Detector"))
    newdet.name_ = std::string(node.attribute("Detector").value());
  
  newdet.type_ = std::string(node.child_value("DetectorType"));

  //have more claibrations? fwhm,etc..
  if (node.child("Calibration")) {
    Qpx::Calibration newcalib;
    newcalib.from_xml(node.child("Calibration"));
    newcalib.bits_ = bits_;
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
    Setting start_time = metadata_.get_attribute("start_time");
    iss >> start_time.value_time;
    metadata_.set_attribute(start_time);
  }

  std::string RealTime(node.attribute("elapsed_real").value()); boost::algorithm::trim(RealTime);
  if (!RealTime.empty()) {
    double rt_ms = boost::lexical_cast<double>(RealTime) * 1000.0;
    Setting real_time = metadata_.get_attribute("real_time");
    real_time.value_duration = boost::posix_time::milliseconds(rt_ms);
    metadata_.set_attribute(real_time);
  }

  std::string LiveTime(node.attribute("elapsed_live").value()); boost::algorithm::trim(LiveTime);
  if (!LiveTime.empty()) {
    double lt_ms = boost::lexical_cast<double>(LiveTime) * 1000.0;
    Setting live_time = metadata_.get_attribute("live_time");
    live_time.value_duration = boost::posix_time::milliseconds(lt_ms);
    metadata_.set_attribute(live_time);
  }

  std::string this_data(root.child_value("spectrum"));

  std::replace( this_data.begin(), this_data.end(), ',', ' ');
  boost::algorithm::trim(this_data);
  std::stringstream channeldata;
  channeldata.str(this_data);

  if(!this->channels_from_string(channeldata, false))
    return false;

  Qpx::Detector newdet;
  for (auto &q : root.children("calibration_details")) {
    if ((std::string(q.attribute("type").value()) != "energy") ||
        !q.child("model"))
      continue;

    Qpx::Calibration newcalib("Energy", bits_);

    if (std::string(q.child("model").attribute("type").value()) == "polynomial")
      newcalib.model_ = Qpx::CalibrationModel::polynomial;

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
  uint32_t range = (pow(2, bits_) - 2);
  std::ofstream myfile(name, std::ios::out | std::ios::app);
  //  myfile.precision(2);
  //  myfile << std::fixed;
  myfile << (metadata_.get_attribute("live_time").value_duration.total_milliseconds() * 0.001) << std::endl
         << (metadata_.get_attribute("real_time").value_duration.total_milliseconds() * 0.001) << std::endl;
      
  for (uint32_t i = 0; i < range; i++)
    myfile << spectrum_[i] << std::endl;
  myfile.close();
}

void Spectrum1D::write_n42(std::string filename) const {
  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child();

  std::stringstream durationdata;
  Qpx::Calibration myCalibration;
  if (metadata_.detectors[0].energy_calibrations_.has_a(Qpx::Calibration("Energy", bits_)))
    myCalibration = metadata_.detectors[0].energy_calibrations_.get(Qpx::Calibration("Energy", bits_));
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

  durationdata << to_iso_extended_string(metadata_.get_attribute("start_time").value_time) << "-5:00"; //fix this hack
  node.append_child("StartTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());
      
  durationdata.str(std::string()); //clear it
  durationdata << "PT" << (metadata_.get_attribute("real_time").value_duration.total_milliseconds() * 0.001) << "S";
  node.append_child("RealTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());

  durationdata.str(std::string()); //clear it
  durationdata << "PT" << (metadata_.get_attribute("live_time").value_duration.total_milliseconds() * 0.001) << "S";
  node.append_child("LiveTime").append_child(pugi::node_pcdata).set_value(durationdata.str().c_str());

  if (myCalibration.valid())
    myCalibration.to_xml(node);

//  if (metadata_.total_count > 0) {
    node.append_child("ChannelData").append_attribute("Compression").set_value("CountedZeroes");
    node.last_child().append_child(pugi::node_pcdata).set_value(this->_data_to_xml().c_str());
//  }

  if (!doc.save_file(filename.c_str()))
    ERR << "<Spectrum1D> Failed to save " << filename;
}

void Spectrum1D::write_spe(std::string filename) const {
  //radware format

  std::ofstream myfile(filename, std::ios::out | std::ios::trunc | std::ios::binary);

  if (!myfile.is_open())
    return;

  uint32_t val = 24;
  myfile.write((char*)&val, sizeof(uint32_t));

  std::string cname = metadata_.get_attribute("name").value_text;
  if (cname.size() < 8)
    cname.resize(8);
  myfile.write ((char*)cname.data(), 8*sizeof(char));

  uint32_t dim1 = pow(2, bits_);
  uint32_t dim2 = 1;
  myfile.write ((char*)&dim1, sizeof(uint32_t));
  myfile.write ((char*)&dim2, sizeof(uint32_t));
  myfile.write ((char*)&dim2, sizeof(uint32_t));
  myfile.write ((char*)&dim2, sizeof(uint32_t));

  myfile.write ((char*)&val, sizeof(uint32_t));

  val = dim1 *4;
  myfile.write ((char*)&val, sizeof(uint32_t));


  float one = 0.0;
  for (int i=0; i < dim1; ++i) {
    one = 0.0;
    if (i < spectrum_.size())
      one = spectrum_[i].convert_to<double>();
    myfile.write ((char*)&one, sizeof(float));
  }

  myfile.write ((char*)&val, sizeof(uint32_t));
  myfile.close();
}

}
