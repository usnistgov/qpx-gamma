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


FormStart::FormStart(ThreadRunner &thread,
                     XMLableDB<Qpx::Detector> &detectors,
                     QWidget *parent)
  : QWidget(parent)
  , runner_thread_(thread)
  , detectors_(detectors)
  , exiting(false)
{
  this->setWindowTitle("DAQ Settings");

  connect(&thread, SIGNAL(settingsUpdated(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::SourceStatus)),
          this, SLOT(update(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::SourceStatus)));

  formOscilloscope = new FormOscilloscope(runner_thread_);
  connect(formOscilloscope, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::SourceStatus)), formOscilloscope, SLOT(toggle_push(bool,Qpx::SourceStatus)));
  formOscilloscope->setVisible(false);

  formSettings = new FormSystemSettings(runner_thread_, detectors_);
  connect(formSettings, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(formSettings, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(formSettings, SIGNAL(gain_matching_requested()), this, SLOT(request_gain_matching()));
  connect(formSettings, SIGNAL(optimization_requested()), this, SLOT(request_optimization()));
  connect(formSettings, SIGNAL(list_view_requested()), this, SLOT(request_list_view()));
  connect(formSettings, SIGNAL(raw_view_requested()), this, SLOT(request_raw_view()));

  connect(this, SIGNAL(refresh()), formSettings, SLOT(update()));
  connect(this, SIGNAL(toggle_push_(bool,Qpx::SourceStatus)), formSettings, SLOT(toggle_push(bool,Qpx::SourceStatus)));
  connect(this, SIGNAL(update_dets()), formSettings, SLOT(updateDetDB()));

  QHBoxLayout *lo = new QHBoxLayout();
  lo->addWidget(formOscilloscope);
  lo->addWidget(formSettings);

  this->setLayout(lo);
}

FormStart::~FormStart()
{
}

void FormStart::update(Qpx::Setting tree, std::vector<Qpx::Detector> dets, Qpx::SourceStatus status) {
  bool oscil = (status & Qpx::SourceStatus::can_oscil);
  if (oscil) {
    formOscilloscope->setVisible(true);
    formOscilloscope->updateMenu(dets);
  } else
    formOscilloscope->setVisible(false);
}

void FormStart::closeEvent(QCloseEvent *event) {
  if (exiting) {
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

void FormStart::toggle_push(bool enable, Qpx::SourceStatus live) {
  emit toggle_push_(enable, live);
}

void FormStart::settings_updated() {
  emit refresh();
}

void FormStart::detectors_updated() {
  emit update_dets();
}

void FormStart::request_gain_matching() {
  emit gain_matching_requested();
}

void FormStart::request_optimization() {
  emit optimization_requested();
}

void FormStart::request_list_view() {
  emit list_view_requested();
}

void FormStart::request_raw_view() {
  emit raw_view_requested();
}
