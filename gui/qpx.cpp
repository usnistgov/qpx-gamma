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
#include "form_system_settings.h"
#include "form_oscilloscope.h"
#include "form_optimization.h"
#include "form_gain_match.h"

#include "widget_profiles.h"

#include "qt_util.h"

qpx::qpx(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::qpx),
  settings_("NIST-MML", "qpx"),
  my_emitter_(),
  qpx_stream_(),
  main_tab_(nullptr),
  detectors_("Detectors"),
  text_buffer_(qpx_stream_, my_emitter_),
  runner_thread_()
{
  qRegisterMetaType<std::vector<Qpx::Trace>>("std::vector<Qpx::Trace>");
  qRegisterMetaType<std::vector<Gamma::Detector>>("std::vector<Gamma::Detector>");
  qRegisterMetaType<Qpx::ListData>("Qpx::ListData");
  qRegisterMetaType<Gamma::Setting>("Gamma::Setting");
  qRegisterMetaType<Gamma::Calibration>("Gamma::Calibration");
  qRegisterMetaType<Qpx::DeviceStatus>("Qpx::DeviceStatus");

  CustomLogger::initLogger(&qpx_stream_, "qpx_%N.log");
  ui->setupUi(this);
  connect(&my_emitter_, SIGNAL(writeLine(QString)), this, SLOT(add_log_text(QString)));

  connect(&runner_thread_, SIGNAL(settingsUpdated(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus)),
          this, SLOT(update_settings(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus)));

  loadSettings();

  connect(ui->qpxTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
  ui->statusBar->showMessage("Offline");

  gui_enabled_ = true;
  px_status_ = Qpx::DeviceStatus(0);

  QPushButton *tb = new QPushButton();
  tb->setIcon(QIcon(":/new/icons/oxy/filenew.png"));
  tb->setIconSize(QSize(16, 16));
  tb->setToolTip("New project");
  tb->setFlat(true);
  connect(tb, SIGNAL(clicked()), this, SLOT(openNewProject()));
  // Add empty, not enabled tab to tabWidget
  ui->qpxTabs->addTab(new QLabel("<center>Open new project by clicking \"+\"</center>"), QString());
  ui->qpxTabs->setTabEnabled(0, false);
  // Add tab button to current tab. Button will be enabled, but tab -- not
  ui->qpxTabs->tabBar()->setTabButton(0, QTabBar::RightSide, tb);

  connect(ui->qpxTabs->tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(tabs_moved(int,int)));

  QTimer::singleShot(50, this, SLOT(choose_profiles()));
}

qpx::~qpx()
{
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

  if (main_tab_ != nullptr) {
    main_tab_->exit();
    main_tab_->close();
  }

  saveSettings();
  event->accept();
}

void qpx::choose_profiles() {
  WidgetProfiles *profiles = new WidgetProfiles(settings_, this);
  connect(profiles, SIGNAL(profileChosen(QString, bool)), this, SLOT(profile_chosen(QString, bool)));
  profiles->exec();
}

void qpx::profile_chosen(QString profile, bool boot) {
  loadSettings();

  main_tab_ = new FormStart(runner_thread_, settings_, detectors_, profile, this);
  ui->qpxTabs->addTab(main_tab_, main_tab_->windowTitle());
  connect(main_tab_, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::DeviceStatus)), main_tab_, SLOT(toggle_push(bool,Qpx::DeviceStatus)));
  connect(this, SIGNAL(settings_changed()), main_tab_, SLOT(settings_updated()));
  connect(this, SIGNAL(update_dets()), main_tab_, SLOT(detectors_updated()));
  connect(main_tab_, SIGNAL(optimization_requested()), this, SLOT(open_optimization()));
  connect(main_tab_, SIGNAL(gain_matching_requested()), this, SLOT(open_gain_matching()));
  connect(main_tab_, SIGNAL(list_view_requested()), this, SLOT(open_list()));

  ui->qpxTabs->setCurrentWidget(main_tab_);
  reorder_tabs();

  if (boot)
    runner_thread_.do_boot();

}

void qpx::tabCloseRequested(int index) {
  if ((index < 0) || (index >= ui->qpxTabs->count()))
      return;
  ui->qpxTabs->setCurrentIndex(index);
  if (ui->qpxTabs->widget(index)->close())
    ui->qpxTabs->removeTab(index);
}

void qpx::add_log_text(QString line) {
  ui->qpxLogBox->append(line);
}

void qpx::loadSettings() {
  settings_.beginGroup("Program");
  QRect myrect = settings_.value("position",QRect(20,20,1234,650)).toRect();
  ui->splitter->restoreState(settings_.value("splitter").toByteArray());
  setGeometry(myrect);

  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();

  settings_.endGroup();

  detectors_.clear();
  detectors_.read_xml(settings_directory_.toStdString() + "/default_detectors.det");
}

void qpx::saveSettings() {
  settings_.beginGroup("Program");
  settings_.setValue("position", this->geometry());
  settings_.setValue("splitter", ui->splitter->saveState());
  settings_.setValue("settings_directory", settings_directory_);
  settings_.endGroup();

  detectors_.write_xml(settings_directory_.toStdString() + "/default_detectors.det");
}

void qpx::updateStatusText(QString text) {
  ui->statusBar->showMessage(text);
}

void qpx::update_settings(Gamma::Setting sets, std::vector<Gamma::Detector> channels, Qpx::DeviceStatus status) {
  px_status_ = status;
  current_dets_ = channels;
  toggleIO(true);
}

void qpx::toggleIO(bool enable) {
  gui_enabled_ = enable;

  if (enable && (px_status_ & Qpx::DeviceStatus::booted))
    ui->statusBar->showMessage("Online");
  else if (enable)
    ui->statusBar->showMessage("Offline");
  else
    ui->statusBar->showMessage("Busy");

  for (int i = 0; i < ui->qpxTabs->count(); ++i)
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
  FormMcaDaq *newSpectraForm = new FormMcaDaq(runner_thread_, settings_, detectors_, current_dets_, this);
  connect(newSpectraForm, SIGNAL(requestAnalysis(FormAnalysis1D*)), this, SLOT(analyze_1d(FormAnalysis1D*)));
  connect(newSpectraForm, SIGNAL(requestAnalysis2D(FormAnalysis2D*)), this, SLOT(analyze_2d(FormAnalysis2D*)));
  connect(newSpectraForm, SIGNAL(requestSymmetriza2D(FormSymmetrize2D*)), this, SLOT(symmetrize_2d(FormSymmetrize2D*)));
  connect(newSpectraForm, SIGNAL(requestEfficiencyCal(FormEfficiencyCalibration*)), this, SLOT(eff_cal(FormEfficiencyCalibration*)));
  connect(newSpectraForm, SIGNAL(requestClose(QWidget*)), this, SLOT(closeTab(QWidget*)));

  connect(newSpectraForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::DeviceStatus)), newSpectraForm, SLOT(toggle_push(bool,Qpx::DeviceStatus)));

  addClosableTab(newSpectraForm, "Close project");
  ui->qpxTabs->setCurrentWidget(newSpectraForm);
  reorder_tabs();

  newSpectraForm->toggle_push(true, px_status_);
}

void qpx::addClosableTab(QWidget* widget, QString tooltip) {
  CloseTabButton *cb = new CloseTabButton(widget);
  cb->setIcon( QIcon::fromTheme("window-close"));
//  tb->setIconSize(QSize(16, 16));
  cb->setToolTip(tooltip);
  cb->setFlat(true);
  connect(cb, SIGNAL(closeTab(QWidget*)), this, SLOT(closeTab(QWidget*)));
  ui->qpxTabs->addTab(widget, widget->windowTitle());
  ui->qpxTabs->tabBar()->setTabButton(ui->qpxTabs->count()-1, QTabBar::RightSide, cb);
}


void qpx::closeTab(QWidget* w) {
  int idx = ui->qpxTabs->indexOf(w);
  PL_DBG << "requested tab close " << idx;
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

  FormListDaq *newListForm = new FormListDaq(runner_thread_, settings_, this);
  addClosableTab(newListForm, "Close");

  connect(newListForm, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(newListForm, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::DeviceStatus)), newListForm, SLOT(toggle_push(bool,Qpx::DeviceStatus)));

  ui->qpxTabs->setCurrentWidget(newListForm);

  reorder_tabs();

  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::open_optimization()
{
  //limit only one of these
  if (hasTab("Optimization") || hasTab("Optimization >>"))
    return;

  FormOptimization *newOpt = new FormOptimization(runner_thread_, settings_, detectors_, this);
  addClosableTab(newOpt, "Close");

  connect(newOpt, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));
  connect(newOpt, SIGNAL(settings_changed()), this, SLOT(update_settings()));

  connect(newOpt, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::DeviceStatus)), newOpt, SLOT(toggle_push(bool,Qpx::DeviceStatus)));

  ui->qpxTabs->setCurrentWidget(newOpt);
  reorder_tabs();

  emit toggle_push(gui_enabled_, px_status_);
}

void qpx::open_gain_matching()
{
  //limit only one of these
  if (hasTab("Gain matching") || hasTab("Gain matching >>"))
    return;

  FormGainMatch *newGain = new FormGainMatch(runner_thread_, settings_, detectors_, this);
  addClosableTab(newGain, "Close");

  connect(newGain, SIGNAL(optimization_approved()), this, SLOT(detectors_updated()));

  connect(newGain, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO(bool)));
  connect(this, SIGNAL(toggle_push(bool,Qpx::DeviceStatus)), newGain, SLOT(toggle_push(bool,Qpx::DeviceStatus)));

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
