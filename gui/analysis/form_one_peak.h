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
 *      FormOnePeak -
 *
 ******************************************************************************/


#ifndef FORM_ONE_PEAK_H
#define FORM_ONE_PEAK_H

#include <QWidget>
#include "widget_plot_fit.h"
#include "gamma_fitter.h"

namespace Ui {
class FormOnePeak;
}

class FormOnePeak : public QWidget
{
  Q_OBJECT

public:
  explicit FormOnePeak(QWidget *parent = 0);
  ~FormOnePeak();

  void setFit(Gamma::Fitter *);

  void update_spectrum();

  void update_fit(bool content_changed = false);
  void make_range(Marker);

  void loadSettings(QSettings &settings_, QString name);
  void saveSettings(QSettings &settings_, QString name);

signals:
  void peaks_changed(bool content_changed);
  void range_changed(Range range);
  void peak_available();

private slots:
  void change_range(Range range);

  void peaks_changed_in_plot(bool);
  void remove_peak();

private:
  Ui::FormOnePeak *ui;

  //data from selected spectrum
  Gamma::Fitter *fit_data_;

  void peak_info();
};

#endif
