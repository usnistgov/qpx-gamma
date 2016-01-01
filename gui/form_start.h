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
 *      Main tab for booting and settings
 *
 ******************************************************************************/

#ifndef FORM_START_H
#define FORM_START_H

#include <QWidget>
#include <QSettings>
#include "custom_logger.h"
#include "thread_runner.h"
#include "form_system_settings.h"
#include "form_oscilloscope.h"


namespace Ui {
class FormStart;
}

class FormStart : public QWidget
{
  Q_OBJECT

public:
  explicit FormStart(ThreadRunner &thread, QSettings &settings, XMLableDB<Gamma::Detector> &detectors, QString profile, QWidget *parent = 0);
  void exit();
  ~FormStart();

signals:
  void toggleIO(bool);
  void statusText(QString);
  void toggle_push_(bool, Qpx::DeviceStatus);
  void refresh();
  void update_dets();
  void optimization_requested();
  void gain_matching_requested();


private slots:
  void update(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus);
  void toggle_push(bool, Qpx::DeviceStatus);
  void toggleIO_(bool);
  void updateStatusText(QString);
  void boot_complete(bool, Qpx::DeviceStatus);
  void settings_updated();
  void detectors_updated();

  void request_gain_matching();
  void request_optimization();

protected:
  void closeEvent(QCloseEvent*);

private:
  //Ui::FormStart *ui;
  QSettings &settings_;

  QString data_directory_;
  QString profile_path_;

  XMLableDB<Gamma::Detector>  &detectors_;

  ThreadRunner        &runner_thread_;

  FormSystemSettings*  formSettings;
  FormOscilloscope*   formOscilloscope;

  bool exiting;

  //helper functions
  void saveSettings();
  void loadSettings();
};

#endif // FORM_START_H
