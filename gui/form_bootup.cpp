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
 *      Bootup interface
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include <QDir>
#include "gui/form_bootup.h"
#include "ui_form_bootup.h"
#include <QFileDialog>

FormBootup::FormBootup(ThreadRunner& thread, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  pixie_(Pixie::Wrapper::getInstance()),
  ui(new Ui::FormBootup)
{
  ui->setupUi(this);
  connect(&runner_thread_, SIGNAL(bootComplete(bool, bool)), this, SLOT(boot_complete(bool, bool)));
}

FormBootup::~FormBootup()
{
  delete ui;
}

void FormBootup::closeEvent(QCloseEvent *event) {
  event->accept();
}

void FormBootup::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool dead = (live == Pixie::LiveStatus::dead);

  ui->bootButton->setEnabled(enable);
}

void FormBootup::on_bootButton_clicked() {
  if (pixie_.settings().live() == Pixie::LiveStatus::dead) {
    emit toggleIO(false);
    emit statusText("Booting...");
    PL_INFO << "Booting pixie...";

    runner_thread_.do_boot();
  } else {
    PL_INFO << "Shutting down";
    pixie_.die();
    ui->bootButton->setText("Boot system");
    ui->bootButton->setIcon(QIcon(":/new/icons/Cowboy-Boot.png"));
    emit toggleIO(true);
  }
}

void FormBootup::boot_complete(bool success, bool online) {
  if (success) {
    ui->bootButton->setText("Reset system");
    ui->bootButton->setIcon(QIcon(":/new/icons/oxy/start.png"));
  }
}
