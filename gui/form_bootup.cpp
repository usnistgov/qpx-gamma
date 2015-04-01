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
#include "ui_dialog_boot_files.h"
#include <QFileDialog>
#include "qt_util.h"

FormBootup::FormBootup(ThreadRunner& thread, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  settings_(settings),
  runner_thread_(thread),
  pixie_(Pixie::Wrapper::getInstance()),
  ui(new Ui::FormBootup)
{
  ui->setupUi(this);
  loadSettings();
  connect(&runner_thread_, SIGNAL(bootComplete(bool, bool)), this, SLOT(boot_complete(bool, bool)));
}

FormBootup::~FormBootup()
{
  delete ui;
}

void FormBootup::closeEvent(QCloseEvent *event) {
  saveSettings();

  event->accept();
}

void FormBootup::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool dead = (live == Pixie::LiveStatus::dead);

  ui->pushBootFiles->setEnabled(enable && dead);
  ui->cwCheck->setEnabled(enable && dead);
  ui->moduleSlotBox->setEnabled(enable && dead);

  ui->bootButton->setEnabled(enable);
}

void FormBootup::on_bootButton_clicked() {
  if (pixie_.settings().live() == Pixie::LiveStatus::dead) {
    emit toggleIO(false);
    emit statusText("Booting...");
    PL_INFO << "Booting pixie...";
    int myslot = ui->moduleSlotBox->value();
    std::vector<uint8_t> myslots = {static_cast<uint8_t>(myslot)}; //one and only module for now;

    runner_thread_.do_boot(ui->cwCheck->isChecked(),
                           boot_files_, myslots);
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

void FormBootup::on_pushBootFiles_clicked()
{
  DialogBootFiles* newBoot = new DialogBootFiles(boot_files_, this);
  connect(newBoot, SIGNAL(bootFilesReady(std::vector<std::string>)), this, SLOT(updateBootFiles(std::vector<std::string>)));
  newBoot->exec();
}

void FormBootup::updateBootFiles(std::vector<std::string> newfiles) {
  boot_files_ = newfiles;
}

void FormBootup::saveSettings() {
  settings_.beginGroup("Pixie");
  settings_.setValue("slot", ui->moduleSlotBox->value());
  settings_.setValue("auto_cw", ui->cwCheck->isChecked());
  settings_.setValue("boot_file_0", QString::fromStdString(boot_files_[0]));
  settings_.setValue("boot_file_1", QString::fromStdString(boot_files_[1]));
  settings_.setValue("boot_file_2", QString::fromStdString(boot_files_[2]));
  settings_.setValue("boot_file_3", QString::fromStdString(boot_files_[3]));
  settings_.setValue("boot_file_4", QString::fromStdString(boot_files_[4]));
  settings_.setValue("boot_file_5", QString::fromStdString(boot_files_[5]));
  settings_.setValue("boot_file_6", QString::fromStdString(boot_files_[6]));
  settings_.endGroup();
}

void FormBootup::loadSettings() {
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();

  settings_.beginGroup("Pixie");
  ui->moduleSlotBox->setValue(settings_.value("slot", 0).toInt());
  ui->cwCheck->setChecked(settings_.value("auto_cw", 1).toBool());
  boot_files_.resize(7);
  boot_files_[0] = settings_.value("boot_file_0", data_directory_ + "/XIA/Firmware/FippiP500.bin").toString().toStdString();
  boot_files_[1] = settings_.value("boot_file_1", data_directory_ + "/XIA/Firmware/syspixie_revC.bin").toString().toStdString();
  boot_files_[2] = settings_.value("boot_file_2", data_directory_ + "/XIA/Firmware/pixie.bin").toString().toStdString();
  boot_files_[3] = settings_.value("boot_file_3", data_directory_ + "/XIA/DSP/PXIcode.bin").toString().toStdString();
  boot_files_[4] = settings_.value("boot_file_4", data_directory_ + "/XIA/Configuration/HPGe.set").toString().toStdString();
  boot_files_[5] = settings_.value("boot_file_5", data_directory_ + "/XIA/DSP/PXIcode.var").toString().toStdString();
  boot_files_[6] = settings_.value("boot_file_6", data_directory_ + "/XIA/DSP/PXIcode.lst").toString().toStdString();
  settings_.endGroup();
}


DialogBootFiles::DialogBootFiles(const std::vector<std::string> &files, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogBootFiles)
{
    ui->setupUi(this);
    my_files_ = files;

    ui->FippiBinFile->setText(QString::fromStdString(my_files_[0]));
    ui->SyspixieBinFile->setText(QString::fromStdString(my_files_[1]));
    ui->PixieBinFile->setText(QString::fromStdString(my_files_[2]));
    ui->PxiBinFile->setText(QString::fromStdString(my_files_[3]));
    ui->SettingsFile->setText(QString::fromStdString(my_files_[4]));
    ui->PxiVarFile->setText(QString::fromStdString(my_files_[5]));
    ui->PxiLstFile->setText(QString::fromStdString(my_files_[6]));
}

DialogBootFiles::~DialogBootFiles()
{
    delete ui;
}

void DialogBootFiles::on_FippiFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->FippiBinFile->text()).dir().absolutePath(),
                                                    "Fippi binary (FippiP500.bin)");
    if (validateFile(this, fileName, false))
        ui->FippiBinFile->setText(fileName);
}

void DialogBootFiles::on_SyspixieFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->SyspixieBinFile->text()).dir().absolutePath(),
                                                    "Syspixie binary (syspixie*.bin)");
    if (validateFile(this, fileName, false))
        ui->SyspixieBinFile->setText(fileName);
}

void DialogBootFiles::on_PixieBinFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->PixieBinFile->text()).dir().absolutePath(),
                                                    "Pixie binary (pixie.bin)");
    if (validateFile(this, fileName, false))
        ui->PixieBinFile->setText(fileName);
}

void DialogBootFiles::on_PxiBinFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->PxiBinFile->text()).dir().absolutePath(),
                                                    "PXI code binary (PXIcode.bin)");
    if (validateFile(this, fileName, false))
        ui->PxiBinFile->setText(fileName);
}

void DialogBootFiles::on_PxiSettingsFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->SettingsFile->text()).dir().absolutePath(),
                                                    "Pixie settings file (*.set)");
    if (validateFile(this, fileName, false))
        ui->SettingsFile->setText(fileName);
}

void DialogBootFiles::on_PxiVarFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->PxiVarFile->text()).dir().absolutePath(),
                                                    "Pxi variable file (PXIcode.var)");
    if (validateFile(this, fileName, false))
        ui->PxiVarFile->setText(fileName);
}

void DialogBootFiles::on_PxiLstFind_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    QFileInfo(ui->PxiLstFile->text()).dir().absolutePath(),
                                                    "Pxi list file (PXIcode.lst)");
    if (validateFile(this, fileName, false))
        ui->PxiLstFile->setText(fileName);
}

void DialogBootFiles::on_buttonBox_accepted()
{
    my_files_[0] = ui->FippiBinFile->text().toStdString();
    my_files_[1] = ui->SyspixieBinFile->text().toStdString();
    my_files_[2] = ui->PixieBinFile->text().toStdString();
    my_files_[3] = ui->PxiBinFile->text().toStdString();
    my_files_[4] = ui->SettingsFile->text().toStdString();
    my_files_[5] = ui->PxiVarFile->text().toStdString();
    my_files_[6] = ui->PxiLstFile->text().toStdString();

    emit bootFilesReady(my_files_);
}
