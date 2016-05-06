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
 *      Module and channel settings UI
 *
 ******************************************************************************/

#ifndef FORM_FITTER_AUTOSELECT_H
#define FORM_FITTER_AUTOSELECT_H

#include <QWidget>
#include <QSettings>
#include "gamma_peak.h"


namespace Ui {
class WidgetFitterAutoselect;
}

class WidgetFitterAutoselect : public QWidget
{
  Q_OBJECT

public:

  explicit WidgetFitterAutoselect(QWidget *parent = 0);
  ~WidgetFitterAutoselect();

  std::set<double> autoselect(const std::map<double, Qpx::Peak> &peaks,
                              const std::set<double> &prevsel) const;

  std::set<double> reselect(const std::map<double, Qpx::Peak> &peaks,
                            const std::set<double> &prevsel) const;

  void loadSettings(QSettings &set, QString name = "");
  void saveSettings(QSettings &set, QString name = "");

private:
  Ui::WidgetFitterAutoselect *ui;
};

#endif // FORM_FITTER_AUTOSELECT_H
