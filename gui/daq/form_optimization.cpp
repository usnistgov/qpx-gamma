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
 *      Optimization interface
 *
 ******************************************************************************/

#include "form_optimization.h"
#include "ui_form_optimization.h"
#include "custom_logger.h"
#include "fityk.h"
#include "qt_util.h"
#include "daq_sink_factory.h"
#include <QSettings>
#include "qt_util.h"
#include "qpx_util.h"
#include "dialog_spectra_templates.h"

#include "UncertainDouble.h"

#include "dialog_domain.h"

Q_DECLARE_METATYPE(Qpx::TrajectoryNode)

FormOptimization::FormOptimization(ThreadRunner& thread, XMLableDB<Qpx::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  opt_runner_thread_(thread),
  detectors_(detectors),
  interruptor_(false),
  opt_plot_thread_(),
  my_run_(false),
  continue_(false),
  tree_experiment_model_(this)
{
  ui->setupUi(this);
  this->setWindowTitle("Experiment");

  connect(&opt_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));
  connect(&opt_plot_thread_, SIGNAL(plot_ready()), this, SLOT(new_daq_data()));

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 10);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 10);

  ui->plotSpectrum->setFit(&selected_fitter_);
  connect(ui->plotSpectrum, SIGNAL(data_changed()), this, SLOT(update_fits()));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(fitting_done()), this, SLOT(fitting_done()));
  connect(ui->plotSpectrum, SIGNAL(fitter_busy(bool)), this, SLOT(fitter_status(bool)));

  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(pass_selected_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(pass_selected_in_plot()));


  ui->treeViewExperiment->setModel(&tree_experiment_model_);
  ui->treeViewExperiment->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->treeViewExperiment->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(item_selected_in_tree(QItemSelection,QItemSelection)));


  QStringList types;
  for (auto &a : Qpx::SinkFactory::getInstance().types())
    if (Qpx::SinkFactory::getInstance().create_prototype(a).dimensions() == 1)
      types.append(QString::fromStdString(a));
  ui->comboSinkType->addItems(types);

  ui->comboCodomain->addItem("FWHM (selected peak)");
  ui->comboCodomain->addItem("Count rate (selected peak)");
  ui->comboCodomain->addItem("Count rate (spectrum)");
  ui->comboCodomain->addItem("% dead time");

  loadSettings();

  update_settings();
  on_comboSinkType_activated("");
}


void FormOptimization::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    std::string path = profile_directory.toStdString() + "/optimize.set";
    pugi::xml_document doc;
    if (doc.load_file(path.c_str())) {
      pugi::xml_node root = doc.child("Optimizer");
      FitSettings fs;
      if (root.child(fs.xml_element_name().c_str())) {
        fs.from_xml(root.child(fs.xml_element_name().c_str()));
        selected_fitter_.apply_settings(fs);
      }
    }
  }

  settings_.beginGroup("Optimization");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  ui->comboCodomain->setCurrentText(settings_.value("co-domain", ui->comboCodomain->currentText()).toString());
  ui->checkAutofit->setChecked(settings_.value("autofit", true).toBool());
  ui->widgetAutoselect->loadSettings(settings_);

  ui->plotSpectrum->loadSettings(settings_);

  settings_.endGroup();
}

void FormOptimization::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    std::string path = profile_directory.toStdString() + "/optimize.set";
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    pugi::xml_node node = root.append_child("Optimizer");
    selected_fitter_.settings().to_xml(node);
    doc.save_file(path.c_str());
  }

  settings_.beginGroup("Optimization");
  settings_.setValue("bits", ui->spinBits->value());
  settings_.setValue("co-domain", ui->comboCodomain->currentText());
  settings_.setValue("autofit", ui->checkAutofit->isChecked());
  ui->widgetAutoselect->saveSettings(settings_);

  ui->plotSpectrum->saveSettings(settings_);

  settings_.endGroup();
}

FormOptimization::~FormOptimization()
{
  delete ui;
}

void FormOptimization::update_settings() {
  ui->comboTarget->clear();

  //should come from other thread?
  current_dets_ = Qpx::Engine::getInstance().get_detectors();

  for (int i=0; i < current_dets_.size(); ++i) {
    QString text = "[" + QString::number(i) + "] "
        + QString::fromStdString(current_dets_[i].name_);
    ui->comboTarget->addItem(text, QVariant::fromValue(i));
  }

  remake_source_domains();
  remake_sink_domains();
  remake_domains();
}

void FormOptimization::closeEvent(QCloseEvent *event) {
  if (my_run_ && opt_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      continue_ = false;
      opt_runner_thread_.terminate();
      opt_runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  if (!experiment_.empty()) {
    int reply = QMessageBox::warning(this, "Optimization data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
    {
      event->ignore();
      return;
    }
  }

  opt_plot_thread_.terminate_wait();

  saveSettings();
  event->accept();
}

void FormOptimization::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);
  bool empty = experiment_.empty();
  bool enable_domain_edit = (enable && !my_run_ && empty);

  ui->pushStart->setEnabled(online && enable_domain_edit);

  ui->comboTarget->setEnabled(enable_domain_edit);
  ui->spinBits->setEnabled(enable_domain_edit);
  ui->comboSinkType->setEnabled(enable_domain_edit);
  ui->pushEditPrototype->setEnabled(enable_domain_edit);

  ui->comboCodomain->setEnabled(enable);

  ui->pushClear->setEnabled(enable && !my_run_ && !empty);
  ui->pushSaveCsv->setEnabled(enable && !empty);
}

void FormOptimization::on_pushStart_clicked()
{
  experiment_.clear();
  FitSettings fitset = selected_fitter_.settings();
  selected_fitter_ = Qpx::Fitter();
  selected_fitter_.apply_settings(fitset);

  display_data();
  continue_ = true;
  start_new_pass();
}

void FormOptimization::on_pushStop_clicked()
{
  continue_ = false;
  interruptor_.store(true);
}

void FormOptimization::start_new_pass()
{
  std::pair<Qpx::ProjectPtr, uint64_t> pr = get_next_point();
  tree_experiment_model_.set_root(exp_project_.get_trajectories());

  if (!pr.first || !pr.second)
  {
    DBG << "No valid next point. Terminating epxeriment.";
    return;
  }

  DBG << "Good next point " << pr.first.operator bool() << " " << pr.second;

  ui->pushStop->setEnabled(true);
  my_run_ = true;
  emit toggleIO(false);

  //  INFO << "<FormOptimization> Starting pass #" << (current_pass_ + 1)
  //       << " with " << ui->comboSetting->currentText().toStdString() << " = "
  //       << current_setting_.number();

  DataPoint newdata;
  newdata.spectrum.apply_settings(selected_fitter_.settings());
  newdata.spectrum.clear();
  //  newdata.independent_variable = current_setting_.number();
  newdata.dependent_variable = std::numeric_limits<double>::quiet_NaN();
  experiment_.push_back(newdata);

  display_data(); //before new data arrives?
  //  ui->tableResults->selectRow(experiment_.size() - 1);
  //  pass_selected_in_table();

  interruptor_.store(false);
  opt_runner_thread_.do_run(pr.first, interruptor_, pr.second);
}

void FormOptimization::fitter_status(bool busy)
{
  ui->tableResults->setEnabled(!busy);
  ui->PlotCalib->setEnabled(!busy);
  ui->treeViewExperiment->setEnabled(!busy);
}

void FormOptimization::run_completed() {
  if (my_run_) {
    ui->pushStop->setEnabled(false);
    my_run_ = false;
    emit toggleIO(true);

    if (continue_)
    {
      INFO << "<FormOptimization> Completed one pass";
      start_new_pass();
    }
  }
}

void FormOptimization::display_data()
{
  ui->tableResults->blockSignals(true);

  if (ui->tableResults->rowCount() != experiment_.size())
    ui->tableResults->setRowCount(experiment_.size());

  ui->tableResults->setColumnCount(9);
  //  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_.metadata.name), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Total cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("Live time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("Real time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("Dead time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(5, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(6, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(7, new QTableWidgetItem("Peak cps", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(8, new QTableWidgetItem("Peak error", QTableWidgetItem::Type));

  QVector<double> xx(experiment_.size());;
  QVector<double> yy(experiment_.size());;
  QVector<double> yy_sigma(experiment_.size(), 0);

  for (int i = 0; i < experiment_.size(); ++i)
  {
    DataPoint &data = experiment_.at(i);
    eval_dependent(data);

    Qpx::Setting rt = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting lt = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);
    double real_ms = rt.value_duration.total_milliseconds();
    double live_ms = lt.value_duration.total_milliseconds();

    add_to_table(ui->tableResults, i, 0, std::to_string(data.independent_variable));

    double total_count = data.spectrum.metadata_.total_count.convert_to<double>();
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

    xx[i] = data.independent_variable;
    yy[i] = data.dependent_variable;
  }

  ui->tableResults->blockSignals(false);

  ui->PlotCalib->clearGraphs();
  if (!xx.isEmpty()) {
    ui->PlotCalib->addPoints(style_pts, xx, yy, QVector<double>(), yy_sigma);

    std::set<double> chosen_peaks_chan;
    //if selected insert;

    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    //    ui->PlotCalib->setLabels(QString::fromStdString(current_setting_.metadata.name),
    //                             ui->comboCodomain->currentText());
  }
  //  ui->PlotCalib->redraw();

  //  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
  //    std::set<double> sel;
  //    sel.insert(experiment_.at(selected_pass_).independent_variable);
  //    ui->PlotCalib->set_selected_pts(sel);
  //    ui->PlotCalib->redraw();
  //  }
  //  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    ui->PlotCalib->redraw();
  }


  ui->pushSaveCsv->setEnabled(experiment_.size());
}

void FormOptimization::pass_selected_in_table()
{
  int selected_pass_ = -1;
  foreach (QModelIndex i, ui->tableResults->selectionModel()->selectedRows())
    selected_pass_ = i.row();
  //  FitSettings fitset = selected_fitter_.settings();
  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
    //    selected_fitter_ = experiment_.at(selected_pass_).spectrum;
    std::set<double> sel;
    sel.insert(experiment_.at(selected_pass_).independent_variable);
    ui->PlotCalib->set_selected_pts(sel);
  }
  else
  {
    //    selected_fitter_ = Qpx::Fitter();
    ui->PlotCalib->set_selected_pts(std::set<double>());
  }
  //  selected_fitter_.apply_settings(fitset);
  ui->PlotCalib->redraw();

  if (!ui->plotSpectrum->busy()) {
    ui->plotSpectrum->updateData();
    if ((selected_pass_ >= 0)
        && (selected_pass_ < experiment_.size())
        && experiment_.at(selected_pass_).selected_peak != Qpx::Peak())
    {
      std::set<double> selected;
      selected.insert(experiment_.at(selected_pass_).selected_peak.center().value());
      ui->plotSpectrum->set_selected_peaks(selected);
    }
    //    if ((selected_fitter_.metadata_.total_count > 0)
    //        && selected_fitter_.peaks().empty()
    //        && ui->checkAutofit->isChecked())
    //      ui->plotSpectrum->perform_fit();
  }
}

void FormOptimization::pass_selected_in_plot()
{
  //allow only one point to be selected!!!
  std::set<double> selection = ui->PlotCalib->get_selected_pts();
  if (selection.size() < 1)
  {
    ui->tableResults->clearSelection();
    return;
  }

  double sel = *selection.begin();

  for (int i=0; i < experiment_.size(); ++i)
  {
    if (experiment_.at(i).independent_variable == sel) {
      ui->tableResults->selectRow(i);
      pass_selected_in_table();
      return;
    }
  }
}

void FormOptimization::update_fits() {

  //  ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->reselect(selected_fitter_.peaks(), ui->plotSpectrum->get_selected_peaks()));
  //  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size()))
  //    experiment_[selected_pass_].spectrum = selected_fitter_;
  update_peak_selection(std::set<double>());
}

void FormOptimization::fitting_done()
{
  Qpx::ProjectPtr p = opt_plot_thread_.current_source();

  if (!p)
    return;

  for (auto &q: p->get_sinks()) {
    Qpx::Metadata md;
    if (q.second)
      md = q.second->metadata();

    if (md == Qpx::Metadata())
      continue;

    p->update_fitter(q.first, selected_fitter_);
  }


  //  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
  //    experiment_[selected_pass_].spectrum = selected_fitter_;
  //    if (!selected_fitter_.peaks().empty() && (experiment_.at(selected_pass_).selected_peak == Qpx::Peak())) {
  //      //      DBG << "<FormOptimization> Autoselecting";
  //      ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->autoselect(selected_fitter_.peaks(), ui->plotSpectrum->get_selected_peaks()));
  //      update_peak_selection(std::set<double>());
  //    }
  //  }
}

void FormOptimization::update_peak_selection(std::set<double> dummy) {
  //  Qpx::Metadata md = selected_fitter_.metadata_;
  //  Qpx::Setting pass = md.attributes.branches.get(Qpx::Setting("Pass"));
  //  if (!pass || (pass.value_int < 0) || (pass.value_int >= experiment_.size()))
  //    return;

  //  std::set<double> selected_peaks = ui->plotSpectrum->get_selected_peaks();
  //  if ((selected_peaks.size() != 1) || !selected_fitter_.peaks().count(*selected_peaks.begin()))
  //    experiment_[pass.value_int].selected_peak = Qpx::Peak();
  //  else
  //    experiment_[pass.value_int].selected_peak = selected_fitter_.peaks().at(*selected_peaks.begin());

  //  eval_dependent(experiment_[pass.value_int]);
  //  display_data();

  //  if ((pass.value_int == current_pass_) && criterion_satisfied(experiment_[pass.value_int]))
  //    interruptor_.store(true);
}

void FormOptimization::update_name()
{
  QString name = "Experiment";
  if (my_run_)
    name += QString::fromUtf8("  \u25b6");
  //  else if (spectra_.changed())
  //    name += QString::fromUtf8(" \u2731");

  if (name != this->windowTitle()) {
    this->setWindowTitle(name);
    emit toggleIO(true);
  }
}


void FormOptimization::new_daq_data() {
  update_name();

  if (ui->plotSpectrum->busy())
    return;

  Qpx::ProjectPtr p = opt_plot_thread_.current_source();

  if (p) {
    for (auto &q: p->get_sinks()) {
      Qpx::Metadata md;
      if (q.second)
        md = q.second->metadata();

      if (md == Qpx::Metadata())
        continue;

      if (p->has_fitter(q.first))
      {
        selected_fitter_ = p->get_fitter(q.first);
        if (selected_fitter_.metadata_.total_count < md.total_count)
          selected_fitter_.setData(q.second);
      }
      else
      {
        FitSettings fitset = selected_fitter_.settings();
        selected_fitter_ = Qpx::Fitter();
        selected_fitter_.setData(q.second);
        selected_fitter_.apply_settings(fitset);
      }

      ui->plotSpectrum->updateData();

      if (!selected_fitter_.peak_count())
        ui->plotSpectrum->perform_fit();

      return;

      //    if ((md.total_count > 0) && (pass) && (pass.value_int < experiment_.size())) {
      //      experiment_[pass.value_int].spectrum.setData(q.second);
      //      double variable = md.attributes.branches.get(current_setting_).number();
      //      experiment_[pass.value_int].independent_variable = variable;
      //      eval_dependent(experiment_[pass.value_int]);
      //      display_data();
      //      if (pass.value_int == current_pass_) {
      //        if (criterion_satisfied(experiment_[pass.value_int]))
      //          interruptor_.store(true);
      //        if (current_pass_ == selected_pass_)
      //        {
      //          pass_selected_in_table();
      //          if (!ui->plotSpectrum->busy() && ui->checkAutofit->isChecked())
      //            ui->plotSpectrum->perform_fit();
      //        }
      //      }
      //    }
    }
  }

  FitSettings fitset = selected_fitter_.settings();
  selected_fitter_ = Qpx::Fitter();
  selected_fitter_.apply_settings(fitset);
  ui->plotSpectrum->updateData();
  return;


}

void FormOptimization::eval_dependent(DataPoint &data)
{
  QString codomain = ui->comboCodomain->currentText();
  data.dependent_variable = std::numeric_limits<double>::quiet_NaN();
  if (codomain == "Count rate (spectrum)")
  {
    double live_ms = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration.total_milliseconds();
    data.dependent_variable = data.spectrum.metadata_.total_count.convert_to<double>();
    if (live_ms > 0)
      data.dependent_variable = data.dependent_variable  / live_ms * 1000.0;
  }
  else if (codomain == "% dead time")
  {
    Qpx::Setting real = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting live = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);

    double dead = real.value_duration.total_milliseconds() - live.value_duration.total_milliseconds();
    if (real.value_duration.total_milliseconds() > 0)
      dead = dead / real.value_duration.total_milliseconds() * 100.0;
    else
      dead = 0;

    data.dependent_variable = dead;
  }
  else if (codomain == "FWHM (selected peak)")
  {
    if (data.selected_peak != Qpx::Peak())
      data.dependent_variable = data.selected_peak.fwhm().value();
  }
  else if (codomain == "Count rate (selected peak)")
  {
    double live_ms = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration.total_milliseconds();

    if (data.selected_peak != Qpx::Peak()) {
      //      DBG << "selpeak area " << data.selected_peak.area_best.to_string() <<
      //             "  cps " << data.selected_peak.cps_best.to_string();
      data.dependent_variable = data.selected_peak.area_best().value();
      if (live_ms > 0)
        data.dependent_variable = data.dependent_variable  / live_ms * 1000.0;
    }
  }
}

bool FormOptimization::criterion_satisfied(DataPoint &data)
{
  Qpx::Metadata md = data.spectrum.metadata_;

  //  if (ui->comboUntil->currentText() == "Real time >")
  //    return md.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id).value_duration > ui->timeDuration->get_duration();
  //  else if (ui->comboUntil->currentText() == "Live time >")
  //    return md.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration > ui->timeDuration->get_duration();
  //  else if (ui->comboUntil->currentText() == "Total count >")
  //    return (md.total_count > ui->doubleCriterion->value());
  //  else if (ui->comboUntil->currentText() == "Peak area % error <")
  //    return ((data.selected_peak != Qpx::Peak())
  //            && (data.selected_peak.area_best().error() < ui->doubleCriterion->value()));
  //  else
  //  {
  //    WARN << "<FormOptimize> Bad criterion for DAQ termination";
  //    current_setting_.metadata.maximum = current_setting_.number();
  //    return true;
  //  }
  return false;
}


void FormOptimization::on_comboTarget_currentIndexChanged(int index)
{
  uint16_t channel = ui->comboTarget->currentData().toInt();
  sink_prototype_.set_det_limit(current_dets_.size());

  std::vector<bool> gates(current_dets_.size(), false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = sink_prototype_.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_.attributes.branches.replace(pattern);
  pattern = sink_prototype_.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_.attributes.branches.replace(pattern);

  exp_project_.set_prototype(sink_prototype_);

  remake_source_domains();
  remake_sink_domains();
  remake_domains();
}

void FormOptimization::remake_domains()
{
  all_domains_.clear();

  for (auto &s : source_settings_)
    if (s.second.metadata.writable
        && s.second.metadata.visible
        && s.second.is_numeric()) {
      Qpx::Domain domain;
      domain.verbose = s.first;
      domain.type = Qpx::DomainType::source;
      domain.value_range = s.second;
      all_domains_[s.first] = domain;
    }

  for (auto &s : sink_settings_)
    if (s.second.metadata.writable
        && s.second.metadata.visible
        && s.second.is_numeric())
    {
      Qpx::Domain domain;
      domain.verbose = s.first;
      domain.type = Qpx::DomainType::sink;
      domain.value_range = s.second;
      all_domains_[s.first] = domain;
    }
}


void FormOptimization::on_pushSaveCsv_clicked()
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

void FormOptimization::on_comboSinkType_activated(const QString &arg1)
{
  sink_prototype_ = Qpx::SinkFactory::getInstance().create_prototype(ui->comboSinkType->currentText().toStdString());
  sink_prototype_.name = "experiment";
  sink_prototype_.bits = ui->spinBits->value();

  Qpx::Setting vis = sink_prototype_.attributes.branches.get(Qpx::Setting("visible"));
  vis.value_int = true;
  sink_prototype_.attributes.branches.replace(vis);

  Qpx::Setting app = sink_prototype_.attributes.branches.get(Qpx::Setting("appearance"));
  QColor col;
  col.setHsv(QColor(Qt::blue).hsvHue(), 48, 160);
  app.value_text = col.name().toStdString();
  sink_prototype_.attributes.branches.replace(app);

  uint16_t channel = ui->comboTarget->currentData().toInt();
  sink_prototype_.set_det_limit(current_dets_.size());

  std::vector<bool> gates(current_dets_.size(), false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = sink_prototype_.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_.attributes.branches.replace(pattern);
  pattern = sink_prototype_.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_.attributes.branches.replace(pattern);

  exp_project_.set_prototype(sink_prototype_);

  //  Qpx::Setting set_pass("Pass");
  //  set_pass.value_int = pass_number;
  //  sink_prototype_.attributes.branches.replace(set_pass);

  //  sink_prototype_.attributes.branches.replace(variable);

  remake_source_domains();
  remake_sink_domains();
  remake_domains();
}

void FormOptimization::remake_source_domains()
{
  source_settings_.clear();
  for (uint16_t i=0; i < current_dets_.size(); ++i) {
    if (!sink_prototype_.chan_relevant(i))
      continue;
    for (auto &q : current_dets_[i].settings_.branches.my_data_)
      source_settings_["[DAQ] " + q.id_
          + " (" + std::to_string(i) + ":" + current_dets_[i].name_ + ")"] = q;
  }
}

void FormOptimization::remake_sink_domains()
{
  sink_settings_.clear();
  Qpx::Setting set;
  sink_prototype_.attributes.cull_invisible();
  set.branches.my_data_ = sink_prototype_.attributes.find_all(set, Qpx::Match::indices);
  for (auto &q : set.branches.my_data_)
    sink_settings_["[" + sink_prototype_.type() + "] " + q.id_] = q;
  for (uint16_t i=0; i < current_dets_.size(); ++i) {
    if (!sink_prototype_.chan_relevant(i))
      continue;
    set.indices.clear();
    set.indices.insert(i);
    set.branches.my_data_ = sink_prototype_.attributes.find_all(set, Qpx::Match::indices);
    for (auto &q : set.branches.my_data_)
      sink_settings_["[" + sink_prototype_.type() + "] " + q.id_
          + " (" + std::to_string(i) + ":" + current_dets_[i].name_ + ")"] = q;
  }
}

void FormOptimization::on_pushEditPrototype_clicked()
{
  DialogSpectrumTemplate* newDialog = new DialogSpectrumTemplate(sink_prototype_, current_dets_, false, this);
  if (newDialog->exec()) {
    sink_prototype_ = newDialog->product();

    exp_project_.set_prototype(sink_prototype_);

    remake_source_domains();
    remake_sink_domains();
    remake_domains();
  }
}

void FormOptimization::on_pushClear_clicked()
{
  experiment_.clear();

  display_data();
  pass_selected_in_table();

  emit toggleIO(true);
}

void FormOptimization::on_comboCodomain_activated(const QString &arg1)
{
  display_data();
}

void FormOptimization::item_selected_in_tree(QItemSelection,QItemSelection)
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  ui->pushAddSubdomain->setEnabled(!idx.empty());
  ui->pushEditDomain->setEnabled(!idx.empty());
  ui->pushDeleteDomain->setEnabled(!idx.empty());

  for (auto &i : idx) {
    if (i.isValid()) {
      QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
      if (!data.canConvert<Qpx::TrajectoryNode>())
        return;
      Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

      if (tn.data_idx < 0)
        return;
      Qpx::ProjectPtr p = exp_project_.get_data(tn.data_idx);
      if (!p)
        return;

      INFO << "project good to view with idx = " << tn.data_idx;

      opt_plot_thread_.monitor_source(p);
      p->activate();
      return;
    }
  }
}

void FormOptimization::on_pushAddSubdomain_clicked()
{
  Qpx::Domain newdomain;
  DialogDomain diag(newdomain, all_domains_, this);
  int ret = diag.exec();
  if (!ret || (newdomain.type == Qpx::DomainType::none))
    return;

  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();

  Qpx::TrajectoryNode tn;
  tn.domain = newdomain;

  for (auto &i : idx) {
    if (i.isValid()) {
      tree_experiment_model_.push_back(i, tn);
      return;
    }
  }

  //  auto root = tree_experiment_model_.getRoot();
  //  root->push_back(tn);
  //  tree_experiment_model_.set_root(root);
}

void FormOptimization::on_pushDeleteDomain_clicked()
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  if (idx.empty())
    return;
  for (auto &i : idx) {
    if (i.isValid()) {
      tree_experiment_model_.remove_row(i);
      return;
    }
  }
}

void FormOptimization::on_pushEditDomain_clicked()
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();

  for (auto &i : idx) {
    if (i.isValid()) {
      QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
      if (!data.canConvert<Qpx::TrajectoryNode>())
        return;
      Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

      if ((tn.data_idx >= 0) || (tn.domain_value != Qpx::Setting()))
        return;

      DialogDomain diag(tn.domain, all_domains_, this);
      int ret = diag.exec();
      if (!ret || (tn.domain.type == Qpx::DomainType::none))
        return;

      tree_experiment_model_.setData(i, QVariant::fromValue(tn), Qt::EditRole);
      return;
    }
  }
}

std::pair<Qpx::ProjectPtr, uint64_t> FormOptimization::get_next_point()
{
  std::pair<Qpx::DomainType, Qpx::TrajectoryPtr> ret = exp_project_.next_setting();
  if (!ret.second)
    return std::pair<Qpx::ProjectPtr, uint64_t>(nullptr,0);

  INFO << "Next setting " << ret.second->type() << " " << ret.second->to_string();

  if (ret.first == Qpx::DomainType::manual)
  {
    Qpx::Setting set = ret.second->domain_value;
    QString text = "Please adjust manual setting \'"
        + QString::fromStdString(set.id_) + "\' and confirm value: ";
    bool ok;
    double d = QInputDialog::getDouble(this,
                                       "Confirm manual setting",
                                       text, set.number(),
                                       set.metadata.minimum,
                                       set.metadata.maximum,
                                       set.metadata.step,
                                       &ok);
    if (ok)
      ret.second->domain_value.set_number(d);
    else
    {
      INFO << "Manual abort";
      update_name();
      return std::pair<Qpx::ProjectPtr, uint64_t>(nullptr,0);
    }
  }
  else if (ret.first == Qpx::DomainType::source)
  {
    Qpx::Engine::getInstance().set_setting(ret.second->domain_value, Qpx::Match::id | Qpx::Match::indices);
    QThread::sleep(1);
    Qpx::Engine::getInstance().get_all_settings();
    double newval = Qpx::Engine::getInstance().pull_settings().get_setting(ret.second->domain_value, Qpx::Match::id | Qpx::Match::indices).number();
    //    if (newval < current_setting_.number())
    //      return;
    ret.second->domain_value.set_number(newval);

    emit settings_changed();
  }

  if (ret.second->data_idx >= 0)
    return std::pair<Qpx::ProjectPtr, uint64_t>(exp_project_.get_data(ret.second->data_idx),
                                                ret.second->domain.criterion);
  else
    return get_next_point();
}

void FormOptimization::on_pushLoadExperiment_clicked()
{
  QSettings settings;
  settings.beginGroup("Program");
  QString data_directory = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();

  QString fileName = QFileDialog::getOpenFileName(this, "Load experiment",
                                                  data_directory, "qpx multi-experiment (*.qmx)");
  if (!validateFile(this, fileName, false))
    return;

  if (!fileName.isEmpty()) {
    pugi::xml_document doc;
    if (doc.load_file(fileName.toStdString().c_str())) {
      if (doc.child(exp_project_.xml_element_name().c_str()))
        exp_project_.from_xml(doc.child(exp_project_.xml_element_name().c_str()));
      exp_project_.set_prototype(sink_prototype_);
      tree_experiment_model_.set_root(exp_project_.get_trajectories());
    }
  }
}

void FormOptimization::on_pushSaveExperiment_clicked()
{
  QSettings settings;
  settings.beginGroup("Program");
  QString data_directory = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();

  QString fileName = CustomSaveFileDialog(this, "Save experiment",
                                          data_directory, "Qpx multi-experiment (*.qmx)");
  if (!validateFile(this, fileName, true))
    return;

  if (!fileName.isEmpty()) {
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    exp_project_.to_xml(root);
    doc.save_file(fileName.toStdString().c_str());
  }

}
