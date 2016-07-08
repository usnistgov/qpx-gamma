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
 *        Hit        single energy event with coincidence flags
 *
 ******************************************************************************/

#include "experiment.h"
#include "custom_logger.h"
#include "UncertainDouble.h"

namespace Qpx {

void ExperimentProject::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  //  if (base_prototypes.empty())
  //    return;

  base_prototypes.to_xml(node);
  if (root_trajectory)
    root_trajectory->to_xml(node);

  if (!data.empty())
  {
    pugi::xml_node datanode = node.append_child("Data");

    for (auto &a : data)
    {
      pugi::xml_node projnode  = datanode.append_child(a.second->xml_element_name().c_str());
      projnode.append_attribute("idx").set_value((long long)a.first);
      a.second->to_xml(projnode);
    }
  }

  changed_ = false;
}


void ExperimentProject::from_xml(const pugi::xml_node &node)
{
  base_prototypes.clear();
  if (node.child(base_prototypes.xml_element_name().c_str()))
    base_prototypes.from_xml(node.child(base_prototypes.xml_element_name().c_str()));

  if (node.child(TrajectoryNode().xml_element_name().c_str()))
  {
    root_trajectory = std::shared_ptr<TrajectoryNode>(new TrajectoryNode());
    root_trajectory->from_xml(node.child(TrajectoryNode().xml_element_name().c_str()));
  }

  next_idx = 1;

  if (node.child("Data"))
  {
    for (auto n : node.child("Data").children())
    {
      int64_t idx = -1;
      if (n.attribute("idx"))
        idx = n.attribute("idx").as_llong();
      if (idx >= 0)
      {
        ProjectPtr proj = ProjectPtr(new Project());
        proj->from_xml(n, true, true);
        data[idx] = proj;
        next_idx = idx + 1;
      }
    }
  }

  std::list<TrajectoryPtr> leafs;
  find_leafs(leafs, root_trajectory);
  for (auto &l : leafs)
  {
    if (!data.count(l->data_idx))
      continue;

    DataPoint dp;
    dp.node = l;
    dp.idx_proj = l->data_idx;
    gather_vars_recursive(dp, l);
    for (auto &s : data[l->data_idx]->get_sinks())
      if (s.second) {
        dp.idx_sink = s.first;
        dp.spectrum_info = s.second->metadata();
        results_.push_back(dp);
      }
  }

  gather_results();
  changed_ = false;
}

void ExperimentProject::find_leafs(std::list<TrajectoryPtr> &list, TrajectoryPtr node)
{
  if (!node)
    return;
  if ((node->data_idx >= 0) && !node->childCount())
    list.push_back(node);
  else
    for (int i = 0; i < node->childCount(); i++)
      find_leafs(list, node->getChild(i));
}


TrajectoryPtr ExperimentProject::next_setting()
{
  if (root_trajectory)
    return root_trajectory->next_setting();
  else
    return nullptr;
}

bool ExperimentProject::push_next_setting(TrajectoryPtr node)
{
  if (!node)
    return false;

  TrajectoryPtr parent = node->getParent();
  if (!parent)
    return false;

  TrajectoryPtr prev;
  if (parent->childCount())
    prev = parent->getChild(parent->childCount()-1);


  if (prev && (prev->domain_value.number() >= node->domain_value.number()))
  {
    LINFO << "<Experiment> Value hit hard limit " << node->to_string();
    prev->domain_value.metadata.maximum = prev->domain_value.number();
    return false;
  }
  else if (node && !node->childCount() && (node->domain.type == none))
  {

    XMLableDB<Metadata> prototypes = base_prototypes;
    set_sink_vars_recursive(prototypes, node);

    data[next_idx] = ProjectPtr(new Project());
    data[next_idx]->set_prototypes(prototypes);
    node->data_idx = next_idx;

    DataPoint dp;
    dp.node = node;
    dp.idx_proj = next_idx;
    gather_vars_recursive(dp, node);
    for (auto &s : data[next_idx]->get_sinks())
      if (s.second) {
        dp.idx_sink = s.first;
        dp.spectrum_info = s.second->metadata();
        results_.push_back(dp);
      }

    next_idx++;
  }

  parent->push_back(*node);
  changed_ = true;
  return true;
}


void ExperimentProject::set_sink_vars_recursive(XMLableDB<Metadata>& prototypes,
                                                TrajectoryPtr node)
{
  if (!node)
    return;
  TrajectoryPtr parent = node->getParent();
  if (!parent)
    return;
  set_sink_vars_recursive(prototypes, parent);

  if (parent->domain.type == DomainType::sink)
    for (auto &p : prototypes.my_data_)
      p.set_attribute(node->domain_value);
}

void ExperimentProject::gather_vars_recursive(DataPoint& dp, TrajectoryPtr node)
{
  if (!node)
    return;
  TrajectoryPtr parent = node->getParent();
  if (!parent)
    return;
  gather_vars_recursive(dp, parent);

  if (parent->domain.type == DomainType::sink) {
    if (dp.spectrum_info.attributes().has(node->domain_value, Match::id | Match::indices))
    {
      dp.domains[parent->domain.verbose] = node->domain_value;
    }
  }
  else if (parent->domain.type != DomainType::none) {
    dp.domains[parent->domain.verbose] = node->domain_value;
  }
}

ProjectPtr ExperimentProject::get_data(int64_t i) const
{
  if (data.count(i))
    return data.at(i);
  else
    return nullptr;
}

void ExperimentProject::delete_data(int64_t i)
{
  if (!data.count(i))
    return;

  data.erase(i);
  changed_ = true;
  std::vector<DataPoint> results;
  for (auto &r : results_)
    if (r.idx_proj != i)
      results.push_back(r);
  results_ = results;
}

void ExperimentProject::gather_results()
{
  if (results_.empty())
    return;

  for (auto &r : results_)
  {
    r.domains.clear();
    if (r.node)
      gather_vars_recursive(r, r.node);
    //    DBG << "domains found " << r.domains.size();
    if (data.count(r.idx_proj) && data.at(r.idx_proj))
    {
      if (data.at(r.idx_proj)->has_fitter(r.idx_sink))
      {
        Fitter f = data.at(r.idx_proj)->get_fitter(r.idx_sink);
        r.spectrum_info = f.metadata_;
        std::set<double> sel = f.get_selected_peaks();
        if (sel.size() && f.contains_peak(*sel.begin()))
          r.selected_peak = f.peak(*sel.begin());
      }
    }
  }
}

bool ExperimentProject::empty() const
{
  if (!data.empty())
    return false;
  if (!root_trajectory)
    return true;
  if (root_trajectory->childCount() > 1)
    return false;
  if (root_trajectory->childCount() < 1)
    return true;
  if (root_trajectory->getChild(0)->childCount())
    return false;
  return true;
}

bool ExperimentProject::has_results() const
{
  return results_.size();
}

bool ExperimentProject::done() const
{
  if (!root_trajectory)
    return false;
  return !(root_trajectory->next_setting()).operator bool();
}

bool ExperimentProject::changed() const
{
  for (auto &q : data)
    if (q.second->changed())
      changed_ = true;

  return changed_;
}

void ExperimentProject::notity_tree_change()
{
  changed_ = true;
}

double ExperimentProject::estimate_total_time()
{
  if (!root_trajectory)
    return 0;
  else
    return root_trajectory->estimated_time();
}

double ExperimentProject::total_real_time()
{
  if (!root_trajectory)
    return 0;
  else
    return tally_real_time(root_trajectory);
}

double ExperimentProject::tally_real_time(TrajectoryPtr node)
{
  if (!node)
    return 0;
  if (data.count(node->data_idx))
  {
    ProjectPtr proj = data.at(node->data_idx);
    if (!proj)
      return 0;
    for (auto &s : proj->get_sinks())
      if (s.second)
      {
        Metadata md = s.second->metadata();
        return md.get_attribute("real_time").value_duration.total_milliseconds() * 0.001;
      }
    return 0;
  }

  double tally = 0;
  for (int i=0; i < node->childCount(); ++i)
    tally += tally_real_time(node->getChild(i));
  return tally;
}

}
