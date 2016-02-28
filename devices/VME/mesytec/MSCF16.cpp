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
 *      MSCF16
 *
 ******************************************************************************/

#include "MSCF16.h"

namespace Qpx {

static DeviceRegistrar<MSCF16> registrar("VME/MesytecRC/MSCF16");

MSCF16::MSCF16() {
  controller_ = nullptr;
  modnum_ = 0;
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  module_ID_code_ = -1;
}


MSCF16::~MSCF16() {
  die();
}

void MSCF16::rebuild_structure(Qpx::Setting &set) {
  Qpx::Setting group("VME/MesytecRC/MSCF16/Group");
  group.enrich(setting_definitions_, true);
  Qpx::Setting chan("VME/MesytecRC/MSCF16/Channel");
  chan.enrich(setting_definitions_, true);

  std::vector<Qpx::Setting> groups;
  for (auto &k : set.branches.my_data_)
    if ((k.metadata.setting_type == Qpx::SettingType::stem) && (k.id_ == "VME/MesytecRC/MSCF16/Group"))
      groups.push_back(k);

  while (groups.size() < 4)
    groups.push_back(group);
  while (groups.size() > 4)
    groups.pop_back();

  int i=0;
  for (auto &gr : groups) {
    std::vector<Qpx::Setting> channels;
    std::set<int32_t> indices;

    gr.metadata.name += " " + std::to_string(i+1);
    for (auto &k : gr.branches.my_data_) {
      if ((k.metadata.setting_type == Qpx::SettingType::stem) && (k.id_ == "VME/MesytecRC/MSCF16/Channel"))
        channels.push_back(k);
      else
        k.metadata.address += i;
    }

    while (channels.size() < 4)
      channels.push_back(chan);
    while (groups.size() > 4)
      channels.pop_back();

    int j=0;
    for (auto &ch : channels) {
      ch.metadata.name += " " + std::to_string(i*4 + j + 1);
      if (ch.indices.size() > 1) {
        int32_t idx = *ch.indices.begin();
        ch.indices.clear();
        ch.indices.insert(idx);
      }
      for (auto &m : ch.branches.my_data_) {
        m.metadata.address += i*4 + j;
        m.indices = ch.indices;
      }
      for (auto &idx : ch.indices)
        indices.insert(idx);
      j++;
    }

    while (gr.branches.has_a(chan))
      gr.branches.remove_a(chan);

    for (auto &q : channels)
      gr.branches.add_a(q);

    gr.indices = indices;

    for (auto &k : gr.branches.my_data_) {
      if (k.metadata.setting_type != Qpx::SettingType::stem)
        k.indices = indices;
    }

    i++;
  }

  while (set.branches.has_a(group))
    set.branches.remove_a(group);

  for (auto &q : groups)
    set.branches.add_a(q);

}

}
