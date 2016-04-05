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
#include "marker.h"
#include "project.h"

namespace Ui {
class FormMultiGates;
}


class TableGates : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::vector<Qpx::Gate> gates_;

public:
  void set_data(std::vector<Qpx::Gate> gates);

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

  void setSpectrum(Qpx::Project *newset, QString name);

  void remake_gate(bool); //private?
  void change_width(int width);
  void make_range(Marker);

  void update_current_gate(Qpx::Gate);
  Qpx::Gate current_gate();
  std::list<MarkerBox2D> boxes() {return all_boxes_;}
  std::list<MarkerBox2D> current_peaks() {return current_peaks_;}

  void choose_peaks(std::list<MarkerBox2D>);

  double width_factor();

  void clear();

  void loadSettings();
  void saveSettings();


signals:
  void gate_selected();
  void boxes_made();
  void range_changed(MarkerBox2D);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void selection_changed(QItemSelection,QItemSelection);
  void on_pushApprove_clicked();
  void on_pushRemove_clicked();
  void on_pushDistill_clicked();
  void on_doubleGateOn_editingFinished();
  void on_doubleOverlaps_editingFinished();
  void update_range(Range);
  void update_peaks(bool);

  void range_changed_in_plot(Range);

  void peaks_changed_in_plot();
  void peak_selection_changed(std::set<double> sel);

  void on_pushAddGatedSpectrum_clicked();

private:
  Ui::FormMultiGates *ui;
  QSettings &settings_;


  Qpx::Project *spectra_;
  QString current_spectrum_;
  Qpx::Metadata tempx;
  std::shared_ptr<Qpx::Sink>gate_x;
  Qpx::Fitter fit_data_;
  int res;
  MarkerBox2D range2d;
  double xmin_, xmax_, ymin_, ymax_, xc_, yc_;



  //double current_gate_;

  TableGates table_model_;
  QSortFilterProxyModel sortModel;

  std::vector<Qpx::Gate> gates_;

  //from parent
  QString data_directory_;
  QString settings_directory_;


  std::list<MarkerBox2D> all_boxes_;
  std::list<MarkerBox2D> current_peaks_;

  int32_t index_of(double, bool fuzzy);
  int32_t current_idx();

  void rebuild_table(bool contents_changed);
  void make_gated_spectra();


};

#endif // FORM_MULTI_GATES_H
