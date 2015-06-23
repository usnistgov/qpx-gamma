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
 *      Pixie::Spectrum::SpectrumRaw to output custom list mode to file
 *
 ******************************************************************************/

#ifndef PIXIE_SPECTRUM_RAW_H
#define PIXIE_SPECTRUM_RAW_H

#include "spectrum.h"

namespace Pixie {
namespace Spectrum {

class SpectrumRaw : public Spectrum
{
public:
  SpectrumRaw() : open_(false) {}
  ~SpectrumRaw();

  static Template get_template() {
    Template new_temp;
    new_temp.type = "Raw";
    //    new_temp.input_types = {""};
    //    new_temp.output_types = {""};
    new_temp.description = "Custom gated list mode to file. Please provide path and name for valid and accessible file.";

    Setting file_setting;
    file_setting.name = "file_name";
    file_setting.node_type = NodeType::setting;
    file_setting.setting_type = SettingType::text;
    file_setting.writable = true;
    file_setting.description = "path and filename for outputting acquisition data";
    new_temp.generic_attributes.push_back(file_setting);

    Setting format_setting;
    format_setting.name = "format";
    format_setting.node_type = NodeType::setting;
    format_setting.setting_type = SettingType::int_menu;
    format_setting.writable = true;
    format_setting.description = "output file format";
    format_setting.value_int = 1;
    format_setting.int_menu_items[0] = "binary";
    format_setting.int_menu_items[1] = "human readable";
    new_temp.generic_attributes.push_back(format_setting);
    
    Setting hit_pattern_write;
    hit_pattern_write.name = "with_pattern";
    hit_pattern_write.node_type = NodeType::setting;
    hit_pattern_write.setting_type = SettingType::boolean;
    hit_pattern_write.writable = true;
    hit_pattern_write.description = "write hit pattern before event energies";
    hit_pattern_write.value_int = 1;
    new_temp.generic_attributes.push_back(hit_pattern_write);

    return new_temp;
  }
  

protected:
  std::string my_type() const override {return "Raw";}

  //1D is ok with all patterns
  bool initialize() override;

  void init_text();
  void init_bin();

  uint64_t _get_count(std::initializer_list<uint16_t> list) const {return 0;}
  std::unique_ptr<std::list<Entry>> _get_spectrum(std::initializer_list<Pair> list);

  //event processing
  void addHit(const Hit&) override;
  void addStats(const StatsUpdate&) override;
  void addRun(const RunInfo&) override;

  void hit_text(const Hit&);
  void hit_bin(const Hit&);

  void stats_text(const StatsUpdate&);
  void stats_bin(const StatsUpdate&);

  void run_text(const RunInfo&);
  void run_bin(const RunInfo&);

  std::string _channels_to_xml() const override {return "written to file";}
  uint16_t _channels_from_xml(const std::string&) override {return 0;}

  std::string file_name_;
  std::ofstream out_file_;
  bool open_;
  int format_;
  bool with_hit_pattern_;
};

}}

#endif // PIXIE_SPECTRUM1D_H
