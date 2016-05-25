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
#include "domain.h"

namespace Qpx {

class ExperimentProject
{
public:
  ExperimentProject() : base_prototypes("SinkPrototypes")
  {
    root_trajectory = std::shared_ptr<Qpx::TrajectoryNode>(new Qpx::TrajectoryNode());
    Qpx::TrajectoryNode tn(root_trajectory);
    tn.domain.verbose = "root";
    root_trajectory->push_back(tn);
    next_idx = 0;
  }

  TrajectoryPtr get_trajectories() const {return root_trajectory;}
  ProjectPtr get_data(int64_t i) const;

  void set_prototype(Qpx::Metadata ptp) { base_prototypes.clear(); base_prototypes.add(ptp); }

  std::pair<DomainType, TrajectoryPtr> next_setting();
  ProjectPtr next_project();

  std::string xml_element_name() const {return "QpxExperiment";}
  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node);

private:
  XMLableDB<Qpx::Metadata> base_prototypes;
  std::shared_ptr<TrajectoryNode> root_trajectory;

  void set_sink_vars_recursive(XMLableDB<Qpx::Metadata>& prototypes, TrajectoryPtr node);

  std::map<int64_t, ProjectPtr> data;
  int64_t next_idx;

};



}

#endif
