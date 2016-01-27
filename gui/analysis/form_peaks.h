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
 *      FormPeaks - 
 *
 ******************************************************************************/


#ifndef FORM_PEAKS_H
#define FORM_PEAKS_H

#include <QWidget>
#include "spectrum1D.h"
#include "special_delegate.h"
#include "isotope.h"
#include "widget_plot1d.h"
#include <QItemSelection>
#include "spectra_set.h"
#include "gamma_fitter.h"

namespace Ui {
class FormPeaks;
}

enum ShowFitElements {
  none      = 0,
  kon       = 1 << 0,
  resid     = 1 << 1,
  prelim    = 1 << 2,
  filtered  = 1 << 3,
  gaussians = 1 << 4,
  baselines = 1 << 5,
  hypermet  = 1 << 6,
  multiplet = 1 << 7,
  edges     = 1 << 8
};

inline ShowFitElements operator|(ShowFitElements a, ShowFitElements b) {return static_cast<ShowFitElements>(static_cast<int>(a) | static_cast<int>(b));}
inline ShowFitElements operator&(ShowFitElements a, ShowFitElements b) {return static_cast<ShowFitElements>(static_cast<int>(a) & static_cast<int>(b));}


class FormPeaks : public QWidget
{
  Q_OBJECT

public:
  explicit FormPeaks(QWidget *parent = 0);
  ~FormPeaks();
  void setFit(Gamma::Fitter *);

  void new_spectrum(QString title = QString());
  void update_fit(bool content_changed = false);

  void perform_fit();

  void tighten();

  void clear();
  void update_spectrum();
  void make_range(Marker);

  void set_visible_elements(ShowFitElements elems, bool interactive = false);

  void loadSettings(QSettings &settings_);
  void saveSettings(QSettings &settings_);

signals:
  void peaks_changed(bool content_changed);
  void range_changed(Range range);

private slots:

  void user_selected_peaks();

  void addMovingMarker(Coord);
  void addMovingMarker(double);
  void removeMovingMarker(double);

  void replot_all();
  void replot_markers();
  void toggle_push();

  void range_moved(double, double);

  void on_pushAdd_clicked();
  void on_pushMarkerRemove_clicked();
  void on_pushFindPeaks_clicked();

  void on_spinSqWidth_editingFinished();
  void on_doubleOverlapWidth_editingFinished();
  void on_spinSum4EdgeW_editingFinished();


  void on_pushKON_clicked();
  void on_pushPrelim_clicked();
  void on_pushCtrs_clicked();
  void on_pushBkg_clicked();
  void on_pushGauss_clicked();
  void on_pushHypermet_clicked();
  void on_pushMulti_clicked();
  void on_pushEdges_clicked();

  void on_pushResid_clicked();

  void on_doubleThresh_editingFinished();

private:
  Ui::FormPeaks *ui;

  //from parent
  std::map<double, double> minima, maxima;

  //data from selected spectrum
  Gamma::Fitter *fit_data_;

  //markers
  Marker list, selected;
  Range range_;

  AppearanceProfile main_graph_, prelim_peak_, filtered_peak_,
                    gaussian_, hypermet_, baseline_, sum4edge_,
                    multiplet_g_, multiplet_h_, flagged_,
                    rise_, fall_, even_;


};

#endif // FORM_CALIBRATION_H
