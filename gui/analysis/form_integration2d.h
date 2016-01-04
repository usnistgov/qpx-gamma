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
 *      FormIntegration2D -
 *
 ******************************************************************************/


#ifndef FORM_INTEGRATION2D_H
#define FORM_INTEGRATION2D_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QItemSelection>
#include <QWidget>
#include <QSettings>
#include "marker.h"
#include "spectra_set.h"
#include "gamma_fitter.h"

namespace Ui {
class FormIntegration2D;
}


class TablePeaks2D : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::vector<Peak2D> peaks_;

public:
  void set_data(std::vector<Peak2D> peaks);

  explicit TablePeaks2D(QObject *parent = 0);
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


class FormIntegration2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormIntegration2D(QSettings &settings, QWidget *parent = 0);
  ~FormIntegration2D();

  void setSpectrum(Qpx::SpectraSet *newset, QString name);
  void setPeaks(std::list<MarkerBox2D> pks);

  std::list<MarkerBox2D> peaks();

  void update_current_peak(MarkerBox2D);
//  MarkerBox2D current_peak();

  void make_range(Marker x, Marker y);

  void choose_peaks(std::list<MarkerBox2D>);

  double width_factor();

  void clear();

  void loadSettings();
  void saveSettings();


signals:
  void peak_selected();
  void boxes_made();
  void range_changed(MarkerBox2D);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void selection_changed(QItemSelection,QItemSelection);
  void on_pushRemove_clicked();
  void on_doubleGateOn_editingFinished();
  void update_range(Range);
  void update_peaks(bool);

  void on_pushAddPeak2d_clicked();

  void on_pushShowDiagonal_clicked();

  void gated_fits_updated();

  void on_pushCentroidX_clicked();

  void on_pushCentroidY_clicked();

  void on_pushCentroidXY_clicked();

private:
  Ui::FormIntegration2D *ui;
  QSettings &settings_;

  Qpx::SpectraSet *spectra_;
  QString current_spectrum_;
  Qpx::Spectrum::Metadata md_;

  //double current_gate_;

  TablePeaks2D table_model_;
  QSortFilterProxyModel sortModel;

  std::vector<Peak2D> peaks_;
  MarkerBox2D range_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  int32_t index_of(MarkerBox2D);
  int32_t index_of(double, double);
  int32_t current_idx();

  Gamma::Fitter fit_x_, fit_y_, fit_d_;

  void rebuild_table(bool contents_changed);
  void make_gates();
};

#endif // FORM_MULTI_GATES_H
