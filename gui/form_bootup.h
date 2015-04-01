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
 *      Dialog for selecting firmware files for bootup of Pixie-4
 *
 ******************************************************************************/

#ifndef FORM_BOOTUP_H
#define FORM_BOOTUP_H

#include <QWidget>
#include <QSettings>
#include <QDialog>
#include <QCloseEvent>
#include "thread_runner.h"
#include "wrapper.h"

namespace Ui {
class FormBootup;
class DialogBootFiles;
}

class DialogBootFiles : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBootFiles(const std::vector<std::string> &files,  QWidget *parent = 0);
    ~DialogBootFiles();

signals:
    void bootFilesReady(std::vector<std::string>);

private slots:
    void on_FippiFind_clicked();
    void on_SyspixieFind_clicked();
    void on_PixieBinFind_clicked();
    void on_PxiBinFind_clicked();
    void on_PxiSettingsFind_clicked();
    void on_PxiVarFind_clicked();
    void on_PxiLstFind_clicked();

    void on_buttonBox_accepted();

private:
    Ui::DialogBootFiles *ui;

    std::vector<std::string> my_files_;

};

class FormBootup : public QWidget
{
  Q_OBJECT

public:
  explicit FormBootup(ThreadRunner&, QSettings&, QWidget *parent = 0);
  ~FormBootup();

  void loadSettings();
  void saveSettings();

signals:
  void toggleIO(bool);
  void statusText(QString);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void updateBootFiles(std::vector<std::string>);
  void on_bootButton_clicked();
  void on_pushBootFiles_clicked();
  void boot_complete(bool, bool);
  void toggle_push(bool enable, Pixie::LiveStatus);

private:
  Ui::FormBootup *ui;

  Pixie::Wrapper& pixie_; //eliminate this

  std::vector<std::string>    boot_files_;        //XIA stuff

  ThreadRunner        &runner_thread_;
  QString data_directory_;
  QSettings &settings_;
};

#endif // FORM_BOOTUP_H
