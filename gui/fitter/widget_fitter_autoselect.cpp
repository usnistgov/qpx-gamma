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
 *      Module and channel settings
 *
 ******************************************************************************/

#include "widget_fitter_autoselect.h"
#include "ui_widget_fitter_autoselect.h"
#include "custom_logger.h"

WidgetFitterAutoselect::WidgetFitterAutoselect(QWidget *parent)
  : QWidget(parent)
  , ui(new Ui::WidgetFitterAutoselect)
{
  ui->setupUi(this);

  ui->comboAutoMethod->addItem("None");
  ui->comboAutoMethod->addItem("Max(height)");
  ui->comboAutoMethod->addItem("Max(area)");
  ui->comboAutoMethod->addItem("Nearest");
}

void WidgetFitterAutoselect::loadSettings(QSettings &set, QString name)
{
  if (name.isEmpty())
    name = "WidgetFitterAutoselect";
  set.beginGroup(name);
  ui->comboAutoMethod->setCurrentText(set.value("autosel_method", "None").toString());
  ui->checkReselect->setChecked(set.value("reselect", false).toBool());
  ui->doubleSlack->setValue(set.value("reselect_slack", 0.5).toDouble());
  set.endGroup();
}

void WidgetFitterAutoselect::saveSettings(QSettings &set, QString name)
{
  if (name.isEmpty())
    name = "WidgetFitterAutoselect";
  set.beginGroup(name);
  set.setValue("autosel_method", ui->comboAutoMethod->currentText());
  set.setValue("reselect", ui->checkReselect->isChecked());
  set.setValue("reselect_slack", ui->doubleSlack->value());
  set.endGroup();
}

WidgetFitterAutoselect::~WidgetFitterAutoselect()
{
  delete ui;
}

std::set<double> WidgetFitterAutoselect::autoselect(const std::map<double, Qpx::Peak> &peaks,
                                              const std::set<double> &prevsel) const
{
  double selection = peaks.begin()->first;

  if (ui->comboAutoMethod->currentText() == "Max(height)")
  {
    double maxheight = peaks.begin()->first;
    for (auto &p : peaks) {
      if (p.second.hypermet_.height_.val > maxheight) {
        maxheight = p.second.hypermet_.height_.val;
        selection = p.first;
      }
    }
  }
  else if (ui->comboAutoMethod->currentText() == "Max(area)")
  {
    double maxarea = peaks.begin()->first;
    for (auto &p : peaks) {
      if (p.second.area_best.val > maxarea) {
        maxarea = p.second.area_best.val;
        selection = p.first;
      }
    }
  }
  else if (ui->comboAutoMethod->currentText() == "Nearest")
  {
    if (prevsel.empty())
      return std::set<double>();

    //make this work for multiple peaks
    double prevpeak = *prevsel.begin();
    double delta = std::numeric_limits<double>::max();
    for (auto &p : peaks) {
      if (abs(p.second.center.val - prevpeak) < delta) {
        delta = abs(p.second.center.val - prevpeak);
        selection = p.first;
      }
    }
  }

  std::set<double> newselr;
  if (ui->comboAutoMethod->currentText() != "None")
    newselr.insert(selection);
  return newselr;
}

std::set<double> WidgetFitterAutoselect::reselect(const std::map<double, Qpx::Peak> &peaks,
                                              const std::set<double> &prevsel) const
{
  if (!ui->checkReselect->isChecked() || prevsel.empty() || peaks.empty())
    return prevsel;

  std::set<double> newselr;

  for (auto &prev : prevsel)
  {
    double selection = peaks.begin()->first;
    double delta = std::numeric_limits<double>::max();
    for (auto &p : peaks) {
      if (abs(p.second.center.val - prev) < delta) {
        delta = abs(p.second.center.val - prev);
        selection = p.first;
      }
    }
    if (abs(selection - prev) < ui->doubleSlack->value() * peaks.at(selection).fwhm_hyp)
      newselr.insert(selection);
  }

  if (newselr.size() == prevsel.size())
    return newselr;
  else
    return prevsel;
}

