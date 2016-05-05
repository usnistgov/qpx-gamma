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
#include "project.h"
#include "thread_plot_signal.h"
#include "thread_runner.h"
#include "widget_plot_multi1d.h"
#include "gamma_fitter.h"

namespace Ui {
class FormGainMatch;
}

class FormGainMatch : public QWidget
{
  Q_OBJECT

public:
  explicit FormGainMatch(ThreadRunner&, XMLableDB<Qpx::Detector>&, QWidget *parent = 0);
  void update_settings();
  ~FormGainMatch();

signals:
  void toggleIO(bool);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void toggle_push(bool, Qpx::SourceStatus);

  void update_plots();
  void run_completed();
  void update_fits();
  void update_peak_selection(std::set<double>);

  void do_run();
  void do_post_processing();


  void on_pushMatchGain_clicked();
  void on_pushStop_clicked();
  void on_comboTarget_currentIndexChanged(int index);
  void on_comboSetting_activated(int index);

private:
  void loadSettings();
  void saveSettings();

  Ui::FormGainMatch *ui;
  AppearanceProfile style_fit, style_pts;

  Qpx::Project    project_;

  ThreadRunner         &gm_runner_thread_;
  XMLableDB<Qpx::Detector> &detectors_;

  ThreadPlotSignal     gm_plot_thread_;
  boost::atomic<bool>  gm_interruptor_;

  std::vector<double> gains, deltas;
  std::vector<Qpx::Peak> peaks_;

  bool my_run_;

  int current_pass;

  Qpx::Setting current_setting_;

  Qpx::Fitter fitter_ref_, fitter_opt_;
  Qpx::Peak peak_ref_, peak_opt_;

  Qpx::Metadata make_prototype(uint16_t bits, uint16_t channel,
                               int pass_number, Qpx::Setting variable,
                               std::string name);

};

#endif // FORM_OPTIMIZATION_H
