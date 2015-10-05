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
 *      MCA acquisition interface
 *
 ******************************************************************************/

#ifndef FORM_MCA_DAQ_H
#define FORM_MCA_DAQ_H

#include <QWidget>
#include <QSettings>
#include "spectra_set.h"
#include "thread_runner.h"
#include "thread_plot_signal.h"
#include "form_analysis_1d.h"
#include "form_analysis_2d.h"

namespace Ui {
class FormMcaDaq;
}

class FormMcaDaq : public QWidget
{
  Q_OBJECT

public:
  explicit FormMcaDaq(ThreadRunner&, QSettings&, XMLable2DB<Gamma::Detector>&, QWidget *parent = 0);

  void replot();
  ~FormMcaDaq();

signals:
  void toggleIO(bool);
  void statusText(QString);
  void requestAnalysis(FormAnalysis1D*);
  void requestAnalysis2D(FormAnalysis2D*);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void on_pushMcaStart_clicked();
  void on_pushMcaStop_clicked();
  void on_pushMcaSimulate_clicked();
  void on_pushMcaSave_clicked();
  void on_pushMcaLoad_clicked();
  void on_pushMcaClear_clicked();
  void on_pushMcaReload_clicked();
  void on_pushTimeNow_clicked();
  void run_completed();
  void on_pushEditSpectra_clicked();
  void update_plots();
  void clearGraphs();

  void newProject();
  void updateSpectraUI();


  void toggle_push(bool, Qpx::LiveStatus);

  void on_pushEnable2d_clicked();

  void on_pushForceRefresh_clicked();

  void reqAnal(QString);
  void analysis_destroyed();

  void reqAnal2D(QString);
  void analysis2d_destroyed();


  void on_pushBuildFromList_clicked();

  void on_pushDetails_clicked();

private:
  Ui::FormMcaDaq *ui;
  QSettings                  &settings_;
  ThreadRunner               &runner_thread_;
  XMLable2DB<Gamma::Detector> &detectors_;

  QString data_directory_;    //data directory
  QString mca_load_formats_;  //valid mca file formats that can be opened

  XMLable2DB<Qpx::Spectrum::Template>  spectra_templates_;
  Qpx::SpectraSet                     spectra_;

  ThreadPlotSignal                plot_thread_;
  boost::atomic<bool>             interruptor_;

  FormAnalysis1D* my_analysis_;
  FormAnalysis2D* my_analysis_2d_;
  bool my_run_;

  void loadSettings();
  void saveSettings();
};

#endif // FORM_MCA_DAQ_H
