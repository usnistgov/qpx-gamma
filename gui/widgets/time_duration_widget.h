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
 *      TimeDurationWidget -
 *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <boost/date_time.hpp>
#include "qt_util.h"


namespace Ui {
class TimeDurationWidget;
}

class TimeDurationWidget : public QWidget
{
  Q_OBJECT

public:
  explicit TimeDurationWidget(QWidget *parent = 0);
  ~TimeDurationWidget();

  uint64_t total_seconds();
  void set_total_seconds(uint64_t secs);

  void set_duration(boost::posix_time::time_duration duration);
  boost::posix_time::time_duration get_duration() const;

  void set_us_enabled(bool use);

signals:
  void editingFinished();
  void valueChanged();

private slots:
  void on_spinM_valueChanged(int);
  void on_spinH_valueChanged(int);
  void on_spinS_valueChanged(int);
  void on_spin_ms_valueChanged(int arg1);

  void on_spinDays_editingFinished();
  void on_spinH_editingFinished();
  void on_spinM_editingFinished();
  void on_spinS_editingFinished();
  void on_spin_ms_editingFinished();

private:
  Ui::TimeDurationWidget *ui;
  bool us_enabled_;

};
