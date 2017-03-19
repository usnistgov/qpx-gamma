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

#include "form_fitter_settings.h"
#include "ui_form_fitter_settings.h"
#include "custom_logger.h"

FormFitterSettings::FormFitterSettings(FitSettings &fs, QWidget *parent) :
  fit_settings_(fs),
  ui(new Ui::FormFitterSettings)
{
  ui->setupUi(this);
  this->setFixedSize(this->size());

  ui->labelCaliEnergy->setText(QString::fromStdString(fit_settings_.cali_nrg_.debug()));
  ui->labelCaliFWHM->setText(QString::fromStdString(fit_settings_.cali_fwhm_.debug()));

  ui->spinFinderCutoffKeV->setValue(fit_settings_.finder_cutoff_kev);

  ui->spinKONwidth->setValue(fit_settings_.KON_width);
  ui->doubleKONsigmaSpectrum->setValue(fit_settings_.KON_sigma_spectrum);
  ui->doubleKONsigmaResid->setValue(fit_settings_.KON_sigma_resid);

  ui->spinRegionMaxPeaks->setValue(fit_settings_.ROI_max_peaks);
  ui->doubleRegionExtendPeaks->setValue(fit_settings_.ROI_extend_peaks);
  ui->doubleRegionExtendBackground->setValue(fit_settings_.ROI_extend_background);

  ui->spinEdgeSamples->setValue(fit_settings_.background_edge_samples);
  ui->checkOnlySum4->setChecked(fit_settings_.sum4_only);

  ui->checkResidAuto->setChecked(fit_settings_.resid_auto);
  ui->spinResidMaxIterations->setValue(fit_settings_.resid_max_iterations);
  ui->spinResidMinAmplitude->setValue(fit_settings_.resid_min_amplitude);
  ui->doubleResidTooClose->setValue(fit_settings_.resid_too_close);

  ui->checkSmallSimplify->setChecked(fit_settings_.small_simplify);
  ui->spinSmallMaxAmplitude->setValue(fit_settings_.small_max_amplitude);

  ui->checkGaussOnly->setChecked(fit_settings_.gaussian_only);

  ui->checkStepEnable->setChecked(fit_settings_.step_amplitude.enabled());
  ui->doubleMinStep->setValue(fit_settings_.step_amplitude.lower());
  ui->doubleMaxStep->setValue(fit_settings_.step_amplitude.upper());
  ui->doubleInitStep->setValue(fit_settings_.step_amplitude.value().value());

  ui->checkTailEnable->setChecked(fit_settings_.tail_amplitude.enabled());
  ui->doubleMinTailAmp->setValue(fit_settings_.tail_amplitude.lower());
  ui->doubleMaxTailAmp->setValue(fit_settings_.tail_amplitude.upper());
  ui->doubleInitTailAmp->setValue(fit_settings_.tail_amplitude.value().value());
  ui->doubleMinTailSlope->setValue(fit_settings_.tail_slope.lower());
  ui->doubleMaxTailSlope->setValue(fit_settings_.tail_slope.upper());
  ui->doubleInitTailSlope->setValue(fit_settings_.tail_slope.value().value());

  ui->checkEnableLskew->setChecked(fit_settings_.Lskew_amplitude.enabled());
  ui->doubleMinLskewAmp->setValue(fit_settings_.Lskew_amplitude.lower());
  ui->doubleMaxLskewAmp->setValue(fit_settings_.Lskew_amplitude.upper());
  ui->doubleInitLskewAmp->setValue(fit_settings_.Lskew_amplitude.value().value());
  ui->doubleMinLskewSlope->setValue(fit_settings_.Lskew_slope.lower());
  ui->doubleMaxLskewSlope->setValue(fit_settings_.Lskew_slope.upper());
  ui->doubleInitLskewSlope->setValue(fit_settings_.Lskew_slope.value().value());

  ui->checkEnableRskew->setChecked(fit_settings_.Rskew_amplitude.enabled());
  ui->doubleMinRskewAmp->setValue(fit_settings_.Rskew_amplitude.lower());
  ui->doubleMaxRskewAmp->setValue(fit_settings_.Rskew_amplitude.upper());
  ui->doubleInitRskewAmp->setValue(fit_settings_.Rskew_amplitude.value().value());
  ui->doubleMinRskewSlope->setValue(fit_settings_.Rskew_slope.lower());
  ui->doubleMaxRskewSlope->setValue(fit_settings_.Rskew_slope.upper());
  ui->doubleInitRskewSlope->setValue(fit_settings_.Rskew_slope.value().value());

  ui->checkWidthCommon->setChecked(fit_settings_.width_common);
  ui->doubleMinWidthCommon->setValue(fit_settings_.width_common_bounds.lower());
  ui->doubleMaxWidthCommon->setValue(fit_settings_.width_common_bounds.upper());
  ui->doubleMinWidthVariable->setValue(fit_settings_.width_variable_bounds.lower());
  ui->doubleMaxWidthVariable->setValue(fit_settings_.width_variable_bounds.upper());

  ui->checkWidthAt511->setChecked(fit_settings_.width_at_511_variable);
  ui->spinWidthAt511Tolerance->setValue(fit_settings_.width_at_511_tolerance);

  ui->doubleLateralSlack->setValue(fit_settings_.lateral_slack);
  ui->spinFitterMaxIterations->setValue(fit_settings_.fitter_max_iter);

  on_checkOnlySum4_clicked();
  on_checkGaussOnly_clicked();
}

void FormFitterSettings::on_buttonBox_accepted()
{
  fit_settings_.finder_cutoff_kev = ui->spinFinderCutoffKeV->value();

  fit_settings_.KON_width = ui->spinKONwidth->value();
  fit_settings_.KON_sigma_spectrum = ui->doubleKONsigmaSpectrum->value();
  fit_settings_.KON_sigma_resid = ui->doubleKONsigmaResid->value();

  fit_settings_.ROI_max_peaks = ui->spinRegionMaxPeaks->value();
  fit_settings_.ROI_extend_peaks = ui->doubleRegionExtendPeaks->value();
  fit_settings_.ROI_extend_background = ui->doubleRegionExtendBackground->value();

  fit_settings_.background_edge_samples = ui->spinEdgeSamples->value();
  fit_settings_.sum4_only = ui->checkOnlySum4->isChecked();

  fit_settings_.resid_auto = ui->checkResidAuto->isChecked();
  fit_settings_.resid_max_iterations = ui->spinResidMaxIterations->value();
  fit_settings_.resid_min_amplitude = ui->spinResidMinAmplitude->value();
  fit_settings_.resid_too_close = ui->doubleResidTooClose->value();

  fit_settings_.small_simplify = ui->checkSmallSimplify->isChecked();
  fit_settings_.small_max_amplitude = ui->spinSmallMaxAmplitude->value();

  fit_settings_.gaussian_only = ui->checkGaussOnly->isChecked();

  fit_settings_.step_amplitude.set_enabled(ui->checkStepEnable->isChecked());
  fit_settings_.step_amplitude.set(ui->doubleMinStep->value(),
                                   ui->doubleMaxStep->value(),
                                   ui->doubleInitStep->value());

  fit_settings_.tail_amplitude.set_enabled(ui->checkTailEnable->isChecked());
  fit_settings_.tail_amplitude.set(ui->doubleMinTailAmp->value(),
                                   ui->doubleMaxTailAmp->value(),
                                   ui->doubleInitTailAmp->value());
  fit_settings_.tail_slope.set(ui->doubleMinTailSlope->value(),
                               ui->doubleMaxTailSlope->value(),
                               ui->doubleInitTailSlope->value());

  fit_settings_.Lskew_amplitude.set_enabled(ui->checkEnableLskew->isChecked());
  fit_settings_.Lskew_amplitude.set(ui->doubleMinLskewAmp->value(),
                                    ui->doubleMaxLskewAmp->value(),
                                    ui->doubleInitLskewAmp->value());
  fit_settings_.Lskew_slope.set(ui->doubleMinLskewSlope->value(),
                                ui->doubleMaxLskewSlope->value(),
                                ui->doubleInitLskewSlope->value());

  fit_settings_.Rskew_amplitude.set_enabled(ui->checkEnableRskew->isChecked());
  fit_settings_.Rskew_amplitude.set(ui->doubleMinRskewAmp->value(),
                                    ui->doubleMaxRskewAmp->value(),
                                    ui->doubleInitRskewAmp->value());
  fit_settings_.Rskew_slope.set(ui->doubleMinRskewSlope->value(),
                                ui->doubleMaxRskewSlope->value(),
                                ui->doubleInitRskewSlope->value());

  fit_settings_.width_common = ui->checkWidthCommon->isChecked();
  fit_settings_.width_common_bounds.preset_bounds(ui->doubleMinWidthCommon->value(),
                                                  ui->doubleMaxWidthCommon->value());
  fit_settings_.width_variable_bounds.preset_bounds(ui->doubleMinWidthVariable->value(),
                                                    ui->doubleMaxWidthVariable->value());

  fit_settings_.width_at_511_variable = ui->checkWidthAt511->isChecked();
  fit_settings_.width_at_511_tolerance = ui->spinWidthAt511Tolerance->value();

  fit_settings_.lateral_slack = ui->doubleLateralSlack->value();
  fit_settings_.fitter_max_iter = ui->spinFitterMaxIterations->value();

  accept();
}

void FormFitterSettings::enforce_bounds()
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

FormFitterSettings::~FormFitterSettings()
{
  delete ui;
}

void FormFitterSettings::closeEvent(QCloseEvent *event) {
  event->accept();
}


void FormFitterSettings::on_buttonBox_rejected()
{
  reject();
}

void FormFitterSettings::on_doubleMinRskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxRskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinRskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxRskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinLskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxLskewSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinLskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxLskewAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinTailSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxTailSlope_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinTailAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxTailAmp_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMinStep_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_doubleMaxStep_valueChanged(double arg1)
{
  enforce_bounds();
}

void FormFitterSettings::on_checkOnlySum4_clicked()
{
  bool enabled = !ui->checkOnlySum4->isChecked();

  ui->boxHypermet->setEnabled(enabled);
  ui->boxDeconvolution->setEnabled(enabled);
  ui->spinResidMaxIterations->setEnabled(enabled);
  ui->spinResidMinAmplitude->setEnabled(enabled);
  ui->doubleResidTooClose->setEnabled(enabled);
  ui->spinFitterMaxIterations->setEnabled(enabled);
  ui->checkResidAuto->setEnabled(enabled);
}

void FormFitterSettings::on_checkGaussOnly_clicked()
{
  bool enabled = !ui->checkGaussOnly->isChecked();

  ui->checkStepEnable->setEnabled(enabled);
  ui->doubleMinStep->setEnabled(enabled);
  ui->doubleMaxStep->setEnabled(enabled);
  ui->doubleInitStep->setEnabled(enabled);

  ui->checkTailEnable->setEnabled(enabled);
  ui->doubleMinTailAmp->setEnabled(enabled);
  ui->doubleMaxTailAmp->setEnabled(enabled);
  ui->doubleInitTailAmp->setEnabled(enabled);
  ui->doubleMinTailSlope->setEnabled(enabled);
  ui->doubleMaxTailSlope->setEnabled(enabled);
  ui->doubleInitTailSlope->setEnabled(enabled);

  ui->checkEnableLskew->setEnabled(enabled);
  ui->doubleMinLskewAmp->setEnabled(enabled);
  ui->doubleMaxLskewAmp->setEnabled(enabled);
  ui->doubleInitLskewAmp->setEnabled(enabled);
  ui->doubleMinLskewSlope->setEnabled(enabled);
  ui->doubleMaxLskewSlope->setEnabled(enabled);
  ui->doubleInitLskewSlope->setEnabled(enabled);

  ui->checkEnableRskew->setEnabled(enabled);
  ui->doubleMinRskewAmp->setEnabled(enabled);
  ui->doubleMaxRskewAmp->setEnabled(enabled);
  ui->doubleInitRskewAmp->setEnabled(enabled);
  ui->doubleMinRskewSlope->setEnabled(enabled);
  ui->doubleMaxRskewSlope->setEnabled(enabled);
  ui->doubleInitRskewSlope->setEnabled(enabled);
}
