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

#ifndef QPX_DOMAIN
#define QPX_DOMAIN

#include "xmlable.h"
#include "generic_setting.h"
#include <set>
#include <string>

namespace Qpx {

enum DomainType
{
  source, sink, manual, none
};

enum DomainAdvanceCriterion
{
  realtime, livetime, totalcount, indefinite
};

struct Domain
{
  Domain() : type(none), criterion_type(indefinite), criterion(0) {}

  std::string verbose;

  DomainType  type;
  Setting     value_range;
  DomainAdvanceCriterion criterion_type;
  uint64_t    criterion;

  bool operator== (const Domain&) const;
  bool operator!= (const Domain&) const;
};

class TrajectoryNode;

class TrajectoryNode : public XMLable, public std::enable_shared_from_this<TrajectoryNode>
{
public:

  Domain domain; //if ==none, then it is a data point

  Setting domain_value;
  int64_t data_idx;

  std::string type() const;
  std::string to_string() const;
  std::string range_to_string() const;
  std::string crit_to_string() const;

private:
  using item_t = std::shared_ptr<TrajectoryNode>;
  using self = TrajectoryNode;
  using const_item_t = std::shared_ptr< const TrajectoryNode >;
  using weak_item_t = std::weak_ptr< TrajectoryNode >;

  std::vector< item_t > branches;
  weak_item_t parent;

public:
  TrajectoryNode(weak_item_t p = weak_item_t());
  TrajectoryNode(weak_item_t p, const TrajectoryNode &other);
  static std::shared_ptr<TrajectoryNode> deep_copy(weak_item_t p, const TrajectoryNode &other);

  int row() const;
  int childCount() const {return branches.size();}
  std::shared_ptr<TrajectoryNode> getParent() const {return parent.lock();}
  std::shared_ptr<TrajectoryNode> getChild(int row) const;
  int childPos(const const_item_t&) const;
//  size_t type_id() const;
//  int id() const;
//  std::string name() const;
  void emplace_back(TrajectoryNode &&t);
  void push_back(const TrajectoryNode &t);
  void remove_child(int row);
  void replace_child(int row, const TrajectoryNode &t);

  void populate();
  std::shared_ptr<TrajectoryNode> next_setting();

  std::string xml_element_name() const override {return "TrajectoryNode";}
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;

  bool operator== (const TrajectoryNode&) const;
  bool shallow_equals(const TrajectoryNode&) const;
  bool valid() const;

  bool has_subdomains() const;
  bool is_datapoint(Domain) const;

  double estimated_time(); //const

private:
  static std::string domain_type_to_string(DomainType);
  static DomainType domain_type_from_string(std::string);

  static std::string criterion_type_to_string(DomainAdvanceCriterion);
  static DomainAdvanceCriterion criterion_type_from_string(std::string);

  std::vector<std::shared_ptr<TrajectoryNode>> subdomains();
};

typedef std::shared_ptr<TrajectoryNode> TrajectoryPtr;


}

#endif
