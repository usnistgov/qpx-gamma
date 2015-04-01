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
 *      Pixie::Simulator generates events based on discrete distribution
 *
 ******************************************************************************/

#ifndef PIXIE_SIMULATOR
#define PIXIE_SIMULATOR

#include <vector>
#include <array>
#include <ctime>
#include <unordered_map>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include "spectra_set.h"

namespace Pixie {

class Simulator {
 public:
  Simulator(): resolution_(0), shift_by_(0), gen(std::time(0)) {}
  Simulator(SpectraSet* all_spectra, std::array<int,2> chans,
            int source_res, int dest_res);

  Hit   getHit();
  StatsUpdate getBlock(double duration);

  uint64_t OCR;
  Settings settings;
  double lab_time, time_factor;
  
 protected:
  boost::random::discrete_distribution<> dist_;
  boost::random::discrete_distribution<> refined_dist_;
  boost::random::mt19937 gen;
  
  std::array<int,2> channels_;
  uint16_t shift_by_;
  uint64_t resolution_;
  PreciseFloat count_;

};

}

#endif
