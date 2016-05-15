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
#include "fitter.h"

namespace Ui {
class FormGainMatch;
}

class FormGainMatch : public QWidget
{
  Q_OBJECT

private:
  struct DataPoint
  {
    Qpx::Fitter spectrum;
    Qpx::Peak selected_peak;
    double independent_variable;
    double dependent_variable;
    double dep_uncert;
  };

public:
  explicit FormGainMatch(ThreadRunner&, XMLableDB<Qpx::Detector>&, QWidget *parent = 0);
  void update_settings();
  ~FormGainMatch();

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
  void update_fit_ref();
  void update_fit_opt();
  void fitting_done_ref();
  void fitting_done_opt();
  void update_peak_selection(std::set<double>);

  void pass_selected_in_table();
  void pass_selected_in_plot();


  void on_pushStart_clicked();
  void on_pushStop_clicked();

  void on_comboReference_currentIndexChanged(int index);
  void on_comboTarget_currentIndexChanged(int index);
  void on_comboSetting_activated(int index);

  void on_pushEditPrototypeRef_clicked();
  void on_pushEditPrototypeOpt_clicked();


  void on_pushAddCustom_clicked();
  void on_pushEditCustom_clicked();
  void on_pushDeleteCustom_clicked();


private:
  void loadSettings();
  void saveSettings();

  XMLableDB<Qpx::Detector> &detectors_;
  Ui::FormGainMatch *ui;
  AppearanceProfile style_fit, style_pts;


  ThreadRunner         &gm_runner_thread_;
  ThreadPlotSignal     gm_plot_thread_;
  Qpx::Project         project_;
  boost::atomic<bool>  interruptor_;
  bool my_run_;


  std::vector<Qpx::Detector> current_dets_;
  Qpx::Metadata sink_prototype_ref_;
  Qpx::Metadata sink_prototype_opt_;

  std::map<std::string, Qpx::Setting> manual_settings_;
  std::map<std::string, Qpx::Setting> source_settings_;
  std::map<std::string, Qpx::Setting> all_settings_;


  int current_pass_;
  Qpx::Setting current_setting_;

  int selected_pass_;
  Qpx::Fitter fitter_ref_,
              fitter_opt_;

  Qpx::Peak peak_ref_;
  PolyBounded response_function_;

  std::vector<DataPoint> experiment_;


  //helper functions

  void init_prototypes();

  Qpx::Metadata make_prototype(uint16_t bits, uint16_t channel,
                               std::string name);

  void display_data();
  void remake_source_domains();
  void remake_domains();
  void do_post_processing(); //check this

  void eval_dependent(DataPoint &data);
  bool criterion_satisfied(DataPoint &data);

  void start_new_pass();

  void update_name();
};

#endif // FORM_OPTIMIZATION_H
