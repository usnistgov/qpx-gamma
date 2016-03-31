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
 *      Qpx::SinkRaw to output custom list mode to file
 *
 ******************************************************************************/


#include <fstream>
#include <algorithm>
#include "spectrum_raw.h"
#include "daq_sink_factory.h"

namespace Qpx {

static SinkRegistrar<SpectrumRaw> registrar("Raw");

SpectrumRaw::SpectrumRaw()
  : open_bin_(false)
  , open_xml_(false)
  , hits_this_spill_(0)
  , total_hits_(0)
{
  Setting base_options = metadata_.attributes;
  metadata_ = Metadata("Raw", "Custom gated list mode to file. Please provide path and name for valid and accessible file", 0,
                    {}, {});
  metadata_.attributes = base_options;

  Qpx::Setting file_setting;
  file_setting.id_ = "file_dir";
  file_setting.metadata.setting_type = Qpx::SettingType::dir_path;
  file_setting.metadata.writable = true;
  file_setting.metadata.flags.insert("preset");
  file_setting.metadata.description = "path to temp output directory";
  metadata_.attributes.branches.add(file_setting);
}

SpectrumRaw::~SpectrumRaw() {
  _flush();
}

bool SpectrumRaw::_initialize() {
  Spectrum::_initialize();

  file_dir_ = get_attr("file_dir").value_text;
  PL_DBG << "trying to create " << file_dir_;
  if (file_dir_.empty())
    return false;
  PL_DBG << "raw: file dir not empty";

  return init_bin();
}

bool SpectrumRaw::init_text() {
  file_name_txt_ = file_dir_ + "/qpx_out.xml";
  xml_root_ = xml_doc_.append_child("QpxListData");

  if (metadata_.attributes.branches.size())
    metadata_.attributes.to_xml(xml_root_);

  open_xml_ = true;
  return true;
}

bool SpectrumRaw::init_bin() {
  file_name_bin_ = file_dir_ + "/qpx_out.bin";

  file_bin_.open(file_name_bin_, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  if (!file_bin_.is_open())
    return false;

  PL_DBG << "successfully opened binary";

  if (!file_bin_.good() || !init_text()) {
    file_bin_.close();
    return false;
  }

  PL_DBG << "binary is good";

  open_bin_ = true;
  return true;
}

void SpectrumRaw::addEvent(const Event& newEvent) {
  if (!open_bin_)
    return;

  std::multiset<Hit> all_hits;
  for (auto &q : newEvent.hits)
    all_hits.insert(q.second);

  for (auto &q : all_hits) {
    q.write_bin(file_bin_);
    hits_this_spill_++;
  }
}

void SpectrumRaw::_push_spill(const Spill& one_spill) {
  Spectrum::_push_spill(one_spill);

  if (!open_xml_)
    return;

  Spill copy = one_spill;
  copy.hits.resize(hits_this_spill_);
  total_hits_ += hits_this_spill_;
  hits_this_spill_ = 0;

  copy.to_xml(xml_root_, true);
}


void SpectrumRaw::_flush() {
  if (open_xml_) {
    PL_DBG << "<SpectrumRaw> writing " << file_name_txt_ << " for \"" << metadata_.name << "\"";
    if (!xml_doc_.save_file(file_name_txt_.c_str()))
      PL_ERR << "<SpectrumRaw> Failed to save " << file_name_txt_;
    open_xml_ = false;
  }
  if (open_bin_) {
    PL_DBG << "<SpectrumRaw> closing " << file_name_bin_ << " for \"" << metadata_.name << "\"";
    file_bin_.close();
    open_bin_ = false;
  }
}

}
