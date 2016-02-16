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

#include "form_peak_info.h"
#include "ui_form_peak_info.h"
#include "custom_logger.h"

FormPeakInfo::FormPeakInfo(Hypermet &hm, QWidget *parent) :
  hm_(hm),
  ui(new Ui::FormPeakInfo)
{
  ui->setupUi(this);
  this->setFixedSize(this->size());

//  ui->labelCaliEnergy->setText(QString::fromStdString(hm_.cali_nrg_.to_string()));
//  ui->labelCaliFWHM->setText(QString::fromStdString(hm_.cali_fwhm_.to_string()));

  ui->labelCenter->setText(QString::number(hm_.center_.val) + "\u00B1" + QString::number(hm_.center_.uncert));
  ui->labelHeight->setText(QString::number(hm_.height_.val) + "\u00B1" + QString::number(hm_.height_.uncert));
  ui->labelWidth->setText(QString::number(hm_.width_.val) + "\u00B1" + QString::number(hm_.width_.uncert));


  ui->labelStep->setText("\u00B1" + QString::number(hm_.step_amplitude.uncert));
  ui->checkStepEnable->setChecked(hm_.step_amplitude.enabled);
  ui->doubleMinStep->setValue(hm_.step_amplitude.lbound);
  ui->doubleMaxStep->setValue(hm_.step_amplitude.ubound);
  ui->doubleInitStep->setValue(hm_.step_amplitude.val);

  ui->labelTailH->setText("\u00B1" + QString::number(hm_.tail_amplitude.uncert));
  ui->labelTailS->setText("\u00B1" + QString::number(hm_.tail_slope.uncert));
  ui->checkTailEnable->setChecked(hm_.tail_amplitude.enabled);
  ui->doubleMinTailAmp->setValue(hm_.tail_amplitude.lbound);
  ui->doubleMaxTailAmp->setValue(hm_.tail_amplitude.ubound);
  ui->doubleInitTailAmp->setValue(hm_.tail_amplitude.val);
  ui->doubleMinTailSlope->setValue(hm_.tail_slope.lbound);
  ui->doubleMaxTailSlope->setValue(hm_.tail_slope.ubound);
  ui->doubleInitTailSlope->setValue(hm_.tail_slope.val);

  ui->labelLskewH->setText("\u00B1" + QString::number(hm_.Lskew_amplitude.uncert));
  ui->labelLskewS->setText("\u00B1" + QString::number(hm_.Lskew_slope.uncert));
  ui->checkEnableLskew->setChecked(hm_.Lskew_amplitude.enabled);
  ui->doubleMinLskewAmp->setValue(hm_.Lskew_amplitude.lbound);
  ui->doubleMaxLskewAmp->setValue(hm_.Lskew_amplitude.ubound);
  ui->doubleInitLskewAmp->setValue(hm_.Lskew_amplitude.val);
  ui->doubleMinLskewSlope->setValue(hm_.Lskew_slope.lbound);
  ui->doubleMaxLskewSlope->setValue(hm_.Lskew_slope.ubound);
  ui->doubleInitLskewSlope->setValue(hm_.Lskew_slope.val);

  ui->labelRskewH->setText("\u00B1" + QString::number(hm_.Rskew_amplitude.uncert));
  ui->labelRskewS->setText("\u00B1" + QString::number(hm_.Rskew_slope.uncert));
  ui->checkEnableRskew->setChecked(hm_.Rskew_amplitude.enabled);
  ui->doubleMinRskewAmp->setValue(hm_.Rskew_amplitude.lbound);
  ui->doubleMaxRskewAmp->setValue(hm_.Rskew_amplitude.ubound);
  ui->doubleInitRskewAmp->setValue(hm_.Rskew_amplitude.val);
  ui->doubleMinRskewSlope->setValue(hm_.Rskew_slope.lbound);
  ui->doubleMaxRskewSlope->setValue(hm_.Rskew_slope.ubound);
  ui->doubleInitRskewSlope->setValue(hm_.Rskew_slope.val);

//  ui->checkWidthCommon->setChecked(hm_.width_common);
//  ui->doubleMinWidthCommon->setValue(hm_.width_common_bounds.lbound);
//  ui->doubleMaxWidthCommon->setValue(hm_.width_common_bounds.ubound);
//  ui->doubleMinWidthVariable->setValue(hm_.width_variable_bounds.lbound);
//  ui->doubleMaxWidthVariable->setValue(hm_.width_variable_bounds.ubound);

//  ui->doubleLateralSlack->setValue(hm_.lateral_slack);
//  ui->spinFitterMaxIterations->setValue(hm_.fitter_max_iter);
}

void FormPeakInfo::on_buttonBox_accepted()
{
  hm_.step_amplitude.enabled = ui->checkStepEnable->isChecked();
  hm_.step_amplitude.lbound = ui->doubleMinStep->value();
  hm_.step_amplitude.ubound = ui->doubleMaxStep->value();
  hm_.step_amplitude.val = ui->doubleInitStep->value();

  hm_.tail_amplitude.enabled = ui->checkTailEnable->isChecked();
  hm_.tail_amplitude.lbound = ui->doubleMinTailAmp->value();
  hm_.tail_amplitude.ubound = ui->doubleMaxTailAmp->value();
  hm_.tail_amplitude.val = ui->doubleInitTailAmp->value();
  hm_.tail_slope.lbound = ui->doubleMinTailSlope->value();
  hm_.tail_slope.ubound = ui->doubleMaxTailSlope->value();
  hm_.tail_slope.val = ui->doubleInitTailSlope->value();

  hm_.Lskew_amplitude.enabled = ui->checkEnableLskew->isChecked();
  hm_.Lskew_amplitude.lbound = ui->doubleMinLskewAmp->value();
  hm_.Lskew_amplitude.ubound = ui->doubleMaxLskewAmp->value();
  hm_.Lskew_amplitude.val = ui->doubleInitLskewAmp->value();
  hm_.Lskew_slope.lbound = ui->doubleMinLskewSlope->value();
  hm_.Lskew_slope.ubound = ui->doubleMaxLskewSlope->value();
  hm_.Lskew_slope.val = ui->doubleInitLskewSlope->value();

  hm_.Rskew_amplitude.enabled = ui->checkEnableRskew->isChecked();
  hm_.Rskew_amplitude.lbound = ui->doubleMinRskewAmp->value();
  hm_.Rskew_amplitude.ubound = ui->doubleMaxRskewAmp->value();
  hm_.Rskew_amplitude.val = ui->doubleInitRskewAmp->value();
  hm_.Rskew_slope.lbound = ui->doubleMinRskewSlope->value();
  hm_.Rskew_slope.ubound = ui->doubleMaxRskewSlope->value();
  hm_.Rskew_slope.val = ui->doubleInitRskewSlope->value();

//  hm_.width_common = ui->checkWidthCommon->isChecked();
//  hm_.width_common_bounds.lbound = ui->doubleMinWidthCommon->value();
//  hm_.width_common_bounds.ubound = ui->doubleMaxWidthCommon->value();
//  hm_.width_variable_bounds.lbound = ui->doubleMinWidthVariable->value();
//  hm_.width_variable_bounds.ubound = ui->doubleMaxWidthVariable->value();

//  hm_.lateral_slack = ui->doubleLateralSlack->value();
//  hm_.fitter_max_iter = ui->spinFitterMaxIterations->value();

  accept();
}

void FormPeakInfo::enforce_bounds()
{

  ui->doubleInitStep->setMinimum(ui->doubleMinStep->value());
  ui->doubleMaxStep->setMinimum(ui->doubleMinStep->value());
  ui->doubleInitStep->setMaximum(ui->doubleMaxStep->value());
  ui->doubleMinStep->setMaximum(ui->doubleMaxStep->value());

  ui->doubleInitTailAmp->setMinimum(ui->doubleMinTailAmp->value());
  ui->doubleMaxTailAmp->setMinimum(ui->doubleMinTailAmp->value());
  ui->doubleInitTailAmp->setMaximum(ui->doubleMaxTailAmp->value());
  ui->doubleMinTailAmp->setMaximum(ui->doubleMaxTailAmp->value());

  ui->doubleInitTailSlope->setMinimum(ui->doubleMinTailSlope->value());
  ui->doubleMaxTailSlope->setMinimum(ui->doubleMinTailSlope->value());
  ui->doubleInitTailSlope->setMaximum(ui->doubleMaxTailSlope->value());
  ui->doubleMinTailSlope->setMaximum(ui->doubleMaxTailSlope->value());

  ui->doubleInitLskewAmp->setMinimum(ui->doubleMinLskewAmp->value());
  ui->doubleMaxLskewAmp->setMinimum(ui->doubleMinLskewAmp->value());
  ui->doubleInitLskewAmp->setMaximum(ui->doubleMaxLskewAmp->value());
  ui->doubleMinLskewAmp->setMaximum(ui->doubleMaxLskewAmp->value());

  ui->doubleInitLskewSlope->setMinimum(ui->doubleMinLskewSlope->value());
  ui->doubleMaxLskewSlope->setMinimum(ui->doubleMinLskewSlope->value());
  ui->doubleInitLskewSlope->setMaximum(ui->doubleMaxLskewSlope->value());
  ui->doubleMinLskewSlope->setMaximum(ui->doubleMaxLskewSlope->value());

  ui->doubleInitRskewAmp->setMinimum(ui->doubleMinRskewAmp->value());
  ui->doubleMaxRskewAmp->setMinimum(ui->doubleMinRskewAmp->value());
  ui->doubleInitRskewAmp->setMaximum(ui->doubleMaxRskewAmp->value());
  ui->doubleMinRskewAmp->setMaximum(ui->doubleMaxRskewAmp->value());

  ui->doubleInitRskewSlope->setMinimum(ui->doubleMinRskewSlope->value());
  ui->doubleMaxRskewSlope->setMinimum(ui->doubleMinRskewSlope->value());
  ui->doubleInitRskewSlope->setMaximum(ui->doubleMaxRskewSlope->value());
  ui->doubleMinRskewSlope->setMaximum(ui->doubleMaxRskewSlope->value());


}

FormPeakInfo::~FormPeakInfo()
{
  delete ui;
}

void FormPeakInfo::closeEvent(QCloseEvent *event) {
  event->accept();
}


void FormPeakInfo::on_buttonBox_rejected()
{
  reject();
}

void FormPeakInfo::on_doubleMinRskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxRskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinRskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxRskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinLskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxLskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinLskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxLskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinTailSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxTailSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinTailAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxTailAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMinStep_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormPeakInfo::on_doubleMaxStep_valueChanged(double arg1)
{
  enforce_bounds();
}
