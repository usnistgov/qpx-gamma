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

#ifndef QPX_EXPERIMENT
#define QPX_EXPERIMENT

#include "project.h"
#include <set>
#include <string>

namespace Qpx {

enum DomainType
{
  source, sink, manual
};

struct Domain
{
  DomainType  type;
  SettingMeta range;
};

struct DomainPoint
{
  Setting domain_value;
  int64_t data_idx;
  Domain  subdomain;
};

struct Trajectory
{
  Domain domain_;

  std::list<DomainPoint> points;

};


class ExperimentProject
{
//  XMLableDB<Qpx::Metadata> base_prototypes;
  Trajectory root_trajectory;

  std::map<int64_t, ProjectPtr> acquisitions;
};



}

#endif
