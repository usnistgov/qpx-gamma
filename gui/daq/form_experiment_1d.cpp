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

#include "form_experiment_1d.h"
#include "widget_detectors.h"
#include "ui_form_experiment_1d.h"
#include "qt_util.h"
#include "dialog_domain.h"
#include <QSettings>

FormExperiment1D::FormExperiment1D(XMLableDB<Qpx::Detector>& dets,
                                         Qpx::ExperimentProject &project, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormExperiment1D),
  detectors_(dets),
  exp_project_(project),
  current_spectrum_(-1)
{
  ui->setupUi(this);

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 10);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 10);

  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(pass_selected_in_table()));
  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(pass_selected_in_plot()));

  ui->comboCodomain->addItem("FWHM (selected peak)");
  ui->comboCodomain->addItem("Count rate (selected peak)");
  ui->comboCodomain->addItem("Count rate (spectrum)");
  ui->comboCodomain->addItem("% dead time");

  loadSettings();

  update_settings();
}

FormExperiment1D::~FormExperiment1D()
{
  delete ui;
}


void FormExperiment1D::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

}

void FormExperiment1D::saveSettings()
{
  QSettings settings_;
}

void FormExperiment1D::update_exp_project()
{
  all_data_points_ = exp_project_.results();
  list_relevant_domains();
  display_data();
}

void FormExperiment1D::toggle_push() {

}

void FormExperiment1D::update_settings() {

}

void FormExperiment1D::pass_selected_in_table()
{
  int selected_pass_ = -1;
  foreach (QModelIndex i, ui->tableResults->selectionModel()->selectedRows())
    selected_pass_ = i.row();
  //  FitSettings fitset = selected_fitter_.settings();
  if ((selected_pass_ >= 0) && (selected_pass_ < filtered_data_points_.size())) {
    //    selected_fitter_ = filtered_data_points_.at(selected_pass_).spectrum;
    std::set<double> sel;
    sel.insert(filtered_data_points_.at(selected_pass_).independent_variable.value());
    ui->PlotCalib->set_selected_pts(sel);
  }
  else
  {
    //    selected_fitter_ = Qpx::Fitter();
    ui->PlotCalib->set_selected_pts(std::set<double>());
  }
  //  selected_fitter_.apply_settings(fitset);
  ui->PlotCalib->redraw();

}

void FormExperiment1D::pass_selected_in_plot()
{
  //allow only one point to be selected!!!
  std::set<double> selection = ui->PlotCalib->get_selected_pts();
  if (selection.size() < 1)
  {
    ui->tableResults->clearSelection();
    return;
  }

  double sel = *selection.begin();

  for (int i=0; i < filtered_data_points_.size(); ++i)
  {
    if (filtered_data_points_.at(i).independent_variable.value() == sel) {
      ui->tableResults->selectRow(i);
      pass_selected_in_table();
      return;
    }
  }
}

void FormExperiment1D::display_data()
{
  ui->tableResults->blockSignals(true);

  if (ui->tableResults->rowCount() != filtered_data_points_.size())
    ui->tableResults->setRowCount(filtered_data_points_.size());

  ui->tableResults->setColumnCount(9);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(ui->comboDomain->currentText(), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Total cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("Live time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("Real time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("Dead time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(5, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(6, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(7, new QTableWidgetItem("Peak cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(8, new QTableWidgetItem("Peak error", QTableWidgetItem::Type));

  QVector<double> xx(filtered_data_points_.size());;
  QVector<double> yy(filtered_data_points_.size());;
  QVector<double> xx_sigma(filtered_data_points_.size(), 0);
  QVector<double> yy_sigma(filtered_data_points_.size(), 0);

  for (int i = 0; i < filtered_data_points_.size(); ++i)
  {
    Qpx::DataPoint &data = filtered_data_points_.at(i);
    eval_dependent(data);

    Qpx::Setting rt = data.spectrum_info.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting lt = data.spectrum_info.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);
    double real_ms = rt.value_duration.total_milliseconds();
    double live_ms = lt.value_duration.total_milliseconds();

    add_to_table(ui->tableResults, i, 0, data.independent_variable.to_string());

    double total_count = data.spectrum_info.total_count.convert_to<double>();
    if (live_ms > 0)
      total_count = total_count / live_ms * 1000.0;
    add_to_table(ui->tableResults, i, 1, std::to_string(total_count));

    add_to_table(ui->tableResults, i, 2, lt.val_to_pretty_string());
    add_to_table(ui->tableResults, i, 3, rt.val_to_pretty_string());

    UncertainDouble dt = UncertainDouble::from_double(real_ms, real_ms - live_ms, 2);
    add_to_table(ui->tableResults, i, 4, dt.error_percent());

    add_to_table(ui->tableResults, i, 5, data.selected_peak.energy().to_string());
    add_to_table(ui->tableResults, i, 6, data.selected_peak.fwhm().to_string());
    add_to_table(ui->tableResults, i, 7, data.selected_peak.cps_best().to_string());
    add_to_table(ui->tableResults, i, 8, data.selected_peak.cps_best().error_percent());

    xx[i] = data.independent_variable.value();
    xx_sigma[i] = data.independent_variable.uncertainty();
    yy[i] = data.dependent_variable.value();
    yy_sigma[i] = data.dependent_variable.uncertainty();
  }

  ui->tableResults->blockSignals(false);

  ui->PlotCalib->clearGraphs();
  if (!xx.isEmpty()) {
    ui->PlotCalib->addPoints(style_pts, xx, yy, xx_sigma, yy_sigma);

    std::set<double> chosen_peaks_chan;
    //if selected insert;

    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    //    ui->PlotCalib->setLabels(QString::fromStdString(current_setting_.metadata.name),
    //                             ui->comboCodomain->currentText());
  }
  //  ui->PlotCalib->redraw();

  //  if ((selected_pass_ >= 0) && (selected_pass_ < filtered_data_points_.size())) {
  //    std::set<double> sel;
  //    sel.insert(filtered_data_points_.at(selected_pass_).independent_variable);
  //    ui->PlotCalib->set_selected_pts(sel);
  //    ui->PlotCalib->redraw();
  //  }
  //  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    ui->PlotCalib->redraw();
  }


  ui->pushSaveCsv->setEnabled(filtered_data_points_.size());
}


void FormExperiment1D::list_relevant_domains()
{
  QString current = ui->comboDomain->currentText();
  std::set<std::string> available;
  for (auto &p : all_data_points_)
    for (auto &d : p.domains) {
      available.insert(d.first);
//      DBG << "domain available " << d.first;
    }
  ui->comboDomain->clear();
  for (auto &d : available)
    ui->comboDomain->addItem(QString::fromStdString(d));
  if (available.count(current.toStdString()))
    ui->comboDomain->setCurrentText(current);
  else
    on_comboDomain_currentIndexChanged(0);
}

void FormExperiment1D::eval_dependent(Qpx::DataPoint &data)
{
  QString codomain = ui->comboCodomain->currentText();
  data.dependent_variable = UncertainDouble();
  if (codomain == "Count rate (spectrum)")
  {
    double live_ms = data.spectrum_info.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration.total_milliseconds();
    double total = data.spectrum_info.total_count.convert_to<double>();
    if (live_ms > 0)
      total = total / live_ms * 1000.0;
    data.dependent_variable = UncertainDouble::from_double(total, sqrt(total));
  }
  else if (codomain == "% dead time")
  {
    Qpx::Setting real = data.spectrum_info.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting live = data.spectrum_info.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);

    double dead = real.value_duration.total_milliseconds() - live.value_duration.total_milliseconds();
    if (real.value_duration.total_milliseconds() > 0)
      dead = dead / real.value_duration.total_milliseconds() * 100.0;
    else
      dead = 0;

    data.dependent_variable = UncertainDouble::from_double(dead, std::numeric_limits<double>::quiet_NaN());
  }
  else if (codomain == "FWHM (selected peak)")
  {
    if (data.selected_peak != Qpx::Peak())
      data.dependent_variable = data.selected_peak.fwhm();
  }
  else if (codomain == "Count rate (selected peak)")
  {
    double live_ms = data.spectrum_info.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration.total_milliseconds();

    if (data.selected_peak != Qpx::Peak()) {
      //      DBG << "selpeak area " << data.selected_peak.area_best.to_string() <<
      //             "  cps " << data.selected_peak.cps_best.to_string();
      data.dependent_variable = data.selected_peak.cps_best();
    }
  }
}

void FormExperiment1D::on_comboDomain_currentIndexChanged(int index)
{
  QString current = ui->comboDomain->currentText();
  filtered_data_points_.clear();
  for (auto p : all_data_points_)
  {
    if (!p.domains.count(current.toStdString()))
      continue;
    p.independent_variable =
        UncertainDouble::from_double(p.domains.at(current.toStdString()).number(),
                                     std::numeric_limits<double>::quiet_NaN());
    filtered_data_points_.push_back(p);
  }
  display_data();
}

void FormExperiment1D::on_comboCodomain_currentIndexChanged(int index)
{
  display_data();
}

void FormExperiment1D::on_pushSaveCsv_clicked()
{
  QSettings settings;
  settings.beginGroup("Program");
  QString data_directory = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();

  QString fileName = CustomSaveFileDialog(this, "Save experiment data",
                                          data_directory, "Comma separated values (*.csv)");
  if (!validateFile(this, fileName, true))
    return;

  QFile f( fileName );
  if( f.open( QIODevice::WriteOnly | QIODevice::Truncate) )
  {
    QTextStream ts( &f );
    QStringList strList;
    for (int j=0; j< ui->tableResults->columnCount(); j++)
      strList << ui->tableResults->horizontalHeaderItem(j)->data(Qt::DisplayRole).toString();
    ts << strList.join(", ") + "\n";

    for (int i=0; i < ui->tableResults->rowCount(); i++)
    {
      strList.clear();

      for (int j=0; j< ui->tableResults->columnCount(); j++)
        strList << ui->tableResults->item(i, j)->data(Qt::DisplayRole).toString();

      ts << strList.join(", ") + "\n";
    }
    f.close();
  }
}
