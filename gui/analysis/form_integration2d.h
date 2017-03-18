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

#pragma once

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QItemSelection>
#include <QWidget>
#include "peak2d.h"
#include "project.h"
#include "fitter.h"
#include "transition.h"

namespace Ui {
class FormIntegration2D;
}


class TablePeaks2D : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::vector<Sum2D> peaks_;

public:
  void set_data(std::vector<Sum2D> peaks);

  explicit TablePeaks2D(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};


class FormIntegration2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormIntegration2D(QWidget *parent = 0);
  ~FormIntegration2D();

  void setSpectrum(Qpx::ProjectPtr newset, int64_t idx);
  void setPeaks(std::list<Bounds2D> pks);

  std::list<Bounds2D> peaks();

  void make_range(double x, double y);
  void choose_peaks(std::set<int64_t>);

  void clearSelection();
  void clear();

  void loadSettings();
  void saveSettings();

signals:
  void peak_selected();
//  void transitions(std::map<double, Qpx::Transition>);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void selection_changed(QItemSelection,QItemSelection);
  void gated_fits_updated(std::set<double>);

  void on_pushAdjust_clicked();
  void on_pushRemove_clicked();
  void on_doubleGateOn_editingFinished();
  void on_pushAddPeak2d_clicked();
  void on_pushShowDiagonal_clicked();
  void on_pushTransitions_clicked();

private:
  Ui::FormIntegration2D *ui;

  Qpx::ProjectPtr project_;
  int64_t current_spectrum_ {0};
  Qpx::ConsumerMetadata md_;

  //double current_gate_;

  TablePeaks2D table_model_;
  QSortFilterProxyModel sortModel;

  std::vector<Sum2D> peaks_;
  Bounds2D range_;

  double gate_width_ {0};

  Qpx::Fitter fit_x_, fit_y_;

  int32_t index_of(double, double);
  int32_t current_idx();

  void rebuild_table(bool contents_changed);
  void make_gates();

  double make_lower(Qpx::Calibration fw, double center);
  double make_upper(Qpx::Calibration fw, double center);

  void adjust(Sum2D& peak);

  void reindex_peaks();
};

