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
 *      Qpx::Sink generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::SinkFactory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::SinkRegistrar for registering new spectrum types.
 *
 ******************************************************************************/

#ifndef QPX_SINK_H
#define QPX_SINK_H

#include <initializer_list>
#include <boost/thread.hpp>

#include "spill.h"
#include "detector.h"
#include "custom_logger.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace Qpx {

typedef std::pair<std::vector<size_t>, PreciseFloat> Entry;
typedef std::list<Entry> EntryList;
typedef std::pair<size_t, size_t> Pair;


class Metadata : public XMLable {
public:
  Metadata();
  Metadata(std::string tp, std::string descr, uint16_t dim,
           std::list<std::string> itypes, std::list<std::string> otypes);

  // read & write
  Setting get_attribute(std::string setting) const;
  Setting get_attribute(std::string setting, int32_t idx) const;
  Setting get_attribute(Setting setting) const;
  void set_attribute(const Setting &setting);

  Setting attributes() const;
  void set_attributes(const Setting &settings);
  void overwrite_all_attributes(Setting settings);

  //read only
  std::string type() const {return type_;}
  std::string type_description() const {return type_description_;}
  uint16_t dimensions() const {return dimensions_;}
  std::list<std::string> input_types() const {return input_types_;}
  std::list<std::string> output_types() const {return output_types_;}


  void disable_presets();
  void set_det_limit(uint16_t limit);
  bool chan_relevant(uint16_t chan) const;


  std::string xml_element_name() const override {return "SinkMetadata";}
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;

  bool shallow_equals(const Metadata& other) const;
  bool operator!= (const Metadata& other) const;
  bool operator== (const Metadata& other) const;

private:
  //this stuff from factory, immutable upon initialization
  std::string type_, type_description_;
  uint16_t dimensions_;
  std::list<std::string> input_types_;
  std::list<std::string> output_types_;

  //can change these
  Setting attributes_;

public:
  std::vector<Qpx::Detector> detectors;

};


class Sink
{
protected:
  Metadata metadata_;
  std::vector<std::vector<double> > axes_;

  mutable boost::shared_mutex shared_mutex_;
  mutable boost::mutex unique_mutex_;
  bool changed_;

public:
  Sink();
  Sink(const Sink& other)
    : metadata_(other.metadata_)
    , axes_ (other.axes_) {}
  virtual Sink* clone() const = 0;
  virtual ~Sink() {}

  //named constructors, used by factory
  bool from_prototype(const Metadata&);
  bool load(const pugi::xml_node &, std::shared_ptr<boost::archive::binary_iarchive>);
  void save(pugi::xml_node &, std::shared_ptr<boost::archive::binary_oarchive>) const;

  //data acquisition
  void push_spill(const Spill&);
  void flush();

  //get count at coordinates in n-dimensional list
  PreciseFloat data(std::initializer_list<size_t> list = {}) const;

  //optimized retrieval of bulk data as list of Entries
  //parameters take dimensions_number of ranges (inclusive)
  std::unique_ptr<EntryList> data_range(std::initializer_list<Pair> list = {});
  void append(const Entry&);

  //retrieve axis-values for given dimension (can be precalculated energies)
  std::vector<double> axis_values(uint16_t dimension) const;

  //export to some format (factory keeps track of file types)
  bool write_file(std::string dir, std::string format) const;
  bool read_file(std::string name, std::string format);

  //Metadata
  Metadata metadata() const;
  void reset_changed();
  bool changed() const;

  //Convenience functions for most common metadata
  std::string type() const;
  uint16_t dimensions() const;

  //Change metadata
  void set_attribute(const Setting &setting);
  void set_attributes(const Setting &settings);
  void set_detectors(const std::vector<Qpx::Detector>& dets);

protected:
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////THIS IS THE MEAT////////////////
  ///implement these to make custom types///
  //////////////////////////////////////////
  
  virtual std::string my_type() const = 0;
  virtual bool _initialize();
  virtual void _recalc_axes() = 0;

  virtual void _set_detectors(const std::vector<Qpx::Detector>& dets) = 0;
  virtual void _push_spill(const Spill&);
  virtual void _push_hit(const Hit&) = 0;
  virtual void _push_stats(const StatsUpdate&) = 0;
  virtual void _flush() {}

  virtual PreciseFloat _data(std::initializer_list<size_t>) const {return 0;}
  virtual std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair>)
    { return std::unique_ptr<std::list<Entry>>(new std::list<Entry>); }
  virtual void _append(const Entry&) {}

  virtual bool _write_file(std::string, std::string) const {return false;}
  virtual bool _read_file(std::string, std::string) {return false;}

  virtual std::string _data_to_xml() const = 0;
  virtual uint16_t _data_from_xml(const std::string&) = 0;

  virtual void _save_data(boost::archive::binary_oarchive&) const {}
  virtual void _load_data(boost::archive::binary_iarchive&) {}

};

typedef std::shared_ptr<Sink> SinkPtr;

}

#endif
