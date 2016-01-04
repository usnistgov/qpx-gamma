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
#include "form_symmetrize2d.h"

namespace Ui {
class FormMcaDaq;
}

class FormMcaDaq : public QWidget
{
  Q_OBJECT

public:
  explicit FormMcaDaq(ThreadRunner&, QSettings&, XMLableDB<Gamma::Detector>&, QWidget *parent = 0);

  void replot();
  ~FormMcaDaq();

signals:
  void toggleIO(bool);
  void statusText(QString);
  void requestAnalysis(FormAnalysis1D*);
  void requestAnalysis2D(FormAnalysis2D*);
  void requestSymmetriza2D(FormSymmetrize2D*);
  void requestEfficiencyCal();

protected:
  void closeEvent(QCloseEvent*);

public slots:
  void toggle_push(bool, Qpx::DeviceStatus);

private slots:
  void on_pushMcaStart_clicked();
  void on_pushMcaStop_clicked();
  void on_pushTimeNow_clicked();
  void run_completed();
  void on_pushEditSpectra_clicked();
  void update_plots();
  void after_export();
  void clearGraphs();

  void start_DAQ();
  void newProject();
  void updateSpectraUI();


  void on_pushEnable2d_clicked();

  void on_pushForceRefresh_clicked();

  void reqAnal(QString);
  void analysis_destroyed();

  void reqAnal2D(QString);
  void reqSym2D(QString);
  void analysis2d_destroyed();
  void sym2d_destroyed();

  void on_pushDetails_clicked();

  void on_toggleIndefiniteRun_clicked();

  void on_pushEfficiencyCal_clicked();

  void projectSave();
  void projectSaveAs();
  void projectExport();
  void projectOpen();
  void projectImport();

private:
  Ui::FormMcaDaq *ui;
  QSettings                  &settings_;
  ThreadRunner               &runner_thread_;
  XMLableDB<Gamma::Detector> &detectors_;

  QString data_directory_;    //data directory
  QString settings_directory_;

  QString mca_load_formats_;  //valid mca file formats that can be opened

  XMLableDB<Qpx::Spectrum::Template>  spectra_templates_;
  Qpx::SpectraSet                     spectra_;

  ThreadPlotSignal                plot_thread_;
  boost::atomic<bool>             interruptor_;

  FormAnalysis1D* my_analysis_;
  FormAnalysis2D* my_analysis_2d_;
  FormSymmetrize2D* my_symmetrization_2d_;
  bool my_run_;

  QMenu  menuLoad;
  QMenu  menuSave;


  void loadSettings();
  void saveSettings();
};

#endif // FORM_MCA_DAQ_H
