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
 *      TableListData - table to display acquired list mode data
 *
 ******************************************************************************/

#ifndef FORM_RAW_VIEW_H
#define FORM_RAW_VIEW_H

#include <QWidget>
#include "spill.h"
#include "engine.h"
#include "special_delegate.h"
#include "widget_detectors.h"
#include "tree_settings.h"

#include <QItemSelectionModel>
#include <QAbstractTableModel>


namespace Ui {
class FormRawView;
}

class FormRawView : public QWidget
{
  Q_OBJECT

public:
  explicit FormRawView(QWidget *parent = 0);
  ~FormRawView();

signals:
  void toggleIO(bool);
  void statusText(QString);

private slots:
  void spillSelectionChanged(int);
  void hit_selection_changed(QItemSelection,QItemSelection);
  void stats_selection_changed(QItemSelection,QItemSelection);
  void toggle_push(bool online, Qpx::SourceStatus);

  void on_pushLoadExperiment_clicked();

protected:
  void closeEvent(QCloseEvent*);

private:
  Ui::FormRawView     *ui;
  QString data_directory_;    //data directory


  std::vector<Qpx::Spill> spills_;
  std::vector<size_t>     hit_counts_;
  std::vector<uint64_t>   bin_offsets_;
  std::ifstream  file_bin_;
  std::streampos bin_begin_, bin_end_;


  std::vector<Qpx::Hit>      hits_;
  std::vector<Qpx::Detector> dets_;
  std::map<int16_t, Qpx::HitModel> hitmodels_;
  std::map<int16_t, Qpx::StatsUpdate> stats_;
  XMLableDB<Qpx::Detector> spill_detectors_;

  TableDetectors det_table_model_;
  TreeSettings               attr_model_;
  QpxSpecialDelegate         attr_delegate_;

  void displayHit(int idx);
  void displayStats(int idx);

  void loadSettings();
  void saveSettings();
};

#endif // FORM_LIST_DAQ_H
