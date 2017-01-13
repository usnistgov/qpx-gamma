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

#ifndef WIDGET_DETECTORS_H_
#define WIDGET_DETECTORS_H_

#include <QDir>
#include <QDialog>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "detector.h"
#include "qt_util.h"
#include "special_delegate.h"
#include "table_settings.h"

namespace Ui {
class WidgetDetectors;
class DialogDetector;
}

class TableCalibrations : public QAbstractTableModel
{
  Q_OBJECT
private:
  XMLableDB<Qpx::Calibration> &myDB;
  bool gain_;
public:
  TableCalibrations(XMLableDB<Qpx::Calibration>& db, bool gain, QObject *parent = 0): myDB(db), QAbstractTableModel(parent), gain_(gain) {}
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};

class DialogDetector : public QDialog
{
  Q_OBJECT
public:
  explicit DialogDetector(Qpx::Detector, QDir, bool, QWidget *parent = 0);
  ~DialogDetector();

private slots:
  void on_lineName_editingFinished();
  void on_comboType_currentIndexChanged(const QString &);
  void on_buttonBox_accepted();
  void on_pushReadOpti_clicked();
  void on_pushRead1D_clicked();

  void on_pushRemove_clicked();
  void selection_changed(QItemSelection,QItemSelection);

  void on_pushRemoveGain_clicked();

  void on_pushClearFWHM_clicked();

  void on_pushClearEfficiency_clicked();

signals:
  void newDetReady(Qpx::Detector);

private:
  void updateDisplay();

  Ui::DialogDetector *ui;
  Qpx::Detector my_detector_;
  QDir root_dir_;
  QString mca_formats_;

  TableCalibrations table_nrgs_;
  QItemSelectionModel selection_nrgs_;

  TableCalibrations table_gains_;
  QItemSelectionModel selection_gains_;

  TableChanSettings   table_settings_model_;
  QpxSpecialDelegate  table_settings_delegate_;
};


class TableDetectors : public QAbstractTableModel
{
  Q_OBJECT
private:
  XMLableDB<Qpx::Detector> *myDB;
public:
  TableDetectors(QObject *parent = 0): QAbstractTableModel(parent) {}
  void setDB(XMLableDB<Qpx::Detector>& db) {myDB = &db;}
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};


class WidgetDetectors : public QDialog
{
  Q_OBJECT

public:
  explicit WidgetDetectors(QWidget *parent = 0);
  ~WidgetDetectors();

  void setData(XMLableDB<Qpx::Detector> &newdb, QString outdir);

signals:
  void detectorsUpdated();

private:
  Ui::WidgetDetectors *ui;

  XMLableDB<Qpx::Detector> *detectors_;
  TableDetectors table_model_;
  QItemSelectionModel selection_model_;

  QString root_dir_;

private slots:
  void on_pushNew_clicked();
  void on_pushEdit_clicked();
  void on_pushDelete_clicked();
  void on_pushImport_clicked();
  void on_pushExport_clicked();

  void addNewDet(Qpx::Detector);
  void on_pushSetDefault_clicked();
  void on_pushGetDefault_clicked();
  void selection_changed(QItemSelection,QItemSelection);
  void selection_double_clicked(QModelIndex);

  void toggle_push();
};

#endif // WidgetDetectors_H
