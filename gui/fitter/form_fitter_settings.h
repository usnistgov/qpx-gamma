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

#ifndef FORM_FITTER_SETTINGS_H
#define FORM_FITTER_SETTINGS_H

#include <QDialog>
#include <QCloseEvent>
#include "fit_settings.h"


namespace Ui {
class FormFitterSettings;
}

class FormFitterSettings : public QDialog
{
  Q_OBJECT

public:
  explicit FormFitterSettings(FitSettings &fs, QWidget *parent = 0);
  ~FormFitterSettings();

protected:
  void closeEvent(QCloseEvent*);


private slots:
  void on_buttonBox_accepted();

  void on_buttonBox_rejected();

  void on_doubleMinRskewSlope_valueChanged(double arg1);
  void on_doubleMaxRskewSlope_valueChanged(double arg1);
  void on_doubleMinRskewAmp_valueChanged(double arg1);
  void on_doubleMaxRskewAmp_valueChanged(double arg1);
  void on_doubleMinLskewSlope_valueChanged(double arg1);
  void on_doubleMaxLskewSlope_valueChanged(double arg1);
  void on_doubleMinLskewAmp_valueChanged(double arg1);
  void on_doubleMaxLskewAmp_valueChanged(double arg1);
  void on_doubleMinTailSlope_valueChanged(double arg1);
  void on_doubleMaxTailSlope_valueChanged(double arg1);
  void on_doubleMinTailAmp_valueChanged(double arg1);
  void on_doubleMaxTailAmp_valueChanged(double arg1);
  void on_doubleMinStep_valueChanged(double arg1);
  void on_doubleMaxStep_valueChanged(double arg1);

  void on_checkOnlySum4_clicked();

  void on_checkGaussOnly_clicked();

private:
  Ui::FormFitterSettings *ui;

  FitSettings &fit_settings_;

  void enforce_bounds();
};

#endif // FORM_FITTER_SETTINGS_H
