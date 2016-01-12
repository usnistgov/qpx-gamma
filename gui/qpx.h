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
 *      qpx - main application window
 *
 ******************************************************************************/

#ifndef qpx_H
#define qpx_H

#include <QMainWindow>
#include <QSettings>

#include "custom_logger.h"
#include "qt_boost_logger.h"

#include "thread_runner.h"

#include "form_start.h"
#include "form_analysis_1d.h"
#include "form_analysis_2d.h"
#include "form_symmetrize2d.h"
#include "form_efficiency_calibration.h"


namespace Ui {
class qpx;
}

class CloseTabButton : public QPushButton {
  Q_OBJECT
public:
  CloseTabButton(QWidget* pt) : parent_tab_(pt) {
    connect(this, SIGNAL(clicked(bool)), this, SLOT(closeme(bool)));
  }
private slots:
  void closeme(bool) { emit closeTab(parent_tab_); }
signals:
  void closeTab(QWidget*);
protected:
  QWidget *parent_tab_;
};

class qpx : public QMainWindow
{
  Q_OBJECT

public:
  explicit qpx(QWidget *parent = 0);
  ~qpx();

private:
  Ui::qpx *ui;

  //connect gui with boost logger framework
  std::stringstream qpx_stream_;
  LogEmitter        my_emitter_;
  LogStreamBuffer   text_buffer_;

  QString                       settings_directory_;    //settings directory
  QString                       data_directory_;        //data directory
  QSettings                     settings_;
  XMLableDB<Gamma::Detector>    detectors_;
  std::vector<Gamma::Detector>  current_dets_;
  ThreadRunner                  runner_thread_;

  FormStart* main_tab_;
  bool gui_enabled_;
  Qpx::DeviceStatus px_status_;

  //helper functions
  void saveSettings();
  void loadSettings();

  void reorder_tabs();

signals:
  void toggle_push(bool, Qpx::DeviceStatus);
  void settings_changed();
  void update_dets();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void update_settings(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus);
  void toggleIO(bool);
  void updateStatusText(QString);

  void tabCloseRequested(int index);
  void closeTab(QWidget*);

  //logger receiver
  void add_log_text(QString);

  void on_splitter_splitterMoved(int pos, int index);

  void analyze_1d(FormAnalysis1D*);
  void analyze_2d(FormAnalysis2D*);
  void symmetrize_2d(FormSymmetrize2D*);
  void eff_cal(FormEfficiencyCalibration*);

  void detectors_updated();
  void update_settings();

  void openNewProject();

  bool hasTab(QString);

  void choose_profiles();
  void profile_chosen(QString, bool);

  void open_gain_matching();
  void open_optimization();
  void open_list();

  void tabs_moved(int, int);
  void addClosableTab(QWidget*, QString);
  void tab_changed(int);

};

#endif // qpx_H
