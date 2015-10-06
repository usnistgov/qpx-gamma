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
 *     Gain matching UI
 *
 ******************************************************************************/

#ifndef FORM_GAIN_MATCH_H
#define FORM_GAIN_MATCH_H

#include <QWidget>
#include <QSettings>
#include "spectra_set.h"
#include "thread_plot_signal.h"
#include "thread_runner.h"
#include "widget_plot1d.h"
#include "gamma_fitter.h"

namespace Ui {
class FormGainMatch;
}

class FormGainMatch : public QWidget
{
  Q_OBJECT

public:
  explicit FormGainMatch(ThreadRunner&, QSettings&, XMLableDB<Gamma::Detector>&, QWidget *parent = 0);
  ~FormGainMatch();

signals:
  void toggleIO(bool);
  void restart_run();
  void post_proc();
  void optimization_approved();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void update_plots();
  void run_completed();

  void on_pushMatchGain_clicked();
  void addMovingMarker(double);
  void removeMovingMarker(double);
  void replot_markers();

  bool find_peaks();

  void on_pushStop_clicked();
  void do_post_processing();
  void do_run();

  void toggle_push(bool, Qpx::LiveStatus);

  void on_pushSaveOpti_clicked();

private:
  void loadSettings();
  void saveSettings();

  Ui::FormGainMatch *ui;

  Qpx::SpectraSet    gm_spectra_;
  Qpx::Spectrum::Template reference_, optimizing_;

  ThreadRunner         &gm_runner_thread_;
  XMLableDB<Gamma::Detector> &detectors_;
  QSettings &settings_;

  ThreadPlotSignal     gm_plot_thread_;
  boost::atomic<bool>  gm_interruptor_;

  std::vector<double> x, y_ref, y_opt;

  bool my_run_;

  Marker moving_, marker_ref_, marker_opt_, a ,b;
  AppearanceProfile ap_reference_, ap_optimized_;

  int bits, current_pass, max_passes;

  Gamma::Peak gauss_ref_, gauss_opt_;

  CustomTimer* minTimer;

};

#endif // FORM_OPTIMIZATION_H
