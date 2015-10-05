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
 *     Optimization UI
 *
 ******************************************************************************/

#ifndef FORM_OPTIMIZATION_H
#define FORM_OPTIMIZATION_H

#include <QWidget>
#include <QSettings>
#include "spectra_set.h"
#include "spectrum1D.h"
#include "thread_plot_signal.h"
#include "thread_runner.h"
#include "widget_plot1d.h"
#include "gamma_fitter.h"

namespace Ui {
class FormOptimization;
}

class FormOptimization : public QWidget
{
  Q_OBJECT

public:
  explicit FormOptimization(ThreadRunner&, QSettings&, XMLable2DB<Gamma::Detector>&, QWidget *parent = 0);
  ~FormOptimization();

signals:
  void toggleIO(bool);
  void restart_run();
  void post_proc();
  void optimization_approved();
  void settings_changed();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void update_plots();
  void run_completed();

  void on_pushStart_clicked();
  void addMovingMarker(double);
  void removeMovingMarker(double);
  void replot_markers();

  bool find_peaks();
  void resultChosen();

  void on_pushStop_clicked();
  void do_post_processing();
  void do_run();

  void toggle_push(bool, Qpx::LiveStatus);

  void on_pushSaveOpti_clicked();

private:
  void loadSettings();
  void saveSettings();

  void plot_derivs(Gamma::Fitter &data);


  Ui::FormOptimization *ui;

  Qpx::Wrapper&      pixie_;
  Qpx::SpectraSet current_spectra_;
  Qpx::Spectrum::Template        optimizing_;

  ThreadRunner         &opt_runner_thread_;
  XMLable2DB<Gamma::Detector> &detectors_;
  QSettings &settings_;

  ThreadPlotSignal     opt_plot_thread_;
  boost::atomic<bool>  interruptor_;

  std::vector<double> x, y_opt;
  std::vector<std::vector<double>> spectra_y_;
  std::vector<uint32_t> spectra_app_;

  double val_min, val_max, val_d, val_current;

  bool my_run_;

  Marker moving_, a, b;
  int bits;

  AppearanceProfile prelim_peak_, filtered_peak_,
                    gaussian_, baseline_,
                    rise_, fall_, even_;

  std::string current_setting_;
  std::vector<Gamma::Peak> peaks_;
  std::vector<double> setting_values_;
  std::vector<double> setting_fwhm_;
};

#endif // FORM_OPTIMIZATION_H
