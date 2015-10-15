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
 *      FormMultiGates -
 *
 ******************************************************************************/


#ifndef FORM_MULTI_GATES_H
#define FORM_MULTI_GATES_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QItemSelection>
#include <QWidget>
#include <QSettings>
#include "gates.h"

namespace Ui {
class FormMultiGates;
}


class TableGates : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::vector<Gamma::Gate> gates_;

public:
  void set_data(std::vector<Gamma::Gate> gates);

  explicit TableGates(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  //bool setData(const QModelIndex & index, const QVariant & value, int role);

  void update();

//signals:
//   void energiesChanged();

public slots:

};


class FormMultiGates : public QWidget
{
  Q_OBJECT

public:
  explicit FormMultiGates(QSettings &settings, QWidget *parent = 0);
  ~FormMultiGates();

  void update_current_gate(Gamma::Gate);
  Gamma::Gate current_gate();

  void clear();

signals:
  void gate_selected();

private slots:
  void selection_changed(QItemSelection,QItemSelection);
  void on_pushApprove_clicked();

  void on_pushRemove_clicked();

private:
  Ui::FormMultiGates *ui;
  QSettings &settings_;

  //double current_gate_;

  TableGates table_model_;
  QSortFilterProxyModel sortModel;

  std::vector<Gamma::Gate> gates_;

  //from parent
  QString data_directory_;

  void loadSettings();
  void saveSettings();

  int32_t index_of(double, bool fuzzy);
  uint32_t current_idx();

  void rebuild_table(bool contents_changed);


};

#endif // FORM_MULTI_GATES_H
