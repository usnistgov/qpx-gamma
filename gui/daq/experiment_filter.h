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

#pragma once

#include <QLabel>
#include <QWidget>
#include <QComboBox>
#include <set>
#include "generic_setting.h"

class WidgetDomainFilter : public QWidget {
  Q_OBJECT

public:
  explicit WidgetDomainFilter(QString name, std::set<double> choices, QWidget *parent = 0);
  double get_choice();
  QString get_name();

signals:
  void filter_changed();

private slots:
  void choiceChanged(int);

private:
  QLabel *label_;
  QComboBox* combo_;
};

class WidgetExperimentFilter : public QWidget {
  Q_OBJECT

public:
  explicit WidgetExperimentFilter(std::map<std::string, std::set<double>> variations,
                                  QWidget *parent = 0);

  bool valid(std::map<std::string, Qpx::Setting> domains);

signals:
  void filter_changed();

private slots:
  void filterChanged();

private:
  std::vector<WidgetDomainFilter*> filters_;
};
