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
 *      Qpx::Spectrum::SpectrumRaw to output custom list mode to file
 *
 ******************************************************************************/

#ifndef SPECTRUM_RAW_H
#define SPECTRUM_RAW_H

#include "spectrum.h"

namespace Qpx {
namespace Spectrum {

class SpectrumRaw : public Spectrum
{
public:
  SpectrumRaw() : open_bin_(false), open_xml_(false), events_this_spill_(0) {}
  ~SpectrumRaw();

  static Metadata get_prototype() {
    Metadata new_temp("Raw", "Custom gated list mode to file. Please provide path and name for valid and accessible file", 0,
                      {}, {});
    populate_options(new_temp.attributes);
    return new_temp;
  }
  

protected:
  std::string my_type() const override {return "Raw";}
  Qpx::Setting default_settings() const override {return this->get_prototype().attributes; }
  static void populate_options(Setting &);

  //1D is ok with all patterns
  bool initialize() override;

  PreciseFloat _get_count(std::initializer_list<uint16_t> list) const {return 0;}
  std::unique_ptr<std::list<Entry>> _get_spectrum(std::initializer_list<Pair> list);

  //event processing
  void addEvent(const Event&) override;
  void addStats(const StatsUpdate&) override;
  void addRun(const RunInfo&) override;
  void _closeAcquisition() override;

  bool init_text();
  bool init_bin();

  void hit_bin(const Event&);

  void stats_text(const StatsUpdate&);
  //void stats_bin(const StatsUpdate&);

  void run_text(const RunInfo&);
  //void run_bin(const RunInfo&);

  std::string _channels_to_xml() const override {return "written to file";}
  uint16_t _channels_from_xml(const std::string&) override {return 0;}

  std::string file_dir_;
  std::string file_name_bin_;
  std::string file_name_txt_;

  std::ofstream file_bin_;
  bool open_xml_, open_bin_;
  pugi::xml_document xml_doc_;
  pugi::xml_node xml_root_;

  bool with_hit_pattern_;
  uint64_t events_this_spill_, total_events_;
};

}}

#endif // SPECTRUM1D_H
