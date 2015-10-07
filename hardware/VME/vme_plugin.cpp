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
 *      QpxVmePlugin
 *
 ******************************************************************************/

#include "vme_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common_xia.h"
#include "custom_logger.h"
#include "custom_timer.h"


namespace Qpx {

static DeviceRegistrar<QpxVmePlugin> registrar("Pixie4");

QpxVmePlugin::QpxVmePlugin() {

  live_ = LiveStatus::dead;

}


QpxVmePlugin::~QpxVmePlugin() {

}


bool QpxVmePlugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {

    }
  }
  return true;
}

void QpxVmePlugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxVmePlugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (live_ == LiveStatus::history)
    return false;

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {

  }
  return true;
}

bool QpxVmePlugin::execute_command(Gamma::Setting &set) {
  if (live_ == LiveStatus::history)
    return false;

  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::command) && (q.value_dbl == 1)) {
        q.value_dbl = 0;
        if (q.id_ == "Vme/Iseg/Sing a song")
          return true;
      }
    }
  }

  return false;
}


bool QpxVmePlugin::boot() {
  if (live_ == LiveStatus::history) {
    PL_WARN << "Cannot boot VME system. Improper initialization of parameters in wrapper. Or boot from history...?";
    return false;
  }

  live_ = LiveStatus::dead;
}

void QpxVmePlugin::get_all_settings() {
}



}
