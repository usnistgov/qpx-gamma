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
 *      Qpx::Spectrum::Spectrum generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::Spectrum::Factory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::Spectrum::Registrar for registering new spectrum types.
 *
 ******************************************************************************/

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <memory>
#include <initializer_list>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "daq_types.h"
#include "detector.h"
#include "custom_logger.h"

namespace Qpx {
namespace Spectrum {

typedef std::pair<std::vector<uint16_t>, PreciseFloat> Entry;
typedef std::list<Entry> EntryList;
typedef std::pair<uint32_t, uint32_t> Pair;


struct Metadata : public XMLable {
 private:
  //this stuff from factory
  std::string type_, type_description_;
  uint16_t dimensions_;
  std::list<std::string> input_types_;
  std::list<std::string> output_types_;

 public:
  //user sets these in prototype
  std::string name;
  uint16_t bits;
  Qpx::Setting attributes;

  //take care of these...
  bool changed;
  PreciseFloat total_count;
  std::vector<Qpx::Detector> detectors;

  std::string type() const {return type_;}
  std::string type_description() const {return type_description_;}
  uint16_t dimensions() const {return dimensions_;}
  std::list<std::string> input_types() const {return input_types_;}
  std::list<std::string> output_types() const {return output_types_;}

  Metadata()
    : type_("invalid")
    , dimensions_(0)
    , bits(14)
    , attributes("Options")
    , total_count(0.0)
    , changed(false)
    { attributes.metadata.setting_type = SettingType::stem; }

  Metadata(std::string tp, std::string descr, uint16_t dim,
           std::list<std::string> itypes, std::list<std::string> otypes)
    : type_(tp)
    , type_description_(descr)
    , dimensions_(dim)
    , input_types_(itypes)
    , output_types_(otypes)
    , bits(14)
    , attributes("Options")
    , total_count(0.0)
    , changed(false)
    { attributes.metadata.setting_type = SettingType::stem; }

 std::string xml_element_name() const override {return "SpectrumMetadata";}

 void to_xml(pugi::xml_node &node) const override;
 void from_xml(const pugi::xml_node &node) override;

 bool shallow_equals(const Metadata& other) const {return (name == other.name);}
 bool operator!= (const Metadata& other) const {return !operator==(other);}
 bool operator== (const Metadata& other) const {
   if (name != other.name) return false;
   if (type_ != other.type_) return false; //assume other type info same
   if (bits != other.bits) return false;
   if (attributes != other.attributes) return false;
   return true;
 }
};


class Spectrum
{
public:
  Spectrum();

  //named constructors, used by factory
  bool from_prototype(const Metadata&);
  bool from_xml(const pugi::xml_node &);

  //get count for specific MCA channel in spectrum
  //list takes as many parameters as there are dimensions
  //implemented in children by _get_count
  PreciseFloat get_count(std::initializer_list<uint16_t> list = {}) const;
  
  //optimized retrieval of whole spectrum as list of Entries (typedef above)
  //parameters take dimensions_ number of ranges of minimum (inclusive) and maximum (exclusive)
  //implemented in children by _get_spectrum and _get_spectrum_update
  std::unique_ptr<EntryList> get_spectrum(std::initializer_list<Pair> list = {});
  void add_bulk(const Entry&);

  //full save with custom format
  void to_xml(pugi::xml_node &) const;

  //export to some format (factory keeps track of file types)
  bool write_file(std::string dir, std::string format) const;
  bool read_file(std::string name, std::string format);

  //for custom saving (implemented in strategies below)
  std::string channels_to_xml() const;
  uint16_t channels_from_xml(const std::string& str);
  
  //retrieve pre-calculated energies for a given channel
  std::vector<double> energies(uint8_t chan = 0) const;
  
  //set and get detectors
  void set_detectors(const std::vector<Qpx::Detector>& dets);

  //feed acquired data to spectrum
  void addSpill(const Spill&);
  void closeAcquisition(); //must call this after completing

  ///////////////////////////////////////////////
  ///////accessors for various properties////////
  ///////////////////////////////////////////////
  Metadata metadata() const;
  
  std::string name() const;
  std::string type() const;
  uint16_t dimensions() const;
  uint16_t bits() const;

  std::map<int, std::list<StatsUpdate>> get_stats();

  //change properties - use carefully...
  void set_generic_attr(Setting setting, Match match = Match::id | Match::indices);
  void set_generic_attrs(Setting settings);
  void reset_changed();

protected:
  ////////////////////////////////////////
  ////////////////////////////////////////
  //////////THIS IS THE MEAT//////////////
  //implement these to make custom types//
  ////////////////////////////////////////
  
  virtual std::string my_type() const {return "INVALID";}
  virtual bool initialize();

  virtual bool _write_file(std::string, std::string) const {return false;}
  virtual bool _read_file(std::string, std::string) {return false;}

  virtual PreciseFloat _get_count(std::initializer_list<uint16_t>) const = 0;
  virtual std::unique_ptr<std::list<Entry>> _get_spectrum(std::initializer_list<Pair>) = 0;
  virtual void _add_bulk(const Entry&) {}

  virtual std::string _channels_to_xml() const = 0;
  virtual uint16_t _channels_from_xml(const std::string&) = 0;

  virtual void pushHit(const Hit&);           //has default behavior
  virtual void addEvent(const Event&) = 0;
  virtual void addStats(const StatsUpdate&);  //has default behavior
  virtual void addRun(const RunInfo&);        //has default behavior
  virtual bool validateEvent(const Event&) const; //has default behavior
  virtual void _closeAcquisition() {}

  virtual void _set_detectors(const std::vector<Qpx::Detector>& dets); //has default behavior

  void recalc_energies();
  Setting get_attr(std::string name) const;

  //////////////////////////////
  ///////member variables///////
  //////////////////////////////
  
  mutable boost::shared_mutex mutex_;
  mutable boost::mutex u_mutex_;

  Metadata metadata_;

  int32_t cutoff_logic_;
  double coinc_window_;

  std::vector<std::vector<double> > energies_;
  
  std::map<int, std::list<StatsUpdate>> stats_list_;
  std::map<int, boost::posix_time::time_duration> real_times_;
  std::map<int, boost::posix_time::time_duration> live_times_;
  std::list<Event> backlog;

  uint64_t recent_count_;
  StatsUpdate recent_start_, recent_end_;

  Pattern pattern_coinc_, pattern_anti_, pattern_add_;
};

}}

#endif // SPECTRUM_H
