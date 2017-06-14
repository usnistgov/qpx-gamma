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
 *      FormExperiment1D -
 *
 ******************************************************************************/

#pragma once

#include <QWidget>
#include "experiment.h"
#include "QPlotAppearance.h"
#include "experiment_filter.h"

namespace Ui {
class FormExperiment1D;
}

class FormExperiment1D : public QWidget
{
  Q_OBJECT

public:
  explicit FormExperiment1D(Qpx::ExperimentProject&,
                            QString &data_directory,
                            int64_t &selected_sink,
                            QWidget *parent = 0);
  ~FormExperiment1D();

  void update_exp_project();

private slots:
  void pass_selected_in_table();
  void pass_selected_in_plot();

  void on_comboDomain_currentIndexChanged(int index);
  void on_comboCodomain_currentIndexChanged(int index);
  void filter_changed();

  void on_pushSaveCsv_clicked();

private:
  Ui::FormExperiment1D *ui;

  Qpx::ExperimentProject &exp_project_;
  QString                &data_directory_;
  int64_t                &selected_sink_;

  QPlot::Appearance style_pts;

  WidgetExperimentFilter *domain_filter_;

  std::vector<Qpx::DataPoint> all_data_points_;
  std::vector<Qpx::DataPoint> filtered_data_points_;

  void loadSettings();
  void saveSettings();

  void list_relevant_domains();
  void display_data();
  void eval_dependent(Qpx::DataPoint &data);
};
