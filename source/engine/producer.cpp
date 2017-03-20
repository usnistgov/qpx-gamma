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
 *      Qpx::Producer abstract class
 *
 ******************************************************************************/

#include "producer.h"

namespace Qpx {

  bool Producer::load_setting_definitions(std::string file) {
    pugi::xml_document doc;

    if (!doc.load_file(file.c_str()))
      return false;

    pugi::xml_node root = doc.first_child();
    if (!root)
      return false;

    for (pugi::xml_node node : root.children()) {
      if (node.name() && (std::string(node.name()) == SettingMeta().xml_element_name())) {
        SettingMeta newset(node);
        if (newset != SettingMeta())
          setting_definitions_[newset.id_] = newset;
      }
    }

    if (!setting_definitions_.empty()) {
      DBG << "<Producer> " << this->device_name() << " retrieved " << setting_definitions_.size() << " setting definitions";
      profile_path_ = file;
      return true;
    } else {
      DBG << "<Producer> " << this->device_name() << " failed to load setting definitions";
      profile_path_.clear();
      return false;
    }
  }


  bool Producer::save_setting_definitions(std::string file) {
    pugi::xml_document doc;

    pugi::xml_node root = doc.append_child();
    root.set_name("SettingDefinitions");
    for (auto &q : setting_definitions_)
      q.second.to_xml(root);

    return doc.save_file(file.c_str());
  }

}

