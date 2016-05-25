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
 *        Qpx::Hit        single energy event with coincidence flags
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

}

void ExperimentProject::from_xml(const pugi::xml_node &node)
{
  if (node.child(base_prototypes.xml_element_name().c_str()))
    base_prototypes.from_xml(node.child(base_prototypes.xml_element_name().c_str()));

  if (node.child(TrajectoryNode().xml_element_name().c_str()))
  {
    root_trajectory = std::shared_ptr<Qpx::TrajectoryNode>(new Qpx::TrajectoryNode());
    root_trajectory->from_xml(node.child(TrajectoryNode().xml_element_name().c_str()));
  }

  next_idx = 0;

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
}

std::pair<DomainType, TrajectoryPtr> ExperimentProject::next_setting()
{
  std::pair<DomainType, TrajectoryPtr> ret(none, nullptr);
  if (root_trajectory)
    ret = root_trajectory->next_setting();
  if (ret.second && !ret.second->childCount() && (ret.second->domain.type == none))
  {
    DBG << "Experiment generating new data point with " << base_prototypes.size() << " sinks";

    XMLableDB<Qpx::Metadata> prototypes = base_prototypes;
    set_sink_vars_recursive(prototypes, ret.second);

    data[next_idx] = ProjectPtr(new Project());
    data[next_idx]->set_prototypes(prototypes);
    ret.second->data_idx = next_idx;
    next_idx++;

  }
  return ret;
}

void ExperimentProject::set_sink_vars_recursive(XMLableDB<Qpx::Metadata>& prototypes,
                                                TrajectoryPtr node)
{
  if (!node)
    return;
  TrajectoryPtr parent = node->getParent();
  if (!parent)
    return;
  set_sink_vars_recursive(prototypes, parent);

  if (parent->domain.type == Qpx::DomainType::sink)
  {
    for (auto &p : prototypes.my_data_)
    {
      if (p.attributes.has(node->domain_value, Qpx::Match::id | Qpx::Match::indices))
      {
        DBG << "Sink prototype " << p.name << " " << p.type() << " " << " has "
            << node->domain_value.id_;
        p.attributes.set_setting_r(node->domain_value, Qpx::Match::id | Qpx::Match::indices);
      }
    }
  }
}

ProjectPtr ExperimentProject::get_data(int64_t i) const
{
  if (data.count(i))
    return data.at(i);
  else
    return nullptr;
}

ProjectPtr ExperimentProject::next_project()
{

}


}
