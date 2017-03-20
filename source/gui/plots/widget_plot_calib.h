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

#pragma once

#include <QWidget>
#include "qp_multi1d.h"
#include "qp_appearance.h"
#include <set>

class WidgetPlotCalib : public QPlot::Multi1D
{
  Q_OBJECT
private:
  struct PointSet
  {
    QPlot::Appearance appearance;
    QVector<double> x, y, x_sigma, y_sigma;
  };

public:
  explicit WidgetPlotCalib(QWidget *parent = 0);

  void clearPrimary() override;
  void replotExtras() override;
  void replotPrimary() override;

  std::set<double> get_selected_pts();
  void set_selected_pts(std::set<double>);

  void addPoints(QPlot::Appearance style,
                 const QVector<double>& x, const QVector<double>& y,
                 const QVector<double>& x_sigma, const QVector<double>& y_sigma);
  void setFit(const QVector<double>& x, const QVector<double>& y, QPlot::Appearance style);

  void set_log_x(bool);
  bool log_x() const;

private slots:
  void apply_scale_type_x();

protected:
  void executeButton(QPlot::Button *) override;

  PointSet fit_;
  QVector<PointSet> points_;

//  std::set<double> selection_;

//  QString scale_type_x_;
  bool scale_log_x_;

  void plotExtraButtons();
  void plotFit();
  void plotPoints();
};
