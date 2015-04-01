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

#ifndef FORM_PLOT2D_H
#define FORM_PLOT2D_H

#include <QWidget>
#include <spectra_set.h>
#include "qsquarecustomplot.h"
#include "qtcolorpicker.h"

namespace Ui {
class FormPlot2D;
}

class FormPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit FormPlot2D(QWidget *parent = 0);
  ~FormPlot2D();

  void setSpectra(Pixie::SpectraSet& new_set);

  void update_plot();
  void replot_markers();
  void reset() {
      colorMap->clearData();
  }

public slots:
  void set_marker(double n);

signals:
  void markers_set(double x, double y);

private slots:
  void on_sliderZoom2d_sliderReleased();
  void gradientChosen(QAction*);
  void scaleTypeChosen(QAction*);

  void on_comboChose2d_activated(const QString &arg1);

  void plot_2d_mouse_upon(int x, int y);
  void plot_2d_mouse_clicked(double x, double y, QMouseEvent* event);

  void on_pushResetScales_clicked();

  void on_pushColorScale_clicked();

  void on_pushDetails_clicked();
  void spectrumDetailsClosed(bool);

private:
  Ui::FormPlot2D *ui;

  Pixie::SpectraSet *mySpectra;

  std::map<std::string, QCPColorGradient> gradients_;
  std::string current_gradient_;

  std::map<std::string, QCPAxis::ScaleType> scale_types_;
  std::string current_scale_type_;


  //coincidence plot
  QCPColorMap *colorMap;
  std::vector<double> co_energies_x_;
  std::vector<double> co_energies_y_;
  double marker_x, marker_y, ext_marker;
  QMenu gradientMenu;
  QMenu scaleTypeMenu;

  //prev
  QString name_2d;
  double zoom_2d;

  void make_marker(double, QColor, int, Qt::Orientations);
};

#endif // WIDGET_PLOT2D_H
