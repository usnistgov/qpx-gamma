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
 *      WidgetPlotCalib - 
 *
 ******************************************************************************/


#ifndef WIDGET_PLOT_CALIB_H
#define WIDGET_PLOT_CALIB_H

#include <QWidget>
#include "qsquarecustomplot.h"
#include "appearance.h"
#include <set>

namespace Ui {
class WidgetPlotCalib;
}

class WidgetPlotCalib : public QWidget
{
  Q_OBJECT
private:
  struct PointSet
  {
    AppearanceProfile appearance;
    QVector<double> x, y, x_sigma, y_sigma;
  };

public:
  explicit WidgetPlotCalib(QWidget *parent = 0);
  ~WidgetPlotCalib();
  void clear_data();
  void clearGraphs();
  void clearPoints();

  void redraw();

  void setLabels(QString x, QString y);
  void setFloatingText(QString);

  std::set<double> get_selected_pts();
  void set_selected_pts(std::set<double>);

  void addPoints(AppearanceProfile style,
                 const QVector<double>& x, const QVector<double>& y,
                 const QVector<double>& x_sigma, const QVector<double>& y_sigma);
  void addFit(const QVector<double>& x, const QVector<double>& y, AppearanceProfile style);

  void set_scale_type_x(QString);
  void set_scale_type_y(QString);
  QString scale_type_x();
  QString scale_type_y();

signals:
  void clickedLeft(double);
  void clickedRight(double);
  void selection_changed();

private slots:
  void update_selection();

  void clicked_item(QCPAbstractItem*);
  void optionsChanged(QAction*);

private:
  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);

  Ui::WidgetPlotCalib *ui;

  AppearanceProfile style_fit;
  QVector<double> x_fit, y_fit;
  QVector<PointSet> points_;

  std::set<double> selection_;

  QString floating_text_;
  QString scale_type_x_;
  QString scale_type_y_;

  QMenu       menuOptions;

  void build_menu();

};

#endif
