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
#include "qsquarecustomplot.h"
#include "spectrum1D.h"
#include "special_delegate.h"
#include "isotope.h"
#include <QItemSelection>
#include <QRadioButton>
#include "spectra_set.h"
#include "gamma_fitter.h"
#include "thread_fitter.h"
#include "marker.h"

namespace Ui {
class FormFitter;
}


class RollbackDialog : public QDialog {
  Q_OBJECT

public:
  explicit RollbackDialog(Gamma::ROI setting, QWidget *parent = 0);
  int get_choice();

private:
  Gamma::ROI      roi_;
  std::vector<QRadioButton*> radios_;
};

class FormFitter : public QWidget
{
  Q_OBJECT

public:
  explicit FormFitter(QWidget *parent = 0);
  ~FormFitter();

  void clear();

  void setFit(Gamma::Fitter *fit);
  void update_spectrum(QString title = QString());
  void updateData();

  void reset_scales();


  void clearSelection();
  std::set<double> get_selected_peaks();

  void make_range(Marker);
  void set_range(Range);

  void perform_fit();

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

public slots:
  void set_selected_peaks(std::set<double> selected_peaks);
  void replace_peaks(std::vector<Gamma::Peak>);

signals:

  void peak_selection_changed(std::set<double> selected_peaks);
  void range_changed(Range range);
  void data_changed();

private slots:
  void plot_mouse_clicked(double x, double y, QMouseEvent *event, bool channels);
  void plot_mouse_press(QMouseEvent*);
  void plot_mouse_release(QMouseEvent*);
  void clicked_plottable(QCPAbstractPlottable *);
  void clicked_item(QCPAbstractItem *);

  void selection_changed();
  virtual void plot_rezoom();
  void exportRequested(QAction*);
  void optionsChanged(QAction*);
  void changeROI(QAction*);
  void zoom_out();



  void fit_updated(Gamma::Fitter);
  void fitting_complete();

  void refit_ROI(double);
  void rollback_ROI(double);
  void delete_ROI(double);

  void createRange(Coord);
  void createROI_bounds_range(double);

  void replot_all();
  void toggle_push();

  void add_peak();
  void adjust_roi_bounds();

  void on_pushRemovePeaks_clicked();
  void on_pushFindPeaks_clicked();
  void on_pushStopFitter_clicked();

  void on_spinSqWidth_editingFinished();
  void on_doubleOverlapWidth_editingFinished();
  void on_spinSum4EdgeW_editingFinished();
  void on_doubleThresh_editingFinished();


  void on_pushShowSUM4_clicked();

private:
  Ui::FormFitter *ui;

  //data from selected spectrum
  Gamma::Fitter *fit_data_;
  std::set<double> selected_peaks_;

  Range range_;

  bool busy_;

  ThreadFitter thread_fitter_;



  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);
  void calc_y_bounds(double lower, double upper);

  void build_menu();

  QCPItemTracer* edge_trc1;
  QCPItemTracer* edge_trc2;

  bool force_rezoom_;
  double minx, maxx;
  double miny, maxy;

  bool mouse_pressed_;

  double minx_zoom, maxx_zoom;

  std::map<double, double> minima_, maxima_;

  QMenu menuExportFormat;
  QMenu menuOptions;
  QMenu menuROI;

  QString scale_type_;
  QString title_text_;

  void trim_log_lower(QVector<double> &array);
  void calc_visible();
  void add_bounds(const QVector<double>& x, const QVector<double>& y);
  void addGraph(const QVector<double>& x, const QVector<double>& y, QPen appearance,
                int fittable = 0, int32_t bits = 0,
                QString name = QString());
  void addEdge(Gamma::SUM4Edge edge, QPen pen);

  void follow_selection();
  void plotButtons();
  void plotEnergyLabels();
  void plotRange();
  void plotTitle();

  void plotROI_options();
  void plotROI_range();

  void set_scale_type(QString);
  QString scale_type();
};

#endif // FORM_CALIBRATION_H
