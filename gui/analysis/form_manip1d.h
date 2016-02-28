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
 *
 ******************************************************************************/

#ifndef FORM_MANIP1D_H
#define FORM_MANIP1D_H

#include <QWidget>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "special_delegate.h"

#include <spectra_set.h>
#include "qsquarecustomplot.h"
#include "widget_plot_multi1d.h"

namespace Ui {
  class FormManip1D;
}

class TableSpectra1D : public QAbstractTableModel
{
  Q_OBJECT
private:

  std::vector<std::string> letters_;

  Qpx::SpectraSet *spectra_;
  std::vector<std::string> var_names_;
  std::vector<Qpx::Spectrum::Metadata> data_;

  std::string make_var_name(int i);

public:
  TableSpectra1D(Qpx::SpectraSet *spectra, QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};

class FormManip1D : public QDialog
{
  Q_OBJECT

public:
  explicit FormManip1D(Qpx::SpectraSet& new_set, QWidget *parent = 0);
  ~FormManip1D();

  void updateUI();
  void replot_markers();

  void update_plot();


signals:
  void finished(bool);

private slots:
  void addMovingMarker(double);
  void removeMovingMarker(double);
  void selection_changed(QItemSelection,QItemSelection);


  void on_pushEval_clicked();

private:

  void spectrumDetails(std::string id);

  Marker moving;
  Qpx::Calibration calib_;

  Ui::FormManip1D *ui;

  Qpx::SpectraSet *mySpectra;

  QpxSpecialDelegate     special_delegate_;
  TableSpectra1D         table_model_;
  QItemSelectionModel    selection_model_;


};

#endif // WIDGET_PLOT1D_H
