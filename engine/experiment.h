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

#pragma once

#include "project.h"
#include "domain.h"

namespace Qpx {

struct DataPoint
{
  Metadata spectrum_info;
  Peak     selected_peak;
  std::map<std::string, Setting> domains;
  int64_t idx_proj;
  int64_t idx_sink;
  TrajectoryPtr node;

  UncertainDouble independent_variable;
  UncertainDouble independent_variable2;
  UncertainDouble dependent_variable;
};


class ExperimentProject
{
public:
  ExperimentProject() : base_prototypes("SinkPrototypes"), changed_(false)
  {
    root_trajectory = std::shared_ptr<Qpx::TrajectoryNode>(new Qpx::TrajectoryNode());
    Qpx::TrajectoryNode tn(root_trajectory);
    tn.domain.verbose = "root";
    root_trajectory->push_back(tn);
    next_idx = 1;
  }

  bool empty() const;
  bool has_results() const;
  bool done() const;
  bool changed() const;
  void notity_tree_change();

  double estimate_total_time();
  double total_real_time();

  bool push_next_setting(Qpx::TrajectoryPtr node);

  TrajectoryPtr get_trajectories() const {return root_trajectory;}
  ProjectPtr get_data(int64_t i) const;
  void delete_data(int64_t);

  void set_prototypes(XMLableDB<Qpx::Metadata> ptp) { base_prototypes = ptp; changed_ = true; }
  XMLableDB<Qpx::Metadata> get_prototypes() const { return base_prototypes; }

  TrajectoryPtr next_setting();

  void gather_results();
  std::vector<DataPoint> results() const {return results_;}

  std::string xml_element_name() const {return "QpxExperiment";}
  void to_xml(pugi::xml_node &node) const;
  void from_xml(const pugi::xml_node &node);

private:
  XMLableDB<Qpx::Metadata> base_prototypes;
  std::shared_ptr<TrajectoryNode> root_trajectory;
  std::map<int64_t, ProjectPtr> data;

  std::vector<DataPoint> results_;

  int64_t       next_idx;
  std::string   identity_;
  mutable bool  changed_;

  void set_sink_vars_recursive(XMLableDB<Qpx::Metadata>& prototypes, TrajectoryPtr node);
  void gather_vars_recursive(DataPoint& dp, TrajectoryPtr node);
  void find_leafs(std::list<TrajectoryPtr> &list, TrajectoryPtr node);
  double tally_real_time(TrajectoryPtr node);
};

}
