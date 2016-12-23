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
 *      QpFitter -
 *
 ******************************************************************************/


#ifndef PLOT_FITTER_H
#define PLOT_FITTER_H

#include "qp_multi1d.h"
#include "coord.h"
#include "fitter.h"

struct RangeSelector
{
  bool visible {false};
  QVariantMap properties;
  QStringList latch_to;
  QPlot::Appearance appearance;
  Coord l, r, c;

  void setProperty(const char *name, const QVariant &value)
  {
    properties[QString(name)] = value;
  }

  QVariant property(const char *name) const
  {
    return properties.value(QString(name));
  }
};

class QpFitter : public QPlot::Multi1D
{
  Q_OBJECT

public:
  explicit QpFitter(QWidget *parent = 0);
  ~QpFitter();

  void clear();

  void setFit(Qpx::Fitter *fit);
  void update_spectrum();

  void set_busy(bool b)
  {
    bool changed = (busy_ != b);
    busy_ = b;
    if (changed)
      adjustY();
  }

  void clearSelection();
  std::set<double> get_selected_peaks();

  void make_range_selection(Coord);
  void set_range_selection(RangeSelector);
  RangeSelector getRangeSelection() const;

public slots:
  void set_selected_peaks(std::set<double> selected_peaks);
  void updateData();

signals:

  void add_peak();
  void delete_selected_peaks();
  void adjust_sum4();
  void adjust_background();
  void peak_info(double);

  void rollback_ROI(double);
  void roi_settings(double);
  void refit_ROI(double);
  void delete_ROI(double);

  void peak_selection_changed(std::set<double> selected_peaks);
  void range_selection_changed(RangeSelector);

protected slots:

  void selection_changed();
  void changeROI(QAction*);

  void createRangeSelection(Coord);
  void adjustY()  Q_DECL_OVERRIDE;


protected:
  void executeButton(QPlot::Button *button) override;
  void mouseClicked(double x, double y, QMouseEvent *event) override;

  Qpx::Fitter *fit_data_;
  bool busy_ {false};

  QMenu menuROI;

  std::set<double> selected_peaks_;
  double selected_roi_ {-1};
  bool hold_selection_ {false};

  RangeSelector range_;

  void trim_log_lower(std::vector<double> &array);
  void calc_visible();

  QCPGraph *addGraph(const std::vector<double>& x, const std::vector<double>& y,
                     QPen appearance, bool smooth,
                     QString name = QString(), double region = -1, double peak = -1);

  void plotBackgroundEdge(Qpx::SUM4Edge edge,
                          const std::vector<double> &x,
                          double region,
                          QString button_name);

  void follow_selection();
  void plotExtraButtons();
  void plotEnergyLabel(double peak_id, double peak_energy, QCPItemTracer *crs);
  void plotRange();

  void plotRegion(double region_id, const Qpx::ROI &region, QCPGraph *data_graph);
  void plotPeak(double region_id, double peak_id, const Qpx::Peak &peak);

  void make_SUM4_range(double region, double peak);
  void make_background_range(double region, bool left);

};


#endif
