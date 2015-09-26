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

FormStart::FormStart(ThreadRunner &thread, QSettings &settings, XMLableDB<Gamma::Detector> &detectors, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  pixie_(Qpx::Wrapper::getInstance()),
  exiting(false)
{
  connect(&thread, SIGNAL(settingsUpdated(Gamma::Setting, std::vector<Gamma::Detector>)), this, SLOT(update(Gamma::Setting, std::vector<Gamma::Detector>)));

  formOscilloscope = new FormOscilloscope(runner_thread_, settings);
  connect(formOscilloscope, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::LiveStatus)), formOscilloscope, SLOT(toggle_push(bool,Qpx::LiveStatus)));

  formPixieSettings = new FormPixieSettings(runner_thread_, detectors_, settings_);
  connect(formPixieSettings, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(formPixieSettings, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  
  connect(this, SIGNAL(refresh()), formPixieSettings, SLOT(update()));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::LiveStatus)), formPixieSettings, SLOT(toggle_push(bool,Qpx::LiveStatus)));
  connect(this, SIGNAL(update_dets()), formPixieSettings, SLOT(updateDetDB()));

  QHBoxLayout *lo = new QHBoxLayout();
  lo->addWidget(formOscilloscope);
  lo->addWidget(formPixieSettings);

  this->setLayout(lo);

  loadSettings();
}

FormStart::~FormStart()
{
  //  delete ui;
}

void FormStart::update(Gamma::Setting tree, std::vector<Gamma::Detector> dets) {
  formOscilloscope->updateMenu(dets);
}

void FormStart::closeEvent(QCloseEvent *event) {
  if (exiting) {
    saveSettings();
    formPixieSettings->close();
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

void FormStart::toggle_push(bool enable, Qpx::LiveStatus live) {
  emit toggle_push_(enable, live);
}

void FormStart::boot_complete(bool success, Qpx::LiveStatus online) {
  //DEPRECATED

  if (success) {
    this->setCursor(Qt::WaitCursor);
    formPixieSettings->updateDetDB();
    if (online == Qpx::LiveStatus::online)
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

  QString filename = data_directory_ + "/qpx_settings.set";

  Gamma::Setting dev_settings;

  FILE* myfile;
  myfile = fopen (filename.toStdString().c_str(), "r");
  if (myfile != nullptr) {
    tinyxml2::XMLDocument docx;
    docx.LoadFile(myfile);
    tinyxml2::XMLElement* root = docx.FirstChildElement(Gamma::Setting().xml_element_name().c_str());
    if (root != nullptr) {
      dev_settings = Gamma::Setting(root);
    }
    fclose(myfile);
  }

  PL_DBG << "dev_settings = " << dev_settings.name << " with " << dev_settings.branches.size();

  if (dev_settings != Gamma::Setting()) {
    dev_settings.value_int = int(Qpx::LiveStatus::dead);
    runner_thread_.do_push_settings(dev_settings);
  }
}

void FormStart::saveSettings() {
  QString filename = data_directory_ + "/qpx_settings.set";

  Gamma::Setting dev_settings = formPixieSettings->get_tree();

  if (dev_settings != Gamma::Setting()) {
    FILE* myfile;
    myfile = fopen (filename.toStdString().c_str(), "w");
    if (myfile != nullptr) {
      tinyxml2::XMLPrinter printer(myfile);
      printer.PushHeader(true, true);
      dev_settings.to_xml(printer);
      fclose(myfile);
    }
  }
}

