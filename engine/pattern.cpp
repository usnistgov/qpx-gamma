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
 *      Types for organizing data aquired from Device
 *        Qpx::Pattern        
 *
 ******************************************************************************/

#include "pattern.h"
#include <boost/algorithm/string.hpp>
#include <sstream>

namespace Qpx {

void Pattern::resize(size_t sz) {
  gates_.resize(sz);
  if (threshold_ > sz)
    threshold_ = sz;
}

void Pattern::set_gates(std::vector<bool> gts) {
  gates_ = gts;
  if (threshold_ > gates_.size())
    threshold_ = gates_.size();
}

void Pattern::set_theshold(size_t sz) {
  threshold_ = sz;
  if (threshold_ > gates_.size())
    threshold_ = gates_.size();
}

bool Pattern::validate(const Event &e) const
{
  bool ret = false;
  return ret;
}

bool Pattern::operator==(const Pattern other) const {
  if (gates_ != other.gates_)
    return false;
  if (threshold_ != other.threshold_)
    return false;
  return true;
}

std::string Pattern::gates_to_string() const
{
  std::string gts;
  for (bool g : gates_)
    if (g)
      gts += "+";
    else
      gts += "o";
  return gts;
}

std::vector<bool> Pattern::gates_from_string(std::string s)
{
  std::vector<bool> gts;
  if (s.size()) {
    boost::algorithm::trim(s);
    if (s.size()) {
      for (char &c : s)
        if (c == '+')
          gts.push_back(true);
        else
          gts.push_back(false);
    }
  }
  return gts;
}


std::string Pattern::to_string() const
{
  std::stringstream ss;
  ss << threshold_;
  return ss.str() + gates_to_string();
}

void Pattern::from_string(std::string s)
{
  threshold_ = 0;
  std::stringstream ss(s);
  ss >> threshold_;
  std::string gts;
  ss >> gts;
  gates_ = gates_from_string(gts);
}


}
