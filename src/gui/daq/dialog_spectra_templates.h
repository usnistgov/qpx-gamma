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
 *      DialogSpectrumTemplate - edit dialog for spectrum template
 *      TableSpectraTemplates - table of spectrum templates
 *      DialogSpectraTemplates - dialog for managing spectrum templates
 *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "special_delegate.h"
#include "qt_util.h"
#include "tree_settings.h"
#include "special_delegate.h"
#include "consumer_metadata.h"

namespace Ui {
class DialogSpectraTemplates;
class DialogSpectrumTemplate;
}


class TableSpectraTemplates : public QAbstractTableModel
{
  Q_OBJECT
private:
  XMLableDB<Qpx::ConsumerMetadata> &templates_;

public:
  TableSpectraTemplates(XMLableDB<Qpx::ConsumerMetadata>& templates, QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};


class DialogSpectraTemplates : public QDialog
{
  Q_OBJECT

public:
  explicit DialogSpectraTemplates(XMLableDB<Qpx::ConsumerMetadata> &newdb,
                                  std::vector<Qpx::Detector> current_dets,
                                  QString savedir, QWidget *parent = 0);
  ~DialogSpectraTemplates();

private:
  Ui::DialogSpectraTemplates *ui;

  XMLableDB<Qpx::ConsumerMetadata> &templates_;

  QpxSpecialDelegate      special_delegate_;
  TableSpectraTemplates         table_model_;
  QItemSelectionModel selection_model_;

  QString root_dir_;
  std::vector<Qpx::Detector> current_dets_;

private slots:

  void on_pushImport_clicked();
  void on_pushExport_clicked();
  void on_pushNew_clicked();
  void on_pushEdit_clicked();
  void on_pushDelete_clicked();

  void on_pushSetDefault_clicked();
  void on_pushUseDefault_clicked();

  void selection_changed(QItemSelection,QItemSelection);
  void selection_double_clicked(QModelIndex);

  void toggle_push();
  void on_pushClear_clicked();
  void on_pushUp_clicked();
  void on_pushDown_clicked();
};
