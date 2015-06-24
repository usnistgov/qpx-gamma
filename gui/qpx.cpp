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
 * Description:
 *      qpx - main application window
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include <utility>
#include <numeric>
#include <cstdint>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "qpx.h"
#include "ui_qpx.h"
#include "custom_timer.h"

#include "form_list_daq.h"
#include "form_mca_daq.h"
#include "form_pixie_settings.h"
#include "form_oscilloscope.h"
#include "form_optimization.h"
#include "form_gain_match.h"

#include "ui_about.h"
#include "qt_util.h"

qpx::qpx(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::qpx),
  settings_("NIST-MML", "qpx"),
  my_emitter_(),
  qpx_stream_(),
  detectors_("Detectors"),
  text_buffer_(qpx_stream_, my_emitter_),
  runner_thread_()
{
  qRegisterMetaType<QList<QVector<double>>>("QList<QVector<double>>");
  qRegisterMetaType<Pixie::ListData>("Pixie::ListData");
  qRegisterMetaType<Pixie::Calibration>("Pixie::Calibration");
  qRegisterMetaType<Pixie::LiveStatus>("Pixie::LiveStatus");

  CustomLogger::initLogger(&qpx_stream_, "qpx_%N.log");
  ui->setupUi(this);
  connect(&my_emitter_, SIGNAL(writeLine(QString)), this, SLOT(add_log_text(QString)));

  loadSettings();

  connect(ui->qpxTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
  ui->statusBar->showMessage("Offline");

  main_tab_ = new FormStart(runner_thread_, settings_, detectors_);
  ui->qpxTabs->addTab(main_tab_, "Settings");
  connect(main_tab_, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Pixie::LiveStatus)), main_tab_, SLOT(toggle_push(bool,Pixie::LiveStatus)));
  connect(this, SIGNAL(settings_changed()), main_tab_, SLOT(settings_updated()));
  connect(this, SIGNAL(update_dets()), main_tab_, SLOT(detectors_updated()));
  ui->qpxTabs->setCurrentWidget(main_tab_);

  gui_enabled_ = true;
  px_status_ = Pixie::LiveStatus::dead;

  //on_pushAbout_clicked();
  PL_INFO << "Hello! Welcome to Multi-NAA at neutron guide D at NCNR. Please click boot to boot :)";

//for now available only in debug mode
//#ifdef QPX_DBG_
  ui->pushOpenOptimize->setEnabled(true);
  ui->pushOpenGainMatch->setEnabled(true);
//#endif

}

qpx::~qpx()
{
  delete ui;
}

void qpx::closeEvent(QCloseEvent *event) {
  if (runner_thread_.isRunning()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition operations",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      /*for (int i = ui->qpxTabs->count() - 1; i >= 0; --i)
        if (ui->qpxTabs->widget(i) != main_tab_)
          ui->qpxTabs->widget(i)->exit();*/

      runner_thread_.terminate();
      runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }


  for (int i = ui->qpxTabs->count() - 1; i >= 0; --i) {
    if (ui->qpxTabs->widget(i) != main_tab_) {
    ui->qpxTabs->setCurrentIndex(i);
    if (!ui->qpxTabs->widget(i)->close()) {
      event->ignore();
      return;
    } else {
      ui->qpxTabs->removeTab(i);
    }
    }
  }

  main_tab_->exit();
  main_tab_->close();

  saveSettings();
  event->accept();
}

void qpx::tabCloseRequested(int index) {
  ui->qpxTabs->setCurrentIndex(index);
  if (ui->qpxTabs->widget(index)->close())
    ui->qpxTabs->removeTab(index);
}

void qpx::on_OutputDirFind_clicked()
{
  QString dirName = QFileDialog::getExistingDirectory(this, "Open Directory", data_directory_,
                                                      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!dirName.isEmpty()) {
    data_directory_ = QDir(dirName).absolutePath();
    saveSettings();
  }
}

void qpx::add_log_text(QString line) {
  ui->qpxLogBox->append(line);
}

void qpx::loadSettings() {
  settings_.beginGroup("Program");
  QRect myrect = settings_.value("position",QRect(20,20,1234,650)).toRect();
  ui->splitter->restoreState(settings_.value("splitter").toByteArray());
  setGeometry(myrect);
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  detectors_.read_xml(data_directory_.toStdString() + "/default_detectors.det");
}

void qpx::saveSettings() {
  settings_.beginGroup("Program");
  settings_.setValue("position", this->geometry());
  settings_.setValue("splitter", ui->splitter->saveState());
  settings_.setValue("save_directory", data_directory_);
  settings_.endGroup();

  detectors_.write_xml(data_directory_.toStdString() + "/default_detectors.det");
}

void qpx::updateStatusText(QString text) {
  ui->statusBar->showMessage(text);
}

void qpx::toggleIO(bool enable) {
  px_status_ = Pixie::Wrapper::getInstance().settings().live();
  gui_enabled_ = enable;

  if (enable && (px_status_ == Pixie::LiveStatus::online))
    ui->statusBar->showMessage("Online");
  else if (enable && (px_status_ == Pixie::LiveStatus::offline))
    ui->statusBar->showMessage("Offline");
  else
    ui->statusBar->showMessage("Busy");

  emit toggle_push(enable, px_status_);
}

void qpx::on_splitter_splitterMoved(int pos, int index)
{
  ui->qpxLogBox->verticalScrollBar()->setValue(ui->qpxLogBox->verticalScrollBar()->maximum());
}

void qpx::on_pushAbout_clicked()
{
  QDialog* about = new QDialog(0,0);

  Ui_Dialog aboutUi;
  aboutUi.setupUi(about);
  about->setWindowTitle("About qpx");

  about->exec();
}

void qpx::detectors_updated() {
  emit update_dets();
}

void qpx::update_settings() {
  emit settings_changed();
}

void qpx::calibrate(FormCalibration* formCalib) {
  if (ui->qpxTabs->indexOf(formCalib) == -1) {
    ui->qpxTabs->addTab(formCalib, "Calibration");
    connect(formCalib, SIGNAL(detectorsChanged()), this, SLOT(detectors_updated()));
  }
  ui->qpxTabs->setCurrentWidget(formCalib);
  formCalib->update_spectrum();
}

void qpx::on_pushOpenSpectra_clicked()
{
  FormMcaDaq *newSpectraForm = new FormMcaDaq(runner_thread_, settings_, detectors_);
  ui->qpxTabs->addTab(newSpectraForm, "Spectra");
  connect(newSpectraForm, SIGNAL(requestCalibration(FormCalibration*)), this, SLOT(calibrate(FormCalibration*)));

  connect(newSpectraForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Pixie::LiveStatus)), newSpectraForm, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  ui->qpxTabs->setCurrentWidget(newSpectraForm);
  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::on_pushOpenList_clicked()
{
  FormListDaq *newListForm = new FormListDaq(runner_thread_, settings_);
  ui->qpxTabs->addTab(newListForm, "List mode");

  connect(newListForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(newListForm, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(this, SIGNAL(toggle_push(bool,Pixie::LiveStatus)), newListForm, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  ui->qpxTabs->setCurrentWidget(newListForm);
  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::on_pushOpenOptimize_clicked()
{
  //limit only one of these
  if (hasTab("Optimization"))
    return;

  FormOptimization *newOpt = new FormOptimization(runner_thread_, settings_, detectors_);
  ui->qpxTabs->addTab(newOpt, "Optimization");

  connect(newOpt, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));
  connect(newOpt, SIGNAL(settings_changed()), this, SLOT(update_settings()));

  connect(newOpt, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Pixie::LiveStatus)), newOpt, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  ui->qpxTabs->setCurrentWidget(newOpt);
  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::on_pushOpenGainMatch_clicked()
{
  //limit only one of these
  if (hasTab("Gain matching"))
    return;

  FormGainMatch *newGain = new FormGainMatch(runner_thread_, settings_, detectors_);
  ui->qpxTabs->addTab(newGain, "Gain matching");

  connect(newGain, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));

  connect(newGain, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Pixie::LiveStatus)), newGain, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  ui->qpxTabs->setCurrentWidget(newGain);
  emit toggle_push(gui_enabled_, px_status_);
}

bool qpx::hasTab(QString tofind) {
  for (int i = 0; i < ui->qpxTabs->count(); ++i)
    if (ui->qpxTabs->tabText(i) == tofind)
      return true;
  return false;
}
