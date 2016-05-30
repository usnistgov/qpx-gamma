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
 *      FormExperiment -
 *
 ******************************************************************************/


#ifndef FORM_EXPERIMENT_H
#define FORM_EXPERIMENT_H

#include <QWidget>
#include "project.h"
#include "thread_runner.h"
#include "thread_plot_signal.h"
#include "form_experiment_setup.h"
#include "form_experiment_1d.h"
#include "form_experiment_2d.h"
#include "experiment.h"
#include "widget_selector.h"

namespace Ui {
class FormExperiment;
}

class FormExperiment : public QWidget
{
  Q_OBJECT

public:
  explicit FormExperiment(ThreadRunner&, QWidget *parent = 0);
  ~FormExperiment();

signals:
  void toggleIO(bool);
  void settings_changed();

public slots:
  void toggle_push(bool, Qpx::SourceStatus);

private slots:
  void selectProject(int64_t idx);

  void new_daq_data();
  void run_completed();

  void fitter_status(bool);
  void update_fits();
  void fitting_done();
  void update_peak_selection(std::set<double>);

  void on_pushNewExperiment_clicked();
  void on_pushLoadExperiment_clicked();
  void on_pushSaveExperiment_clicked();

  void on_pushStart_clicked();
  void on_pushStop_clicked();

  void populate_selector();
  void spectrumDoubleclicked(SelectorItem item);
  void choose_spectrum(SelectorItem item);
  void toggle_from_setup();

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormExperiment  *ui;
  FormExperimentSetup *form_experiment_setup_;
  FormExperiment1D    *form_experiment_1d_;
  FormExperiment2D    *form_experiment_2d_;

  ThreadRunner         &runner_thread_;
  ThreadPlotSignal     exp_plot_thread_;
  boost::atomic<bool>  interruptor_;
  bool my_run_;
  bool continue_;

  //to be shared by reference with children
  Qpx::ExperimentProject exp_project_;
  QString data_directory_;

  int64_t selected_sink_;
  Qpx::Fitter selected_fitter_;


  void start_new_pass();
  std::pair<Qpx::ProjectPtr, uint64_t> get_next_point();

  void loadSettings();
  void saveSettings();
  void update_name();
};

#endif // FORM_CALIBRATION_H
