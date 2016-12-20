#ifndef QP_BOUNDS_H
#define QP_BOUNDS_H

#include "qcustomplot.h"

namespace QPlot
{

class Bounds
{
public:
  void clear();
  bool empty() const;
  void add(double key, double val);
  void add(double key, double val, double uncert);
  QCPRange getDomain(QCP::SignDomain sd = QCP::sdBoth) const;
  QCPRange getRange(QCPRange domain = QCPRange()) const;

private:
  std::map<double, double> minima_, maxima_;
  double global_min_ {std::numeric_limits<double>::max()};
  double global_max_ {std::numeric_limits<double>::min()};

  std::map<QCP::SignDomain, QCPRange> domains_;

  void add_minimum(double key, double val);
  void add_maximum(double key, double val);
  bool add_to_domains(double key, double val);
  void add_to_domain(double key, QCP::SignDomain sd);
};


}

#endif
