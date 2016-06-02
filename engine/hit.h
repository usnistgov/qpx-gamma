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
 *      Types for organizing data aquired from device
 *        Qpx::Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#ifndef QPX_HIT
#define QPX_HIT

#include <vector>
#include <map>
#include <fstream>

#include "digitized_value.h"
#include "time_stamp.h"
#include "xmlable.h"

namespace Qpx {

struct Hit {

  int16_t       source_channel;
  TimeStamp     timestamp;
  DigitizedVal  energy;
  std::map<std::string, uint16_t> extras; //does not save to file
  std::vector<uint16_t> trace;
  
  inline Hit()
  {
    source_channel = -1;
  }

  inline bool operator==(const Hit other) const
  {
    if (source_channel != other.source_channel) return false;
    if (timestamp != other.timestamp) return false;
    if (energy != other.energy) return false;
    if (extras != other.extras) return false;
    if (trace != other.trace) return false;
    return true;
  }

  inline bool operator!=(const Hit other) const
  {
    return !operator==(other);
  }

  inline bool operator<(const Hit other) const
  {
    return (timestamp < other.timestamp);
  }

  inline bool operator>(const Hit other) const
  {
    return (timestamp > other.timestamp);
  }

  inline void write_bin(std::ofstream &outfile) const
  {
    outfile.write((char*)&source_channel, sizeof(source_channel));
    timestamp.write_bin(outfile);
    uint16_t nrg = energy.val(energy.bits());
    outfile.write((char*)&nrg, sizeof(nrg));
    if (trace.size())
      outfile.write((char*)trace.data(), sizeof(uint16_t) * trace.size());
  }

  inline void read_bin(std::ifstream &infile, std::map<int16_t, Hit> &model_hits)
  {
    int16_t channel = -1;
    infile.read(reinterpret_cast<char*>(&channel), sizeof(channel));
    if (!model_hits.count(channel))
      return;
    *this = model_hits[channel];
    timestamp.read_bin(infile);

    uint16_t nrg = 0;
    infile.read(reinterpret_cast<char*>(&nrg), sizeof(nrg));
    energy.set_val(nrg);
    if (trace.size())
      infile.read(reinterpret_cast<char*>(trace.data()), sizeof(uint16_t) * trace.size());
  }

  void from_xml(const pugi::xml_node &);
  void to_xml(pugi::xml_node &) const;
  std::string xml_element_name() const {return "Hit";}
  std::string to_string() const;

};

}

#endif
