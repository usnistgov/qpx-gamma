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
 *      DialogProfile - dialog for editing the definition of detector
 *      TableProfiles - table model for detector set
 *      WidgetProfiles - dialog for managing detector set
 *
 ******************************************************************************/

#ifndef WIDGET_PROFILES_H_
#define WIDGET_PROFILES_H_

#include <QDir>
#include <QDialog>
#include <QSettings>

#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "form_daq_settings.h"
#include "qt_util.h"

namespace Ui {
class WidgetProfiles;
}

class TableProfiles : public QAbstractTableModel
{
  Q_OBJECT
private:
  std::vector<Gamma::Setting> &profiles_;
public:
  TableProfiles(std::vector<Gamma::Setting>& pf, QObject *parent = 0): QAbstractTableModel(parent), profiles_(pf) {}
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};


class WidgetProfiles : public QDialog
{
  Q_OBJECT

public:
  explicit WidgetProfiles(QSettings &settings, QWidget *parent = 0);
  ~WidgetProfiles();

signals:
  void profileChosen(QString, bool);

private:
  Ui::WidgetProfiles *ui;

  QSettings &settings_;

  std::vector<Gamma::Setting> profiles_;

  TableProfiles table_model_;
  QItemSelectionModel selection_model_;

  QString root_dir_;

  void update_profiles();

private slots:
  void on_pushNew_clicked();
  void on_pushEdit_clicked();
  void on_pushDelete_clicked();

  void addNewDet(Gamma::Detector);
  void selection_changed(QItemSelection,QItemSelection);
  void selection_double_clicked(QModelIndex);
  void toggle_push();
  void on_pushApply_clicked();
  void on_pushApplyBoot_clicked();
  void on_OutputDirFind_clicked();
};

#endif // WidgetProfiles_H
