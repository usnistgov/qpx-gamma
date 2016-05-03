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

#include <QSettings>
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
#include "form_system_settings.h"
#include "form_oscilloscope.h"
#include "form_optimization.h"
#include "form_gain_match.h"

#include "qt_util.h"

qpx::qpx(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::qpx),
  my_emitter_(),
  qpx_stream_(),
  main_tab_(nullptr),
  detectors_("Detectors"),
  text_buffer_(qpx_stream_, my_emitter_),
  runner_thread_()
{
  qRegisterMetaType<std::vector<Qpx::Trace>>("std::vector<Qpx::Trace>");
  qRegisterMetaType<std::vector<Qpx::Detector>>("std::vector<Qpx::Detector>");
  qRegisterMetaType<Qpx::ListData>("Qpx::ListData");
  qRegisterMetaType<Qpx::Setting>("Qpx::Setting");
  qRegisterMetaType<Qpx::Calibration>("Qpx::Calibration");
  qRegisterMetaType<Qpx::SourceStatus>("Qpx::SourceStatus");
  qRegisterMetaType<Qpx::Fitter>("Qpx::Fitter");
  qRegisterMetaType<boost::posix_time::time_duration>("boost::posix_time::time_duration");

  CustomLogger::initLogger(&qpx_stream_, "qpx_%N.log");
  ui->setupUi(this);
  connect(&my_emitter_, SIGNAL(writeLine(QString)), this, SLOT(add_log_text(QString)));

  connect(&runner_thread_, SIGNAL(settingsUpdated(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::SourceStatus)),
          this, SLOT(update_settings(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::SourceStatus)));

  loadSettings();

  connect(ui->qpxTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
  ui->statusBar->showMessage("Offline");

  gui_enabled_ = true;
  px_status_ = Qpx::SourceStatus(0);

  QPushButton *tb = new QPushButton();
  tb->setIcon(QIcon(":/icons/oxy/16/filenew.png"));
//  tb->setIconSize(QSize(16, 16));
  tb->setToolTip("New project");
  tb->setFlat(true);
  connect(tb, SIGNAL(clicked()), this, SLOT(openNewProject()));
  // Add empty, not enabled tab to tabWidget
  ui->qpxTabs->addTab(new QLabel("<center>Open new project by clicking \"+\"</center>"), QString());
  ui->qpxTabs->setTabEnabled(0, false);
  // Add tab button to current tab. Button will be enabled, but tab -- not
  ui->qpxTabs->tabBar()->setTabButton(0, QTabBar::RightSide, tb);

  connect(ui->qpxTabs->tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(tabs_moved(int,int)));
  connect(ui->qpxTabs, SIGNAL(currentChanged(int)), this, SLOT(tab_changed(int)));

  main_tab_ = new FormStart(runner_thread_, detectors_, this);
  ui->qpxTabs->addTab(main_tab_, "DAQ");
//  ui->qpxTabs->addTab(main_tab_, main_tab_->windowTitle());
  ui->qpxTabs->setTabIcon(ui->qpxTabs->count() - 1, QIcon(":/icons/oxy/16/applications_systemg.png"));
  connect(main_tab_, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::SourceStatus)), main_tab_, SLOT(toggle_push(bool,Qpx::SourceStatus)));
  connect(this, SIGNAL(settings_changed()), main_tab_, SLOT(settings_updated()));
  connect(this, SIGNAL(update_dets()), main_tab_, SLOT(detectors_updated()));
  connect(main_tab_, SIGNAL(optimization_requested()), this, SLOT(open_optimization()));
  connect(main_tab_, SIGNAL(gain_matching_requested()), this, SLOT(open_gain_matching()));
  connect(main_tab_, SIGNAL(list_view_requested()), this, SLOT(open_list()));

  QSettings settings;
  settings.beginGroup("Program");
  QString profile_directory = settings.value("profile_directory", "").toString();

  if (!profile_directory.isEmpty())
    ui->qpxTabs->setCurrentWidget(main_tab_);

  reorder_tabs();
}

qpx::~qpx()
{
  CustomLogger::closeLogger();
  delete ui;
}

void qpx::closeEvent(QCloseEvent *event) {
  if (runner_thread_.running()) {
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
  } else {
    runner_thread_.terminate();
    runner_thread_.wait();
  }


  for (int i = ui->qpxTabs->count() - 2; i >= 0; --i) {
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

  if (main_tab_ != nullptr) {
    main_tab_->exit();
    main_tab_->close();
  }

  saveSettings();
  event->accept();
}

void qpx::tabCloseRequested(int index) {
  if ((index < 0) || (index >= ui->qpxTabs->count()))
      return;
  ui->qpxTabs->setCurrentIndex(index);
  if (ui->qpxTabs->widget(index)->close())
    ui->qpxTabs->removeTab(index);
}

void qpx::tab_changed(int index) {
  if ((index < 0) || (index >= ui->qpxTabs->count()))
    return;
  if (main_tab_ == nullptr)
    return;
  runner_thread_.set_idle_refresh(ui->qpxTabs->widget(index) == main_tab_);
}

void qpx::add_log_text(QString line) {
  ui->qpxLogBox->append(line);
}

void qpx::loadSettings() {
  QSettings settings;
  settings.beginGroup("Program");
  QRect myrect = settings.value("position",QRect(20,20,1234,650)).toRect();
  ui->splitter->restoreState(settings.value("splitter").toByteArray());
  setGeometry(myrect);

  QString settings_directory = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  detectors_.clear();
  detectors_.read_xml(settings_directory.toStdString() + "/default_detectors.det");
}

void qpx::saveSettings() {
  QSettings settings;
  settings.beginGroup("Program");
  settings.setValue("position", this->geometry());
  settings.setValue("splitter", ui->splitter->saveState());

  QString settings_directory = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  detectors_.write_xml(settings_directory.toStdString() + "/default_detectors.det");
}

void qpx::updateStatusText(QString text) {
  ui->statusBar->showMessage(text);
}

void qpx::update_settings(Qpx::Setting sets, std::vector<Qpx::Detector> channels, Qpx::SourceStatus status) {
  px_status_ = status;
  current_dets_ = channels;
  toggleIO(true);
}

void qpx::toggleIO(bool enable) {
  gui_enabled_ = enable;

  if (enable && (px_status_ & Qpx::SourceStatus::booted))
    ui->statusBar->showMessage("Online");
  else if (enable)
    ui->statusBar->showMessage("Offline");
  else
    ui->statusBar->showMessage("Busy");

  for (int i = 0; i < ui->qpxTabs->count(); ++i)
    if (ui->qpxTabs->widget(i) != main_tab_)
      ui->qpxTabs->setTabText(i, ui->qpxTabs->widget(i)->windowTitle());

  emit toggle_push(enable, px_status_);
}

void qpx::on_splitter_splitterMoved(int pos, int index)
{
  ui->qpxLogBox->verticalScrollBar()->setValue(ui->qpxLogBox->verticalScrollBar()->maximum());
}

void qpx::detectors_updated() {
  emit update_dets();
}

void qpx::update_settings() {
  emit settings_changed();
}

void qpx::analyze_1d(FormAnalysis1D* formAnal) {
  int idx = ui->qpxTabs->indexOf(formAnal);
  if (idx == -1) {
    addClosableTab(formAnal, "Close");
    connect(formAnal, SIGNAL(detectorsChanged()), this, SLOT(detectors_updated()));
  } else
    ui->qpxTabs->setTabText(idx, formAnal->windowTitle());
  ui->qpxTabs->setCurrentWidget(formAnal);
  formAnal->update_spectrum();
  reorder_tabs();
}

void qpx::analyze_2d(FormAnalysis2D* formAnal) {
  int idx = ui->qpxTabs->indexOf(formAnal);
  if (idx == -1) {
    addClosableTab(formAnal, "Close");
    connect(formAnal, SIGNAL(detectorsChanged()), this, SLOT(detectors_updated()));
  } else
    ui->qpxTabs->setTabText(idx, formAnal->windowTitle());
  ui->qpxTabs->setCurrentWidget(formAnal);
  reorder_tabs();
}

void qpx::symmetrize_2d(FormSymmetrize2D* formSym) {
  int idx = ui->qpxTabs->indexOf(formSym);
  if (idx == -1) {
    addClosableTab(formSym, "Close");
    connect(formSym, SIGNAL(detectorsChanged()), this, SLOT(detectors_updated()));
  } else
    ui->qpxTabs->setTabText(idx, formSym->windowTitle());
  ui->qpxTabs->setCurrentWidget(formSym);
  reorder_tabs();

}

void qpx::eff_cal(FormEfficiencyCalibration *formEf) {
  int idx = ui->qpxTabs->indexOf(formEf);
  if (idx == -1) {
    addClosableTab(formEf, "Close");
    connect(formEf, SIGNAL(detectorsChanged()), this, SLOT(detectors_updated()));
  } else
    ui->qpxTabs->setTabText(idx, formEf->windowTitle());
  ui->qpxTabs->setCurrentWidget(formEf);
  reorder_tabs();
}

void qpx::openNewProject()
{
  FormMcaDaq *newSpectraForm = new FormMcaDaq(runner_thread_, detectors_, current_dets_, this);
  connect(newSpectraForm, SIGNAL(requestAnalysis(FormAnalysis1D*)), this, SLOT(analyze_1d(FormAnalysis1D*)));
  connect(newSpectraForm, SIGNAL(requestAnalysis2D(FormAnalysis2D*)), this, SLOT(analyze_2d(FormAnalysis2D*)));
  connect(newSpectraForm, SIGNAL(requestSymmetriza2D(FormSymmetrize2D*)), this, SLOT(symmetrize_2d(FormSymmetrize2D*)));
  connect(newSpectraForm, SIGNAL(requestEfficiencyCal(FormEfficiencyCalibration*)), this, SLOT(eff_cal(FormEfficiencyCalibration*)));
  connect(newSpectraForm, SIGNAL(requestClose(QWidget*)), this, SLOT(closeTab(QWidget*)));

  connect(newSpectraForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::SourceStatus)), newSpectraForm, SLOT(toggle_push(bool,Qpx::SourceStatus)));

  addClosableTab(newSpectraForm, "Close project");
  ui->qpxTabs->setCurrentWidget(newSpectraForm);
  reorder_tabs();

  newSpectraForm->toggle_push(true, px_status_);
}

void qpx::addClosableTab(QWidget* widget, QString tooltip) {
  CloseTabButton *cb = new CloseTabButton(widget);
  cb->setIcon( QIcon(":/icons/oxy/16/application_exit.png"));
//  tb->setIconSize(QSize(16, 16));
  cb->setToolTip(tooltip);
  cb->setFlat(true);
  connect(cb, SIGNAL(closeTab(QWidget*)), this, SLOT(closeTab(QWidget*)));
  ui->qpxTabs->addTab(widget, widget->windowTitle());
  ui->qpxTabs->tabBar()->setTabButton(ui->qpxTabs->count()-1, QTabBar::RightSide, cb);
}

void qpx::closeTab(QWidget* w) {
  int idx = ui->qpxTabs->indexOf(w);
  tabCloseRequested(idx);
}

void qpx::reorder_tabs() {
  for (int i = 0; i < ui->qpxTabs->count(); ++i)
    if (ui->qpxTabs->tabText(i).isEmpty() && (i != (ui->qpxTabs->count() - 1)))
      ui->qpxTabs->tabBar()->moveTab(i, ui->qpxTabs->count() - 1);
}

void qpx::tabs_moved(int, int) {
  reorder_tabs();
}

void qpx::open_list()
{
  if (hasTab("List view") || hasTab("List view >>"))
    return;

  FormListDaq *newListForm = new FormListDaq(runner_thread_, this);
  addClosableTab(newListForm, "Close");

  connect(newListForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(newListForm, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::SourceStatus)), newListForm, SLOT(toggle_push(bool,Qpx::SourceStatus)));

  ui->qpxTabs->setCurrentWidget(newListForm);

  reorder_tabs();

  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::open_optimization()
{
  //limit only one of these
  if (hasTab("Optimization") || hasTab("Optimization >>"))
    return;

  FormOptimization *newOpt = new FormOptimization(runner_thread_, detectors_, this);
  addClosableTab(newOpt, "Close");

  connect(newOpt, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));
  connect(newOpt, SIGNAL(settings_changed()), this, SLOT(update_settings()));

  connect(newOpt, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::SourceStatus)), newOpt, SLOT(toggle_push(bool,Qpx::SourceStatus)));

  ui->qpxTabs->setCurrentWidget(newOpt);
  reorder_tabs();

  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::open_gain_matching()
{
  //limit only one of these
  if (hasTab("Gain matching") || hasTab("Gain matching >>"))
    return;

  FormGainMatch *newGain = new FormGainMatch(runner_thread_, detectors_, this);
  addClosableTab(newGain, "Close");

  connect(newGain, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));

  connect(newGain, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::SourceStatus)), newGain, SLOT(toggle_push(bool,Qpx::SourceStatus)));

  ui->qpxTabs->setCurrentWidget(newGain);
  reorder_tabs();

  emit toggle_push(gui_enabled_, px_status_);
}

bool qpx::hasTab(QString tofind) {
  for (int i = 0; i < ui->qpxTabs->count(); ++i)
    if (ui->qpxTabs->tabText(i) == tofind)
      return true;
  return false;
}
