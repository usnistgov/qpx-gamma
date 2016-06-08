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

#ifndef FORM_SYSTEM_SETTINGS_H
#define FORM_SYSTEM_SETTINGS_H

#include <QWidget>
#include <QCloseEvent>
#include <QTableView>
#include <QTreeView>
#include <QMenu>
#include "detector.h"
#include "special_delegate.h"
#include "thread_runner.h"
#include "table_settings.h"
#include "tree_settings.h"

namespace Ui {
class FormSystemSettings;
}

class FormSystemSettings : public QWidget
{
  Q_OBJECT

public:
  explicit FormSystemSettings(ThreadRunner&, XMLableDB<Qpx::Detector>&, QWidget *parent = 0);
  Qpx::Setting get_tree() {return dev_settings_;}
  ~FormSystemSettings();

  void exit();

public slots:
  void refresh();
  void update(const Qpx::Setting &tree, const std::vector<Qpx::Detector> &channelsupdate, Qpx::SourceStatus);

signals:
  void toggleIO(bool);
  void statusText(QString);

protected:
  void closeEvent(QCloseEvent*);

public slots:
  void updateDetDB();
  
private slots:
  void begin_editing();
  void stop_editing(QWidget*,QAbstractItemDelegate::EndEditHint);
  void on_pushSettingsRefresh_clicked();

  void toggle_push(bool enable, Qpx::SourceStatus status);
  void post_boot();

  void push_settings();
  void push_from_table(int chan, Qpx::Setting setting);
  void chose_detector(int chan, std::string name);

  void ask_binary_tree(Qpx::Setting, QModelIndex index);
  void ask_execute_tree(Qpx::Setting, QModelIndex index);
  void ask_binary_table(Qpx::Setting, QModelIndex index);
  void ask_execute_table(Qpx::Setting, QModelIndex index);

  void on_checkShowRO_clicked();
  void on_bootButton_clicked();

  void apply_detector_presets();
  void open_detector_DB();

  void on_spinRefreshFrequency_valueChanged(int arg1);

  void on_pushChangeProfile_clicked();

  void choose_profiles();
  void profile_chosen();


  void refresh_oscil();

private:
  Ui::FormSystemSettings *ui;

  Qpx::SourceStatus current_status_;

  XMLableDB<Qpx::Detector>            &detectors_;

  ThreadRunner        &runner_thread_;
  bool editing_;

  Qpx::Setting               dev_settings_;
  std::vector<Qpx::Detector> channels_;

  QTableView*         viewTableSettings;
  TableChanSettings   table_settings_model_;
  QpxSpecialDelegate  table_settings_delegate_;

  QTreeView*          viewTreeSettings;
  TreeSettings        tree_settings_model_;
  QpxSpecialDelegate  tree_delegate_;

  QMenu detectorOptions;

  void loadSettings();
  void saveSettings();
  void chan_settings_to_det_DB();

  bool exiting;
};

#endif // FORM_SYSTEM_SETTINGS_H
