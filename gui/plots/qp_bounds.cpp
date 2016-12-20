#include "qp_bounds.h"
//#include "custom_logger.h"

namespace QPlot
{

void Bounds::clear()
{
  domains_.clear();
  minima_.clear();
  maxima_.clear();
  global_min_ = std::numeric_limits<double>::max();
  global_max_ = std::numeric_limits<double>::min();
}

bool Bounds::empty() const
{
  return domains_.empty();
}

void Bounds::add(double key, double val, double uncert)
{
  if (std::isfinite(uncert) && uncert && add_to_domains(key, val))
  {
    add_minimum(key, val - uncert);
    add_maximum(key, val + uncert);
  }
  else
    add(key, uncert);
}

void Bounds::add(double key, double val)
{
  if (add_to_domains(key, val))
  {
    add_minimum(key, val);
    add_maximum(key, val);
  }
}

void Bounds::add_minimum(double key, double val)
{
  if (!minima_.count(key) || minima_.at(key) > val)
    minima_[key] = val;
  global_min_ = std::min(global_min_, val);
}

void Bounds::add_maximum(double key, double val)
{
  if (!maxima_.count(key) || maxima_.at(key) < val)
    maxima_[key] = val;
  global_max_ = std::max(global_max_, val);
}

bool Bounds::add_to_domains(double key, double val)
{
  if (!std::isfinite(key) || !std::isfinite(val))
    return false;
  add_to_domain(key, QCP::sdBoth);
  if (val <= 0)
    add_to_domain(key, QCP::sdNegative);
  if (val >= 0)
    add_to_domain(key, QCP::sdPositive);
  return true;
}

void Bounds::add_to_domain(double key, QCP::SignDomain sd)
{
  if (!domains_.count(sd))
    domains_[sd] = QCPRange(key, key);
  else
  {
    domains_[sd].lower = std::min(domains_[sd].lower, key);
    domains_[sd].upper = std::max(domains_[sd].upper, key);
  }
}

QCPRange Bounds::getDomain(QCP::SignDomain sd) const
{
  if (domains_.count(sd))
    return domains_.at(sd);
  else if (!domains_.empty())
    return QCPRange(global_min_, global_max_);
  else
    return QCPRange();
}

QCPRange Bounds::getRange(QCPRange domain) const
{
  double min {std::numeric_limits<double>::max()};
  double max {std::numeric_limits<double>::min()};

  for (auto it = minima_.lower_bound(domain.lower); it != minima_.upper_bound(domain.upper); ++it)
    min = std::min(min, it->second);

  for (auto it = maxima_.lower_bound(domain.lower); it != maxima_.upper_bound(domain.upper); ++it)
    max = std::max(max, it->second);

  if ((min != std::numeric_limits<double>::max())
      && (max != std::numeric_limits<double>::min()))
    return QCPRange(min, max);
  else
    return QCPRange();
}

}
