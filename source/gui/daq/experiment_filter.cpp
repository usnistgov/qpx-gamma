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
 *      FormFitter -
 *
 ******************************************************************************/

#include "experiment_filter.h"
#include <QBoxLayout>
#include "custom_logger.h"

WidgetDomainFilter::WidgetDomainFilter(QString name, std::set<double> choices, QWidget *parent) :
  QWidget(parent)
{
  label_ = new QLabel();
  label_->setFixedHeight(25);
  label_->setText(name);

  combo_ = new QComboBox();
  combo_->setFixedHeight(25);
  for (auto &c : choices)
    combo_->addItem(QString::number(c), c);

  connect(combo_, SIGNAL(currentIndexChanged(int)), this, SLOT(choiceChanged(int)));

  QHBoxLayout *hl = new QHBoxLayout();
  hl->setContentsMargins(0,0,0,0);
  hl->setSpacing(3);
  hl->setSizeConstraint(QLayout::SetMinimumSize);
  hl->addWidget(label_);
  hl->addWidget(combo_);

  setLayout(hl);
}

double WidgetDomainFilter::get_choice()
{
  return combo_->currentData().toDouble();
}

QString WidgetDomainFilter::get_name()
{
  return label_->text();
}

void WidgetDomainFilter::choiceChanged(int)
{
  emit filter_changed();
}



WidgetExperimentFilter::WidgetExperimentFilter(std::map<std::string, std::set<double>> variations,
                                               QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout *vl = new QVBoxLayout();
  vl->setContentsMargins(0,0,0,0);
  vl->setSpacing(3);
  vl->setSizeConstraint(QLayout::SetMinimumSize);

  for (auto &v : variations)
  {
    WidgetDomainFilter *filter = new WidgetDomainFilter(QString::fromStdString(v.first), v.second, this);
    vl->addWidget(filter);
    filters_.push_back(filter);
    connect(filter, SIGNAL(filter_changed()), this, SLOT(filterChanged()));
  }

  setLayout(vl);
}

bool WidgetExperimentFilter::valid(std::map<std::string, Qpx::Setting> domains)
{
  if (filters_.empty())
    return true;
  for (auto &f : filters_)
  {
    if (domains.count(f->get_name().toStdString()) &&
        (domains.at(f->get_name().toStdString()).number() != f->get_choice()))
      return false;
  }
  return true;
}

void WidgetExperimentFilter::filterChanged()
{
  emit filter_changed();
}
