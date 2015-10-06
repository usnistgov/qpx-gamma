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
 *      Qpx::Sorter generates spills from list files
 *
 ******************************************************************************/

#ifndef PIXIE_SORTER
#define PIXIE_SORTER

#include "daq_types.h"

#include <list>

namespace Qpx {

class Sorter {
 public:
  Sorter(): valid_(false), open_bin_(false) {}
  Sorter(std::string name_xml);
  ~Sorter();

  Spill get_spill();

  bool valid() {return valid_;}
  void order(std::string name_out);

  uint64_t OCR;
  
 protected:
  std::string file_name_xml_;
  std::string file_name_bin_;

  std::ifstream  file_bin_;
  FILE          *file_xml_;
  std::streampos bin_begin_, bin_end_;

  //tinyxml2::XMLElement* root_;

  std::list<StatsUpdate> spills_;
  RunInfo start_, end_;
  
  bool open_bin_;

  //PreciseFloat count_;
  bool valid_;
};

}

#endif
