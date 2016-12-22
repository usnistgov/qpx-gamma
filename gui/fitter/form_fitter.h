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
 *      FormFitter -
 *
 ******************************************************************************/


#ifndef FORM_FITTER_H
#define FORM_FITTER_H

#include <QWidget>
#include "qp_multi1d.h"
#include "special_delegate.h"
#include <QItemSelection>
#include <QMediaPlayer>

#include "project.h"
#include "thread_fitter.h"
#include "coord.h"

namespace Ui {
class FormFitter;
}

struct Range {
  bool visible;
  QVariantMap properties;
  QStringList latch_to;
  QPlot::Appearance appearance;
  Coord l, r, c;

  void setProperty(const char *name, const QVariant &value) {
    properties[QString(name)] = value;
  }

  QVariant property(const char *name) const {
    return properties.value(QString(name));
  }
};

class FormFitter : public QWidget
{
  Q_OBJECT

public:
  explicit FormFitter(QWidget *parent = 0);
  ~FormFitter();

  void clear();

  void setFit(Qpx::Fitter *fit);
  void update_spectrum();

  bool busy() { return busy_; }


  void clearSelection();
  std::set<double> get_selected_peaks();

  void make_range(Coord);
  void set_range(Range);

  void perform_fit();

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

public slots:
  void set_selected_peaks(std::set<double> selected_peaks);
  void updateData();

signals:

  void peak_selection_changed(std::set<double> selected_peaks);
  void range_changed(Range range);
  void data_changed();
  void fitting_done();
  void fitter_busy(bool);

private slots:
  void plot_mouse_clicked(double,double,Qt::MouseButton);
  void clicked_item(QCPAbstractItem *);
  void switch_scale_type();

  void selection_changed();
  void changeROI(QAction*);

  void fit_updated(Qpx::Fitter);
  void fitting_complete();

  void refit_ROI(double);
  void rollback_ROI(double);
  void delete_ROI(double);

  void createRange(Coord);

  void toggle_push();

  void add_peak();
  void adjust_sum4();
  void adjust_background();

  void delete_selected_peaks();

  void on_pushFindPeaks_clicked();
  void on_pushStopFitter_clicked();
  void on_pushSettings_clicked();

private:
  Ui::FormFitter *ui;

  //data from selected spectrum
  Qpx::Fitter *fit_data_;
  std::set<double> selected_peaks_;
  double selected_roi_;

  Range range_;

  bool busy_;

  ThreadFitter thread_fitter_;

  QCPItemTracer* edge_trc1;
  QCPItemTracer* edge_trc2;

  bool mouse_pressed_;
  bool hold_selection_;

  double minx_zoom, maxx_zoom;

  QMenu menuROI;

  QMediaPlayer *player;

  void trim_log_lower(QVector<double> &array);
  void calc_visible();
  QCPGraph *addGraph(const QVector<double>& x, const QVector<double>& y, QPen appearance,
                     bool smooth, QString name = QString(),
                     double region = -1, double peak = -1);

  void plotBackgroundEdge(Qpx::SUM4Edge edge,
                          const std::vector<double> &x,
                          double region,
                          QString button_name);

  void follow_selection();
  void plotButtons();
  void plotEnergyLabel(double peak_id, double peak_energy, QCPItemTracer *crs);
  void plotRange();

  void plotRegion(double region_id, const Qpx::ROI &region, QCPGraph *data_graph);
  void plotPeak(double region_id, double peak_id, const Qpx::Peak &peak);

  void peak_info(double peak);
  void roi_settings(double region);

  void make_SUM4_range(double region, double peak);
  void make_background_range(double region, bool left);

};

#endif // FORM_CALIBRATION_H
