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
 *      DialogDetector - dialog for editing the definition of detector
 *      TableDetectors - table model for detector set
 *      WidgetDetectors - dialog for managing detector set
 *
 ******************************************************************************/

#pragma once

#include <QDir>
#include <QDialog>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "detector.h"
#include "special_delegate.h"
#include "table_settings.h"

namespace Ui {
class DialogDetector;
}

class TableCalibrations : public QAbstractTableModel
{
  Q_OBJECT
private:
  XMLableDB<Qpx::Calibration> myDB;
  bool gain_;
public:
  TableCalibrations(QObject *parent = 0): QAbstractTableModel(parent) {}
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update(const XMLableDB<Qpx::Calibration>& db, bool gain);
};

class DialogDetector : public QDialog
{
  Q_OBJECT
public:
  explicit DialogDetector(Qpx::Detector, bool, QWidget *parent = 0);
  ~DialogDetector();

private slots:
  void selection_changed(QItemSelection,QItemSelection);

  void on_lineName_editingFinished();
  void on_comboType_currentIndexChanged(const QString &);
  void on_buttonBox_accepted();
  void on_pushReadOpti_clicked();
  void on_pushRead1D_clicked();
  void on_pushRemove_clicked();
  void on_pushRemoveGain_clicked();
  void on_pushClearFWHM_clicked();
  void on_pushClearEfficiency_clicked();

signals:
  void newDetReady(Qpx::Detector);

private:
  void updateDisplay();

  Ui::DialogDetector *ui;
  Qpx::Detector my_detector_;
  QDir settings_directory_;
  QString mca_formats_;

  TableCalibrations table_nrgs_;
  QItemSelectionModel selection_nrgs_;

  TableCalibrations table_gains_;
  QItemSelectionModel selection_gains_;

  TableChanSettings   table_settings_model_;
  QpxSpecialDelegate  table_settings_delegate_;
};
