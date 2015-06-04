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

#include "gui/form_start.h"
#include "form_bootup.h"
#include "form_oscilloscope.h"
#include <QBoxLayout>

FormStart::FormStart(ThreadRunner &thread, QSettings &settings, XMLableDB<Pixie::Detector> &detectors, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  pixie_(Pixie::Wrapper::getInstance()),
  exiting(false)
{
  connect(&runner_thread_, SIGNAL(bootComplete(bool, bool)), this, SLOT(boot_complete(bool, bool)));

  formBootup = new FormBootup(runner_thread_, settings_);
  connect(formBootup, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(formBootup, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(this, SIGNAL(toggle_push_(bool,Pixie::LiveStatus)), formBootup, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addWidget(formBootup);
  QSpacerItem *si = new QSpacerItem(200,20, QSizePolicy::Preferred, QSizePolicy::Preferred);
  hl->addSpacerItem(si);

  FormOscilloscope *formOscilloscope = new FormOscilloscope(runner_thread_);
  connect(formOscilloscope, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(this, SIGNAL(toggle_push_(bool,Pixie::LiveStatus)), formOscilloscope, SLOT(toggle_push(bool,Pixie::LiveStatus)));

  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addWidget(formOscilloscope);

  formPixieSettings = new FormPixieSettings(runner_thread_, detectors_, settings_);
  connect(formPixieSettings, SIGNAL(toggleIO(bool)), this, SLOT(toggleIO_(bool)));
  connect(formPixieSettings, SIGNAL(statusText(QString)), this, SLOT(updateStatusText(QString)));
  connect(this, SIGNAL(refresh()), formPixieSettings, SLOT(update()));
  connect(this, SIGNAL(toggle_push_(bool,Pixie::LiveStatus)), formPixieSettings, SLOT(toggle_push(bool,Pixie::LiveStatus)));
  connect(this, SIGNAL(update_dets()), formPixieSettings, SLOT(updateDetDB()));

  QHBoxLayout *lo = new QHBoxLayout();
  lo->addLayout(vl);
  lo->addWidget(formPixieSettings);
//  lo->addStretch();

  this->setLayout(lo);
}

FormStart::~FormStart()
{
//  delete ui;
}

void FormStart::closeEvent(QCloseEvent *event) {
  if (exiting) {
    formPixieSettings->close();
    formBootup->close();
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

void FormStart::toggle_push(bool enable, Pixie::LiveStatus live) {
  emit toggle_push_(enable, live);
}

void FormStart::boot_complete(bool success, bool online) {
  if (success) {
    this->setCursor(Qt::WaitCursor);
    QThread::sleep(1);
    formPixieSettings->apply_settings();
    if (online) {
      pixie_.control_adjust_offsets();  //oscil function, if called through runner would invoke do_oscil
      pixie_.settings().load_optimization(); //if called through runner would invoke do_oscil
      runner_thread_.do_oscil();
    }
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
