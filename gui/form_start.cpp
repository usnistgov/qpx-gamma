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
 *      Polynomial fit
 *
 ******************************************************************************/

#include "form_start.h"
#include <QBoxLayout>

FormStart::FormStart(ThreadRunner &thread, QSettings &settings, XMLableDB<Gamma::Detector> &detectors, QString profile, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  profile_path_(profile),
  exiting(false)
{
  connect(&thread, SIGNAL(settingsUpdated(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus)),
          this, SLOT(update(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus)));

  formOscilloscope = new FormOscilloscope(runner_thread_, settings);
  connect(formOscilloscope, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::DeviceStatus)), formOscilloscope, SLOT(toggle_push(bool,Qpx::DeviceStatus)));

  formSettings = new FormSystemSettings(runner_thread_, detectors_, settings_);
  connect(formSettings, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(formSettings, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  
  connect(this, SIGNAL(refresh()), formSettings, SLOT(update()));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::DeviceStatus)), formSettings, SLOT(toggle_push(bool,Qpx::DeviceStatus)));
  connect(this, SIGNAL(update_dets()), formSettings, SLOT(updateDetDB()));

  QHBoxLayout *lo = new QHBoxLayout();
  lo->addWidget(formOscilloscope);
  lo->addWidget(formSettings);

  this->setLayout(lo);

  loadSettings();
}

FormStart::~FormStart()
{
  //  delete ui;
}

void FormStart::update(Gamma::Setting tree, std::vector<Gamma::Detector> dets, Qpx::DeviceStatus status) {
  formOscilloscope->updateMenu(dets);
}

void FormStart::closeEvent(QCloseEvent *event) {
  if (exiting) {
    saveSettings();
    formSettings->close();
    formOscilloscope->close();
    event->accept();
  }
  else
    event->ignore();
  return;
}

void FormStart::exit() {
  exiting = true;
}

void FormStart::updateStatusText(QString txt) {
  emit statusText(txt);
}

void FormStart::toggleIO_(bool enable) {
  emit toggleIO(enable);
}

void FormStart::toggle_push(bool enable, Qpx::DeviceStatus live) {
  emit toggle_push_(enable, live);
}

void FormStart::boot_complete(bool success, Qpx::DeviceStatus status) {
  //DEPRECATED

  if (success) {
    this->setCursor(Qt::WaitCursor);
    formSettings->updateDetDB();
    if (status == Qpx::DeviceStatus::can_oscil)
      runner_thread_.do_oscil(formOscilloscope->xdt());
    runner_thread_.do_refresh_settings();
    this->setCursor(Qt::ArrowCursor);
  }
}

void FormStart::settings_updated() {
  emit refresh();
}

void FormStart::detectors_updated() {
  emit update_dets();
}

void FormStart::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  runner_thread_.do_initialize(profile_path_);
}

void FormStart::saveSettings() {
  Gamma::Setting dev_settings = formSettings->get_tree();

  pugi::xml_document doc;

  dev_settings.condense();
  dev_settings.strip_metadata();
  dev_settings.to_xml(doc);

  if (!doc.save_file(profile_path_.toStdString().c_str()))
    PL_ERR << "<FormStart> Failed to save device settings"; //should be in engine class?

}

