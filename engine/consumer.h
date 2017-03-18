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
 *      Qpx::Consumer generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::ConsumerFactory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::ConsumerRegistrar for registering new spectrum types.
 *
 ******************************************************************************/

#pragma once

#include "consumer_metadata.h"
#include "spill.h"
#include <initializer_list>
#include <boost/thread.hpp>

#include "json.hpp"
using namespace nlohmann;

#ifdef H5_ENABLED
#include "H5CC_Group.h"
#endif

namespace Qpx {

typedef std::pair<std::vector<size_t>, PreciseFloat> Entry;
typedef std::list<Entry> EntryList;
typedef std::pair<size_t, size_t> Pair;


class Consumer
{
protected:
  ConsumerMetadata metadata_;
  std::vector<std::vector<double> > axes_;

  mutable boost::shared_mutex shared_mutex_;
  mutable boost::mutex unique_mutex_;
  bool changed_;

public:
  Consumer();
  Consumer(const Consumer& other)
    : metadata_(other.metadata_)
    , axes_ (other.axes_) {}
  virtual Consumer* clone() const = 0;
  virtual ~Consumer() {}

  //named constructors, used by factory
  bool from_prototype(const ConsumerMetadata&);
  bool load(const pugi::xml_node &);
  void save(pugi::xml_node &) const;

  #ifdef H5_ENABLED
  virtual bool load(H5CC::Group&, bool withdata);
  virtual bool save(H5CC::Group&) const;
  #endif

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

  //ConsumerMetadata
  ConsumerMetadata metadata() const;
  void reset_changed();
  bool changed() const;

  //Convenience functions for most common metadata
  std::string type() const;
  uint16_t dimensions() const;
  std::string debug() const;

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

  #ifdef H5_ENABLED
  virtual void _load_data(H5CC::Group&) {}
  virtual void _save_data(H5CC::Group&) const {}
  #endif

};

typedef std::shared_ptr<Consumer> SinkPtr;

}
