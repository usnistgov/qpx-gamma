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

#include "domain.h"
#include "custom_logger.h"
#include "UncertainDouble.h"

namespace Qpx {

bool Domain::operator== (const Domain& other) const
{
  return (type == other.type) && (value_range == other.value_range);
}

bool Domain::operator!= (const Domain& other) const
{
  return !operator ==(other);
}


std::string TrajectoryNode::domain_type_to_string(DomainType dt)
{
  if (dt == source)
    return "source";
  else if (dt == sink)
    return "sink";
  else if (dt == manual)
    return "manual";
  else
    return "none";
}

DomainType TrajectoryNode::domain_type_from_string(std::string dt)
{
  if (dt == "source")
    return source;
  else if (dt == "sink")
    return sink;
  else if (dt == "manual")
    return manual;
  else
    return none;
}

std::string TrajectoryNode::criterion_type_to_string(DomainAdvanceCriterion ct)
{
  if (ct == realtime)
    return "realtime";
  else if (ct == livetime)
    return "livetime";
  else if (ct == totalcount)
    return "totalcount";
  else
    return "indefinite";
}

DomainAdvanceCriterion TrajectoryNode::criterion_type_from_string(std::string ct)
{
  if (ct == "realtime")
    return realtime;
  else if (ct == "livetime")
    return livetime;
  else if (ct == "totalcount")
    return totalcount;
  else
    return indefinite;
}


TrajectoryNode::TrajectoryNode(weak_item_t p)
  : parent(p)
  , data_idx(-1)
{
}

TrajectoryNode::TrajectoryNode(weak_item_t p, const TrajectoryNode &other)
  : parent(p)
{
  domain = other.domain;
  domain_value = other.domain_value;
  data_idx = other.data_idx;
  branches = other.branches;
}

std::shared_ptr<TrajectoryNode> TrajectoryNode::deep_copy(weak_item_t p, const TrajectoryNode &other)
{
  std::shared_ptr<TrajectoryNode> ret(new TrajectoryNode(p));
  ret->domain = other.domain;
  ret->domain_value = other.domain_value;
  ret->data_idx = other.data_idx;
  for (auto &b : other.branches)
  {
    if (!b)
      continue;
    ret->branches.push_back(deep_copy(ret, *b));
  }
  return ret;
}


std::string TrajectoryNode::type() const
{
  std::string ret;
  if (domain.type != none)
    ret = "(d) ";
  if (domain_value != Setting())
    ret += "(v) ";
  if (data_idx > -1)
    ret += "+ ";

  if (ret.empty())
    ret = domain.verbose;
  return ret;
}

std::string TrajectoryNode::to_string() const
{
  std::string ret;
  if (domain_value != Setting())
    ret = (domain_value.metadata.name.empty() ? domain_value.id_ : domain_value.metadata.name)
        + " " + domain.value_range.indices_to_string();
//        + " = " + domain_value.val_to_pretty_string()
//        + " " + domain_value.metadata.unit + " ";
  if (ret.empty() && (domain.type != none)) {
    //    if (!ret.empty())
    //      ret += "\n";
    ret += (domain.value_range.metadata.name.empty() ? domain.verbose : domain.value_range.metadata.name)
        + " " + domain.value_range.indices_to_string();
  }
  return ret;
}

std::string TrajectoryNode::range_to_string() const
{
  if ((domain_value == Setting()) && (domain.type != none))
    return domain.value_range.metadata.value_range();
  else if (domain_value != Setting())
    return " = " + domain_value.val_to_pretty_string()
           + " " + domain_value.metadata.unit;
  else
    return "";
}


std::string TrajectoryNode::crit_to_string() const
{
  std::string ret;
  if (branches.empty() && (domain.criterion_type != indefinite))
  {
    if (domain.criterion_type == totalcount)
    {
      UncertainDouble ud = UncertainDouble::from_int(domain.criterion, 0);
      ud.setSigFigs(2);
      ret += " until count > " + ud.to_string();
    }
    else
    {
      Setting st;
      st.metadata.setting_type = SettingType::time_duration;
      st.value_duration = boost::posix_time::seconds(domain.criterion);
      ret += " for " + st.val_to_pretty_string();
//          + " " + criterion_type_to_string(domain.criterion_type);
    }
  }
  return ret;
}


int TrajectoryNode::row() const
{
  if(parent.expired())
    return 0;
  return parent.lock()->childPos( self::shared_from_this());
}

int TrajectoryNode::childPos(const const_item_t& item) const
{
  auto it = std::find(std::begin(branches),std::end(branches),item);
  if(it != branches.end())
    return it - branches.begin();
  return -1;
}

std::shared_ptr<TrajectoryNode> TrajectoryNode::getChild(int row) const
{
  if (row < 0)
    return nullptr;
  size_t i = static_cast<size_t>(row);
  if (i < branches.size())
    return branches.at(i);
  else
    return nullptr;
}

void TrajectoryNode::to_xml(pugi::xml_node &root) const
{
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  pugi::xml_node domain_node = node.append_child("Domain");
  domain_node.append_attribute("description").set_value(domain.verbose.c_str());
  if (domain.type != none) {
    domain_node.append_attribute("type").set_value(domain_type_to_string(domain.type).c_str());
    domain_node.append_attribute("criterion_type").set_value(criterion_type_to_string(domain.criterion_type).c_str());
    domain_node.append_attribute("criterion").set_value((unsigned long long)domain.criterion);
    domain.value_range.to_xml(domain_node);
  }

  if ((domain_value != Setting()) || (data_idx > -1))
  {
    pugi::xml_node value_node = node.append_child("Data");
    value_node.append_attribute("data_idx").set_value((long long)data_idx);
    domain_value.to_xml(value_node);
  }

  if (!branches.empty())
  {
    pugi::xml_node branches_node = node.append_child("Branches");
    for (auto &b : branches) {
      if (b)
        b->to_xml(branches_node);
    }
  }
  //    branches.to_xml(node);
}

void TrajectoryNode::from_xml(const pugi::xml_node &node)
{
  domain = Domain();
  if (node.child("Domain")) {
    pugi::xml_node domain_node = node.child("Domain");

    std::string typestr;
    if (domain_node.attribute("type"))
      typestr = std::string(domain_node.attribute("type").value());
    domain.type = domain_type_from_string(typestr);

    if (domain_node.attribute("description"))
      domain.verbose = std::string(domain_node.attribute("description").value());

    std::string crittype;
    if (domain_node.attribute("criterion_type"))
      crittype = std::string(domain_node.attribute("criterion_type").value());
    domain.criterion_type = criterion_type_from_string(crittype);

    if (domain_node.attribute("criterion"))
      domain.criterion = domain_node.attribute("criterion").as_ullong();

    if (domain_node.child(domain.value_range.xml_element_name().c_str()))
      domain.value_range.from_xml(domain_node.child(domain.value_range.xml_element_name().c_str()));
  }

  if (node.child("Data")) {
    pugi::xml_node data_node = node.child("Data");
    if (data_node.attribute("data_idx"))
      data_idx = data_node.attribute("data_idx").as_llong(-1);

    if (data_node.child(domain_value.xml_element_name().c_str()))
      domain_value.from_xml(data_node.child(domain_value.xml_element_name().c_str()));
  }

  branches.clear();
  if (node.child("Branches"))
  {
    pugi::xml_node branches_node = node.child("Branches");
    for (auto &b : branches_node.children()) {
      item_t i = item_t(new TrajectoryNode(self::shared_from_this()));
      i->from_xml(b);
//      if (i->valid())
        branches.push_back(i);
    }
  }
}

bool TrajectoryNode::operator== (const TrajectoryNode& other) const
{
  return ((domain.type == other.domain.type)
          && (domain.value_range == other.domain.value_range)
          && (domain.criterion == other.domain.criterion)
          && (domain_value == other.domain_value)
          && (data_idx == other.data_idx)
          && (branches == other.branches));
}

bool TrajectoryNode::shallow_equals(const TrajectoryNode& other) const
{
  return ((domain.type == other.domain.type) && domain_value.shallow_equals(other.domain_value));
}


bool TrajectoryNode::valid() const
{
  return ((domain_value != Setting()) || (data_idx > -1) || !branches.empty()
          || (domain.value_range != Qpx::SettingMeta()));
}

void TrajectoryNode::remove_child(int row)
{
  if (row < 0)
    return;
  size_t i = static_cast<size_t>(row);
  if (i < branches.size())
  {
    auto it = std::next(branches.begin(), i);
    branches.erase(it);
  }
}


void TrajectoryNode::replace_child(int row, const TrajectoryNode &t)
{
  if (row < 0)
    return;
  size_t i = static_cast<size_t>(row);
  if (i < branches.size())
    branches[i] = item_t(new TrajectoryNode(self::shared_from_this(), t));
}


void TrajectoryNode::emplace_back(TrajectoryNode &&t)
{
  branches.emplace_back(std::make_shared<TrajectoryNode>(self::shared_from_this(),
                                                         std::forward<TrajectoryNode>(t)));
}

void TrajectoryNode::push_back(const TrajectoryNode &t)
{
  branches.push_back(item_t(new TrajectoryNode(self::shared_from_this(), t)) );
}

void TrajectoryNode::populate()
{
  if (domain.type == none)
    return;
}

double TrajectoryNode::estimated_time()
{
  if (data_idx >= 0)
    return 0;
  if (domain_value != Qpx::Setting())
    return 0;

  double tally = 0;
  for (auto &b : branches)
    if (b && (!b->is_datapoint(domain) || (domain.type == Qpx::DomainType::none)))
      tally += b->estimated_time();
  if (getParent().operator bool() && !has_subdomains())
    tally = domain.criterion;

  double numof = 1;
  if ((domain.type != Qpx::DomainType::none) && (domain.value_range.metadata.step > 0))
    numof = ((domain.value_range.metadata.maximum - domain.value_range.metadata.minimum)
      / domain.value_range.metadata.step) + 1;

  return tally*numof;
}

std::vector<std::shared_ptr<TrajectoryNode>> TrajectoryNode::subdomains()
{
  std::vector<item_t> ret;
  if (!has_subdomains())
    return ret;

  for (auto &b : branches)
    if (b && !b->is_datapoint(domain))
      ret.push_back(b);

  return ret;
}

bool TrajectoryNode::has_subdomains() const
{
  for (auto &b : branches)
    if (b && (b->domain.type != none))
      return true;
  return false;
}

bool TrajectoryNode::is_datapoint(Domain d) const
{
  if (domain_value.metadata != d.value_range.metadata)
    return false;
  return true;
}


std::shared_ptr<TrajectoryNode> TrajectoryNode::next_setting()
{
  std::shared_ptr<TrajectoryNode> retval;

  if (domain.type == none) {
    if (branches.empty())
      return retval;
    for (auto &b : branches)
      if (b && (retval = b->next_setting()))
        return retval;
  }

  for (auto &s : subdomains())
  {
    item_t last;
    if (!s)
      continue;

    if ((s->domain.type == none) && (retval = s->next_setting()))
      return retval;

    for (auto &b : branches) {
      if (b && b->is_datapoint(domain) && (b->domain == s->domain))
      {
        std::shared_ptr<TrajectoryNode> ret = b->next_setting();
        if (ret)
          return ret;
        last = b;
      }
    }

    if (!last)
    {
      item_t i = deep_copy(self::shared_from_this(), *s);
      i->domain_value = domain.value_range;
      i->domain_value.set_number(i->domain_value.metadata.minimum);
      retval = i;
      return retval;
    }
    else if (last->domain_value.number() >= last->domain_value.metadata.maximum)
      continue;
    else
    {
      item_t i = deep_copy(self::shared_from_this(), *s);
      i->domain_value = last->domain_value;
      i->domain_value++;
      retval = i;
      return retval;
    }
  }

  if (branches.empty())
  {
    item_t i = item_t(new TrajectoryNode(self::shared_from_this()));
    i->domain_value = domain.value_range;
    i->domain_value.set_number(i->domain_value.metadata.minimum);
    retval = i;
  }
  else
  {
    item_t last = branches.back();
    if (last && last->is_datapoint(domain)
        && (last->domain_value.number() < last->domain_value.metadata.maximum))
    {
      item_t i = item_t(new TrajectoryNode(self::shared_from_this()));
      i->domain_value = last->domain_value;
      i->domain_value++;
      retval = i;
    }
  }

  return retval;
}



}
