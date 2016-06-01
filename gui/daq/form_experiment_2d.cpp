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
 *      FormExperiment2D -
 *
 ******************************************************************************/

#include "form_experiment_2d.h"
#include "ui_form_experiment_2d.h"
#include "qt_util.h"
#include "dialog_domain.h"
#include <QSettings>


FormExperiment2D::FormExperiment2D(Qpx::ExperimentProject &project,
                                   QString &data_directory,
                                   int64_t &selected_sink,
                                   QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormExperiment2D),
  exp_project_(project),
  data_directory_(data_directory),
  selected_sink_(selected_sink),
  domain_filter_(nullptr)
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
  ui->comboCodomain->addItem("Centroid (selected peak)");
  ui->comboCodomain->adjustSize();

  ui->comboDomain->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  ui->comboDomain2->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  ui->PlotCalib->set_antialiased(false);
  ui->PlotCalib->set_scale_type("Linear");
  ui->PlotCalib->set_show_legend(true);

  loadSettings();
}

FormExperiment2D::~FormExperiment2D()
{
  delete ui;
}


void FormExperiment2D::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Experiment2d");
  ui->comboDomain->setCurrentText(settings_.value("domain", ui->comboDomain->currentText()).toString());
  ui->comboDomain2->setCurrentText(settings_.value("domain2", ui->comboDomain2->currentText()).toString());
  ui->comboCodomain->setCurrentText(settings_.value("co-domain", ui->comboCodomain->currentText()).toString());
  settings_.endGroup();
}

void FormExperiment2D::saveSettings()
{
  QSettings settings_;

  settings_.beginGroup("Experiment2d");
  settings_.setValue("domain", ui->comboDomain->currentText());
  settings_.setValue("domain2", ui->comboDomain2->currentText());
  settings_.setValue("co-domain", ui->comboCodomain->currentText());
  settings_.endGroup();
}

void FormExperiment2D::update_exp_project()
{
  all_data_points_ = exp_project_.results();
  list_relevant_domains();
  display_data();
}

void FormExperiment2D::pass_selected_in_table()
{
//  int selected_pass_ = -1;
//  foreach (QModelIndex i, ui->tableResults->selectionModel()->selectedRows())
//    selected_pass_ = i.row();
//  if ((selected_pass_ >= 0) && (selected_pass_ < filtered_data_points_.size())) {
//    std::set<double> sel;
//    sel.insert(filtered_data_points_.at(selected_pass_).independent_variable.value());
//    ui->PlotCalib->set_selected_pts(sel);
//  }
//  else
//    ui->PlotCalib->set_selected_pts(std::set<double>());
//  ui->PlotCalib->redraw();
}

void FormExperiment2D::pass_selected_in_plot()
{
  //allow only one point to be selected!!!
//  std::set<double> selection = ui->PlotCalib->get_selected_pts();
//  if (selection.size() < 1)
//  {
//    ui->tableResults->clearSelection();
//    return;
//  }

//  double sel = *selection.begin();

//  for (int i=0; i < filtered_data_points_.size(); ++i)
//  {
//    if (filtered_data_points_.at(i).independent_variable.value() == sel) {
//      ui->tableResults->selectRow(i);
//      pass_selected_in_table();
//      return;
//    }
//  }
}

void FormExperiment2D::display_data()
{
  ui->tableResults->blockSignals(true);

  if (ui->tableResults->rowCount() != filtered_data_points_.size())
    ui->tableResults->setRowCount(filtered_data_points_.size());

  QString name = ui->comboDomain->currentData().toString();
  QString name2 = ui->comboDomain2->currentData().toString();

  ui->tableResults->setColumnCount(11);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(name, QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem(name2, QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("Total cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("Live time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("Real time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(5, new QTableWidgetItem("Dead time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(6, new QTableWidgetItem("Center", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(7, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(8, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(9, new QTableWidgetItem("Peak cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(10, new QTableWidgetItem("Peak error", QTableWidgetItem::Type));

  std::set<double> xs;
  std::set<double> ys;

  for (int i = 0; i < filtered_data_points_.size(); ++i)
  {
    Qpx::DataPoint &data = filtered_data_points_.at(i);
    eval_dependent(data);

    Qpx::Setting rt = data.spectrum_info.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting lt = data.spectrum_info.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);
    double real_ms = rt.value_duration.total_milliseconds();
    double live_ms = lt.value_duration.total_milliseconds();

    add_to_table(ui->tableResults, i, 0, data.independent_variable.to_string());
    add_to_table(ui->tableResults, i, 1, data.independent_variable2.to_string());

    double total_count = data.spectrum_info.total_count.convert_to<double>();
    if (live_ms > 0)
      total_count = total_count / live_ms * 1000.0;
    add_to_table(ui->tableResults, i, 2, std::to_string(total_count));

    add_to_table(ui->tableResults, i, 3, lt.val_to_pretty_string());
    add_to_table(ui->tableResults, i, 4, rt.val_to_pretty_string());

    UncertainDouble dt = UncertainDouble::from_double(real_ms, real_ms - live_ms, 2);
    add_to_table(ui->tableResults, i, 5, dt.error_percent());

    add_to_table(ui->tableResults, i, 6, data.selected_peak.center().to_string());
    add_to_table(ui->tableResults, i, 7, data.selected_peak.energy().to_string());
    add_to_table(ui->tableResults, i, 8, data.selected_peak.fwhm().to_string());
    add_to_table(ui->tableResults, i, 9, data.selected_peak.cps_best().to_string());
    add_to_table(ui->tableResults, i, 10, data.selected_peak.cps_best().error_percent());

    xs.insert(data.independent_variable.value());
    ys.insert(data.independent_variable2.value());
  }
  ui->tableResults->blockSignals(false);

  std::map<double, uint16_t> xtoi;
  std::map<double, uint16_t> ytoi;

  Qpx::Calibration xcal, ycal;
  xcal.type_ = name.toStdString();
  xcal.units_.clear();
  xcal.model_ = Qpx::CalibrationModel::polynomial;
  xcal.bits_ = 1;
  xcal.coefficients_.clear();

  ycal = xcal;
  ycal.type_ = name2.toStdString();

  int i=0;
  for (auto &x : xs)
  {
    xtoi[x] = i;
    i++;
  }
  if (xtoi.size())
    xcal.coefficients_.push_back(xtoi.begin()->first);
  if (xtoi.size() > 1)
    xcal.coefficients_.push_back((xtoi.rbegin()->first - xtoi.begin()->first) / (xtoi.size()-1));


  i=0;
  for (auto &y : ys)
  {
    ytoi[y] = i;
    i++;
  }
  if (ytoi.size())
    ycal.coefficients_.push_back(ytoi.begin()->first);
  if (xtoi.size() > 1)
    ycal.coefficients_.push_back((ytoi.rbegin()->first - ytoi.begin()->first) / (ytoi.size()-1));


  bool flip90 = ytoi.size() > xtoi.size();

  std::shared_ptr<Qpx::EntryList> data = std::shared_ptr<Qpx::EntryList>(new Qpx::EntryList());
  for (int i = 0; i < filtered_data_points_.size(); ++i)
  {
    Qpx::DataPoint &dp = filtered_data_points_.at(i);
    uint16_t d1 = xtoi.at(dp.independent_variable.value());
    uint16_t d2 = ytoi.at(dp.independent_variable2.value());
    if (flip90)
    {
      uint16_t d = d1;
      d1 = d2;
      d2 = d;
    }

    Qpx::Entry e;
    e.first = std::vector<uint16_t>({d1,d2});
    e.second = dp.dependent_variable.value();
    data->push_back(e);
  }

  if (!flip90)
  {
    ui->PlotCalib->update_plot(xtoi.size(), ytoi.size(), data);
    ui->PlotCalib->set_axes(xcal, ycal, 1, ui->comboCodomain->currentText());
  }
  else
  {
    ui->PlotCalib->update_plot(ytoi.size(), xtoi.size(), data);
    ui->PlotCalib->set_axes(ycal, xcal, 1, ui->comboCodomain->currentText());
  }

//  ui->PlotCalib->clearGraphs();
//  if (!xx.isEmpty()) {
//    ui->PlotCalib->addPoints(style_pts, xx, yy, xx_sigma, yy_sigma);

//    std::set<double> chosen_peaks_chan;
//    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
//  }
//  ui->PlotCalib->setLabels(name, ui->comboCodomain->currentText());
//  ui->PlotCalib->set_selected_pts(std::set<double>());
//  ui->PlotCalib->redraw();

  ui->pushSaveCsv->setEnabled(filtered_data_points_.size());
}


void FormExperiment2D::list_relevant_domains()
{
  QString current = ui->comboDomain->currentText();
  QString current2 = ui->comboDomain2->currentText();
  std::map<std::string, Qpx::Setting> available;
  for (auto &p : all_data_points_)
  {
    if (p.idx_sink != selected_sink_)
      continue;
    for (auto &d : p.domains)
    {
//    cull irrelevant source settings filter by detector indices
      available.insert(d);
    }
  }
  ui->comboDomain->clear();
  ui->comboDomain2->clear();
  for (auto &d : available) {
    std::string name = d.second.metadata.name;
    if (name.empty())
      name = d.second.id_;
    ui->comboDomain->addItem(QString::fromStdString(d.first), QString::fromStdString(name));
    ui->comboDomain2->addItem(QString::fromStdString(d.first), QString::fromStdString(name));
  }
  if (available.count(current.toStdString()))
    ui->comboDomain->setCurrentText(current);
  if (available.count(current2.toStdString()))
    ui->comboDomain2->setCurrentText(current2);

  if (!available.count(current.toStdString()) || !available.count(current2.toStdString()))
    on_comboDomain_currentIndexChanged(0);
}

void FormExperiment2D::eval_dependent(Qpx::DataPoint &data)
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
  else if (codomain == "Centroid (selected peak)")
  {
    if (data.selected_peak != Qpx::Peak())
      data.dependent_variable = data.selected_peak.center();
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

void FormExperiment2D::on_comboDomain_currentIndexChanged(int index)
{
  domains_changed();
}

void FormExperiment2D::on_comboDomain2_currentIndexChanged(int index)
{
  domains_changed();
}

void FormExperiment2D::domains_changed()
{
  QString current = ui->comboDomain->currentText();
  QString current2 = ui->comboDomain2->currentText();
  std::map<std::string,std::set<double>> variations;
  for (auto &p : all_data_points_)
  {
    if (p.idx_sink != selected_sink_)
      continue;
    for (auto &d : p.domains)
    {
      if ((d.first == current.toStdString()) || (d.first == current2.toStdString()))
        continue;
//    cull irrelevant source settings filter by detector indices
      variations[d.first].insert(d.second.number());
    }
  }

  clearLayout(ui->filterLayout);
  domain_filter_ = new WidgetExperimentFilter(variations, this);
  connect(domain_filter_, SIGNAL(filter_changed()), this, SLOT(filter_changed()));
  ui->filterLayout->addWidget(domain_filter_);
  ui->labelFilters->setVisible(!variations.empty());

  filter_changed();
}

void FormExperiment2D::filter_changed()
{
  QString current = ui->comboDomain->currentText();
  QString current2 = ui->comboDomain2->currentText();
  filtered_data_points_.clear();
  for (auto p : all_data_points_)
  {
    if ((p.idx_sink != selected_sink_)
        || !p.domains.count(current.toStdString())
        || !p.domains.count(current2.toStdString())
        || !domain_filter_->valid(p.domains))
      continue;
    p.independent_variable =
        UncertainDouble::from_double(p.domains.at(current.toStdString()).number(),
                                     std::numeric_limits<double>::quiet_NaN());
    p.independent_variable2 =
        UncertainDouble::from_double(p.domains.at(current2.toStdString()).number(),
                                     std::numeric_limits<double>::quiet_NaN());
    filtered_data_points_.push_back(p);
  }
  display_data();
}

void FormExperiment2D::on_comboCodomain_currentIndexChanged(int index)
{
  display_data();
}

void FormExperiment2D::on_pushSaveCsv_clicked()
{
  QString fileName = CustomSaveFileDialog(this, "Save experiment data",
                                          data_directory_, "Comma separated values (*.csv)");
  if (!validateFile(this, fileName, true))
    return;

  data_directory_ = path_of_file(fileName);

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

