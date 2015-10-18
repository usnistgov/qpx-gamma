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
 * Description:
 *      TimeDurationWidget -
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "time_duration_widget.h"
#include "ui_time_duration_widget.h"
#include "custom_logger.h"

TimeDurationWidget::TimeDurationWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::TimeDurationWidget)
{
  ui->setupUi(this);
}

TimeDurationWidget::~TimeDurationWidget()
{
  delete ui;
}

uint64_t TimeDurationWidget::total_seconds() {
  return (60 * (60 * (ui->spinDays->value() * 24 + ui->spinH->value()) + ui->spinM->value()) + ui->spinS->value());
}

void TimeDurationWidget::set_total_seconds(uint64_t secs) {
  ui->spinS->setValue(secs % 60);
  uint64_t total_minutes = secs / 60;
  ui->spinM->setValue(total_minutes % 60);
  uint64_t total_hours = total_minutes / 60;
  ui->spinH->setValue(total_hours % 24);
  ui->spinDays->setValue(total_hours / 24);
}

void TimeDurationWidget::on_spinS_valueChanged(int new_value)
{
  if (new_value >= 60) {
    int mins = static_cast<int>(new_value) / 60;
    int leftover = new_value - (mins * 60);
    ui->spinM->setValue(ui->spinM->value() + mins);
    if (ui->spinDays->value() != ui->spinDays->maximum())
      ui->spinS->setValue(leftover);
    else
      ui->spinS->setValue(0);
  } else if (new_value < 0) {
    if ((ui->spinM->value() > 0) || (ui->spinH->value() > 0)  || (ui->spinDays->value() > 0)) {
      ui->spinM->setValue(ui->spinM->value() - 1);
      ui->spinS->setValue(60.0 + new_value);
    } else {
      ui->spinS->setValue(0);
    }
  }
}

void TimeDurationWidget::on_spinM_valueChanged(int new_value)
{
  if (new_value >= 60) {
    int hours = new_value / 60;
    int leftover = new_value - (hours * 60);
    ui->spinH->setValue(ui->spinH->value() + hours);
    if (ui->spinDays->value() != ui->spinDays->maximum())
      ui->spinM->setValue(leftover);
    else
      ui->spinM->setValue(0);
  } else if (new_value < 0) {
    if ((ui->spinH->value() > 0)  || (ui->spinDays->value() > 0)) {
      ui->spinH->setValue(ui->spinH->value() - 1);
      ui->spinM->setValue(60 + new_value);
    } else
      ui->spinM->setValue(0);
  }
}

void TimeDurationWidget::on_spinH_valueChanged(int new_value)
{
  if (new_value >= 24) {
    int days = new_value / 24;
    int leftover = new_value - (days * 24);
    ui->spinDays->setValue(ui->spinDays->value() + days);
    if (ui->spinDays->value() != ui->spinDays->maximum())
      ui->spinH->setValue(leftover);
    else
      ui->spinH->setValue(0);
  } else if (new_value < 0) {
    if (ui->spinDays->value() > 0) {
      ui->spinDays->setValue(ui->spinDays->value() - 1);
      ui->spinH->setValue(24 + new_value);
    } else
      ui->spinH->setValue(0);
  }

}

void TimeDurationWidget::on_spinDays_editingFinished()
{
  emit editingFinished();
}

void TimeDurationWidget::on_spinH_editingFinished()
{
  emit editingFinished();
}

void TimeDurationWidget::on_spinM_editingFinished()
{
  emit editingFinished();
}

void TimeDurationWidget::on_spinS_editingFinished()
{
  emit editingFinished();
}
