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

  void update_plots();
  void update_fits();
  void update_peak_selection(std::set<double>);
  void update_pass_selection();

  void run_completed();


  void on_pushStart_clicked();
  void on_pushStop_clicked();
  void on_comboTarget_currentIndexChanged(int index);
  void on_comboSetting_activated(const QString &arg1);
  void on_comboUntil_activated(const QString &arg1);
  void on_pushAddCustom_clicked();

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


  Qpx::Setting manual_settings_;
  std::map<std::string, Qpx::Setting> all_settings_;


  int current_pass_;
  Qpx::Setting current_setting_;

  int selected_pass_;
  Qpx::Fitter selected_fitter_;

  std::vector<DataPoint> experiment_;


  //helper functions

  void new_run();
  Qpx::Metadata make_prototype(uint16_t bits, uint16_t channel,
                               int pass_number, Qpx::Setting variable,
                               std::string name);
  void populate_display();
  void do_post_processing();
  void eval_dependent(DataPoint &data);
  bool criterion_satisfied(DataPoint &data);
};

#endif // FORM_OPTIMIZATION_H
