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
 *      Isotope DB browser
 *
 ******************************************************************************/

#ifndef WIDGET_ISOTOPES_H
#define WIDGET_ISOTOPES_H

#include <QWidget>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "isotope.h"
#include "special_delegate.h"

namespace Ui {
class WidgetIsotopes;
}

class TableGammas : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::vector<RadTypes::Gamma> gammas_;

public:
  void set_gammas(const XMLableDB<RadTypes::Gamma> &);
  XMLableDB<RadTypes::Gamma> get_gammas();
  void clear();

  explicit TableGammas(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role);

  void update();

signals:
   void energiesChanged();

public slots:

};


class WidgetIsotopes : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetIsotopes(QWidget *parent = 0);
  void setDir(QString filedir);
  ~WidgetIsotopes();
  std::vector<double> current_gammas() const;
  QString current_isotope() const;
  void set_current_isotope(QString);

  void push_energies(std::vector<double>);

  bool save_close();

signals:
  void energiesSelected();


private slots:
  void isotopeChosen(QString);
  void on_pushSum_clicked();
  void on_pushRemove_clicked();
  void on_pushAddGamma_clicked();

  void on_pushRemoveIsotope_clicked();

  void on_pushAddIsotope_clicked();

  void selection_changed(QItemSelection, QItemSelection);
  void energies_changed();

private:

  TableGammas table_gammas_;
  QItemSelectionModel selection_model_;
  QpxSpecialDelegate  special_delegate_;

  Ui::WidgetIsotopes *ui;
  QString root_dir_;
  bool modified_;

  XMLableDB<RadTypes::Isotope> isotopes_;

  std::vector<double> current_gammas_;

};

#endif // WIDGET_ISOTOPES_H
