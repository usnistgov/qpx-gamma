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
 *      Module and channel settings UI
 *
 ******************************************************************************/

#ifndef FORM_PIXIE_SETTINGS_H
#define FORM_PIXIE_SETTINGS_H

#include <QWidget>
#include <QSettings>
#include <QCloseEvent>
#include "detector.h"
#include "special_delegate.h"
#include "thread_runner.h"
#include "table_chan_settings.h"

#include "wrapper.h" //eliminate this

namespace Ui {
class FormPixieSettings;
}

class FormPixieSettings : public QWidget
{
  Q_OBJECT

public:
  explicit FormPixieSettings(ThreadRunner&, XMLableDB<Pixie::Detector>&, QSettings&, QWidget *parent = 0);
  ~FormPixieSettings();
  //void setData(ThreadRunner&, XMLableDB<Pixie::Detector>&);

  void apply_settings();

public slots:
  void refresh();
  void update();

signals:
  void toggleIO(bool);
  void statusText(QString);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void on_comboFilterSamples_currentIndexChanged(int index);
  void on_boxCoincWait_editingFinished();
  void on_pushSettingsRefresh_clicked();
  void on_pushOptimizeAll_clicked();
  void on_buttonCompTau_clicked();
  void on_buttonCompBLC_clicked();

  void updateDetDB();
  void updateDetChoices();
  void toggle_push(bool enable, Pixie::LiveStatus live);

  void on_pushDetDB_clicked();

private:
  Ui::FormPixieSettings *ui;

  Pixie::Wrapper& pixie_; //eliminate this

  XMLableDB<Pixie::Detector>            &detectors_;
  std::vector<std::string> default_detectors_;

  ThreadRunner        &runner_thread_;
  QSettings &settings_;

  TableChanSettings   channel_settings_model_;
  QpxSpecialDelegate  settings_delegate_;

  QString data_directory_;

  void loadSettings();
  void saveSettings();

};

#endif // FORM_PIXIE_SETTINGS_H
