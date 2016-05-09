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
#include "project.h"
#include "thread_plot_signal.h"
#include "thread_runner.h"
#include "gamma_fitter.h"
#include "form_fitter.h"

namespace Ui {
class FormOptimization;
}

class FormOptimization : public QWidget
{
  Q_OBJECT

private:
  struct DataPoint
  {
    Qpx::Fitter spectrum;
    Qpx::Peak selected_peak;
    double independent_variable;
    double dependent_variable;
  };

public:
  explicit FormOptimization(ThreadRunner&, XMLableDB<Qpx::Detector>&, QWidget *parent = 0);
  void update_settings();
  ~FormOptimization();

signals:
  void toggleIO(bool);
  void settings_changed();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void toggle_push(bool, Qpx::SourceStatus);
  void new_daq_data();
  void run_completed();

  void fitter_status(bool);
  void update_fits();
  void fitting_done();
  void update_peak_selection(std::set<double>);

  void pass_selected_in_table();
  void pass_selected_in_plot();


  void on_pushStart_clicked();
  void on_pushStop_clicked();

  void on_comboTarget_currentIndexChanged(int index);
  void on_comboSetting_activated(const QString &arg1);
  void on_comboUntil_activated(const QString &arg1);
  void on_comboSinkType_activated(const QString &arg1);
  void on_pushSaveCsv_clicked();

  void on_pushEditPrototype_clicked();

  void on_pushAddCustom_clicked();
  void on_pushEditCustom_clicked();
  void on_pushDeleteCustom_clicked();

private:
  void loadSettings();
  void saveSettings();

  XMLableDB<Qpx::Detector> &detectors_;
  Ui::FormOptimization *ui;
  AppearanceProfile style_pts;

  ThreadRunner         &opt_runner_thread_;
  ThreadPlotSignal     opt_plot_thread_;
  Qpx::Project         project_;
  boost::atomic<bool>  interruptor_;
  bool my_run_;

  std::vector<Qpx::Detector> current_dets_;
  Qpx::Metadata sink_prototype_;


  std::map<std::string, Qpx::Setting> manual_settings_;
  std::map<std::string, Qpx::Setting> source_settings_;
  std::map<std::string, Qpx::Setting> sink_settings_;
  std::map<std::string, Qpx::Setting> all_settings_;


  int current_pass_;
  Qpx::Setting current_setting_;

  int selected_pass_;
  Qpx::Fitter selected_fitter_;

  std::vector<DataPoint> experiment_;


  //helper functions

  void remake_source_domains();
  void remake_sink_domains();
  void remake_domains();
  void start_new_pass();

  void display_data();
  void do_post_processing();
  void eval_dependent(DataPoint &data);
  bool criterion_satisfied(DataPoint &data);
  void update_name();
};

#endif // FORM_OPTIMIZATION_H
