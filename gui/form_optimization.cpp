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

#include "UncertainDouble.h"

FormOptimization::FormOptimization(ThreadRunner& thread, XMLableDB<Qpx::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  opt_runner_thread_(thread),
  detectors_(detectors),
  interruptor_(false),
  opt_plot_thread_(project_),
  my_run_(false)
{
  ui->setupUi(this);
  this->setWindowTitle("Experiment");

  //loadSettings();
  connect(&opt_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  current_pass_ = 0;
  selected_pass_ = -1;

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

  //connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  populate_display();
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(pass_selected_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(pass_selected_in_plot()));

  connect(&opt_plot_thread_, SIGNAL(plot_ready()), this, SLOT(new_daq_data()));

  ui->timeDuration->set_us_enabled(false);
  ui->timeDuration->setVisible(false);
  ui->doubleCriterion->setValue(false);

  ui->comboUntil->addItem("Real time >");
  ui->comboUntil->addItem("Live time >");
  ui->comboUntil->addItem("Total count >");
  ui->comboUntil->addItem("Peak area % error <");

  ui->comboCodomain->addItem("FWHM (selected peak)");
  ui->comboCodomain->addItem("Counts (selected peak)");
  ui->comboCodomain->addItem("Total count");
  ui->comboCodomain->addItem("% dead time");

  manual_settings_ = Qpx::Setting("manual_optimization_settings");
  manual_settings_.metadata.setting_type = Qpx::SettingType::stem;

  loadSettings();

  update_settings();

  opt_plot_thread_.start();
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
      if (root.child(manual_settings_.xml_element_name().c_str()))
        manual_settings_.from_xml(root.child(manual_settings_.xml_element_name().c_str()));
      FitSettings fs;
      if (root.child(fs.xml_element_name().c_str())) {
        fs.from_xml(root.child(fs.xml_element_name().c_str()));
        selected_fitter_.apply_settings(fs);
      }
    }
  }

  settings_.beginGroup("Optimization");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  //  ui->spinOptChan->setValue(settings_.value("channel", 0).toInt());
  ui->comboUntil->setCurrentText(settings_.value("criterion", ui->comboUntil->currentText()).toString());
  ui->doubleCriterion->setValue(settings_.value("double_criterion", 1.0).toDouble());
  ui->timeDuration->set_total_seconds(settings_.value("time_criterion", 60).toULongLong());
  ui->comboCodomain->setCurrentText(settings_.value("co-domain", ui->comboUntil->currentText()).toString());
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
    if (!manual_settings_.branches.empty())
      manual_settings_.to_xml(node);
    selected_fitter_.settings().to_xml(node);
    doc.save_file(path.c_str());
  }

  settings_.beginGroup("Optimization");
  settings_.setValue("bits", ui->spinBits->value());
  //  settings_.setValue("channel", ui->spinOptChan->value());
  settings_.setValue("double_criterion", ui->doubleCriterion->value());
  settings_.setValue("time_criterion", QVariant::fromValue(ui->timeDuration->total_seconds()));
  settings_.setValue("criterion", ui->comboUntil->currentText());
  settings_.setValue("co-domain", ui->comboCodomain->currentText());
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
  std::vector<Qpx::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  for (int i=0; i < chans.size(); ++i) {
    QString text = "[" + QString::number(i) + "] "
        + QString::fromStdString(chans[i].name_);
    ui->comboTarget->addItem(text, QVariant::fromValue(i));
  }

  on_comboSetting_activated(ui->comboSetting->currentText());
  on_comboUntil_activated(ui->comboUntil->currentText());
}

void FormOptimization::closeEvent(QCloseEvent *event) {
  if (my_run_ && opt_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      current_setting_.value_dbl += current_setting_.metadata.step;
      QThread::sleep(2);

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
  project_.terminate();
  opt_plot_thread_.wait();

  saveSettings();
  event->accept();
}

void FormOptimization::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);

  ui->pushStart->setEnabled(enable && online && !my_run_);
  ui->comboSetting->setEnabled(enable && !my_run_);
  ui->doubleSpinStart->setEnabled(enable && !my_run_);
  ui->doubleSpinDelta->setEnabled(enable);
  ui->doubleSpinEnd->setEnabled(enable);
  ui->comboTarget->setEnabled(enable && !my_run_);
  ui->spinBits->setEnabled(enable && !my_run_);
  ui->comboCodomain->setEnabled(enable && !my_run_);
  ui->pushAddCustom->setEnabled(enable && !my_run_);
}

void FormOptimization::on_pushStart_clicked()
{
  experiment_.clear();
  FitSettings fitset = selected_fitter_.settings();
  selected_fitter_ = Qpx::Fitter();
  selected_fitter_.apply_settings(fitset);
  current_setting_.id_ = ui->comboSetting->currentText().toStdString();
  current_setting_.value_dbl = ui->doubleSpinStart->value();
  current_setting_.metadata.minimum = ui->doubleSpinStart->value();
  current_setting_.metadata.maximum = ui->doubleSpinEnd->value();
  current_setting_.metadata.step = ui->doubleSpinDelta->value();

  current_pass_ = 0;
  selected_pass_ = -1;

  populate_display();
  new_run();
}

void FormOptimization::on_pushStop_clicked()
{
  current_setting_.value_dbl = current_setting_.metadata.maximum + 1;
  interruptor_.store(true);
}

Qpx::Metadata FormOptimization::make_prototype(uint16_t bits, uint16_t channel,
                                               int pass_number, Qpx::Setting variable,
                                               std::string name)
{
  Qpx::Metadata    spectrum_prototype;
  spectrum_prototype = Qpx::SinkFactory::getInstance().create_prototype("1D");
  spectrum_prototype.name = name;
  spectrum_prototype.bits = bits;

  Qpx::Setting vis = spectrum_prototype.attributes.branches.get(Qpx::Setting("visible"));
  vis.value_int = true;
  spectrum_prototype.attributes.branches.replace(vis);


  Qpx::Setting app = spectrum_prototype.attributes.branches.get(Qpx::Setting("appearance"));
  QColor col;
  col.setHsv(QColor(Qt::blue).hsvHue(), 48, 160);
  app.value_text = col.name().toStdString();
  spectrum_prototype.attributes.branches.replace(app);

  std::vector<bool> gates(channel + 1, false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = spectrum_prototype.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  spectrum_prototype.attributes.branches.replace(pattern);
  pattern = spectrum_prototype.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  spectrum_prototype.attributes.branches.replace(pattern);

  Qpx::Setting set_pass("Pass");
  set_pass.value_int = pass_number;
  spectrum_prototype.attributes.branches.replace(set_pass);

  spectrum_prototype.attributes.branches.replace(variable);

  return spectrum_prototype;
}

void FormOptimization::new_run()
{
  int optchan = ui->comboTarget->currentData().toInt();
  current_setting_.indices.clear();
  current_setting_.indices.insert(optchan);

  if (current_setting_.metadata.flags.count("manual"))
  {
    bool ok;
    double d = QInputDialog::getDouble(this, QString::fromStdString(current_setting_.id_),
                                       "Value?=", current_setting_.value_dbl,
                                       current_setting_.metadata.minimum,
                                       current_setting_.metadata.maximum,
                                       current_setting_.metadata.step,
                                       &ok);
    if (ok)
      current_setting_.value_dbl = d;
    else
      return;
  }
  else
  {
    Qpx::Engine::getInstance().set_setting(current_setting_, Qpx::Match::id | Qpx::Match::indices);
    QThread::sleep(1);
    Qpx::Engine::getInstance().get_all_settings();
    current_setting_.value_dbl = Qpx::Engine::getInstance().pull_settings().get_setting(current_setting_, Qpx::Match::id | Qpx::Match::indices).value_dbl;
  }

  ui->pushStop->setEnabled(true);
  my_run_ = true;
  emit toggleIO(false);

  INFO << "<FormOptimization> Starting pass #" << current_pass_
       << " with " << current_setting_.id_ << "[" << optchan << "] = "
       << current_setting_.value_dbl;

  project_.clear();
  project_.add_sink(make_prototype(ui->spinBits->value(), optchan,
                                   current_pass_, current_setting_,
                                   "Optimizing"));
  ui->plotSpectrum->update_spectrum();

  DataPoint newdata;
  newdata.spectrum.apply_settings(selected_fitter_.settings());
  newdata.independent_variable = current_setting_.value_dbl;
  newdata.dependent_variable = std::numeric_limits<double>::quiet_NaN();

  experiment_.push_back(newdata);
  populate_display(); //before new data arrives?

  emit settings_changed();

  opt_plot_thread_.start();
  interruptor_.store(false);
  opt_runner_thread_.do_run(project_, interruptor_, 0);
}

void FormOptimization::fitter_status(bool busy)
{
  ui->tableResults->setEnabled(!busy);
  ui->PlotCalib->setEnabled(!busy);
}

void FormOptimization::run_completed() {
  if (my_run_) {
    project_.terminate();
    opt_plot_thread_.wait();

    ui->pushStop->setEnabled(false);
    my_run_ = false;
    emit toggleIO(true);

    if (current_pass_ >= 0) {
      INFO << "<FormOptimization> Completed pass #" << current_pass_;
      do_post_processing();
    }
  }
}

void FormOptimization::do_post_processing() {

  if (current_setting_.value_dbl < current_setting_.metadata.maximum) {
    current_setting_.value_dbl += current_setting_.metadata.step;
    current_pass_++;
    QThread::sleep(2);
    new_run();
  } else {
    INFO << "<FormOptimization> Experiment finished";
    update_name();
    return;
  }
}

void FormOptimization::populate_display()
{
  int old_row_count = ui->tableResults->rowCount();

  ui->tableResults->blockSignals(true);

  if (ui->tableResults->rowCount() != experiment_.size())
    ui->tableResults->setRowCount(experiment_.size());

  ui->tableResults->setColumnCount(9);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_.metadata.name), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Total count", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("Real time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("Live time", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("% dead", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(5, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(6, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(7, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(8, new QTableWidgetItem("%error", QTableWidgetItem::Type));

  QVector<double> xx(experiment_.size());;
  QVector<double> yy(experiment_.size());;
  QVector<double> yy_sigma(experiment_.size(), 0);

  for (int i = 0; i < experiment_.size(); ++i)
  {
    const DataPoint &data = experiment_.at(i);

    QTableWidgetItem *st = new QTableWidgetItem(QString::number(data.independent_variable));
    st->setFlags(st->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 0, st);

    double total_count = data.spectrum.metadata_.total_count.convert_to<double>();
    QTableWidgetItem *tc = new QTableWidgetItem(QString::number(total_count));
    tc->setFlags(tc->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 1, tc);

    Qpx::Setting real = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id);
    Qpx::Setting live = data.spectrum.metadata_.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id);


    QTableWidgetItem *rt = new QTableWidgetItem(QString::fromStdString(real.val_to_pretty_string()));
    rt->setFlags(rt->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 2, rt);

    QTableWidgetItem *lt = new QTableWidgetItem(QString::fromStdString(live.val_to_pretty_string()));
    lt->setFlags(lt->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 3, lt);

    double dead = real.value_duration.total_milliseconds() - live.value_duration.total_milliseconds();
    if (real.value_duration.total_milliseconds() > 0)
      dead = dead / real.value_duration.total_milliseconds() * 100.0;
    else
      dead = 0;

    QTableWidgetItem *dt = new QTableWidgetItem(QString::number(dead));
    dt->setFlags(dt->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 4, dt);

    UncertainDouble nrg = UncertainDouble::from_double(data.selected_peak.energy.val,
                                                       data.selected_peak.energy.uncert, 2);
    QTableWidgetItem *en = new QTableWidgetItem(QString::fromStdString(nrg.to_string(false, true)));
    en->setFlags(en->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 5, en);

    QTableWidgetItem *fw = new QTableWidgetItem(QString::number(data.selected_peak.fwhm_hyp));
    fw->setFlags(fw->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 6, fw);

    UncertainDouble ar = UncertainDouble::from_double(data.selected_peak.area_best.val,
                                                       data.selected_peak.area_best.uncert, 2);
    QTableWidgetItem *area = new QTableWidgetItem(QString::fromStdString(ar.to_string(false, true)));
    area->setFlags(area->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 7, area);

    QTableWidgetItem *err = new QTableWidgetItem(QString::number(data.selected_peak.sum4_.peak_area.err()));
    err->setFlags(err->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(i, 8, err);

    xx[i] = data.independent_variable;
    yy[i] = data.dependent_variable;
  }

  ui->tableResults->blockSignals(false);

  ui->PlotCalib->clearGraphs();
  if (!xx.isEmpty()) {
    ui->PlotCalib->addPoints(xx, yy, yy_sigma, style_pts);

    std::set<double> chosen_peaks_chan;
    //if selected insert;

    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    ui->PlotCalib->setLabels(QString::fromStdString(current_setting_.metadata.name),
                             ui->comboCodomain->currentText());
  }
//  ui->PlotCalib->redraw();

  if (experiment_.size() && (experiment_.size() == (old_row_count + 1))) {
    ui->tableResults->selectRow(old_row_count);
    pass_selected_in_table();
  } else if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
      std::set<double> sel;
      sel.insert(experiment_.at(selected_pass_).independent_variable);
      ui->PlotCalib->set_selected_pts(sel);
      ui->PlotCalib->redraw();
  }
  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    ui->PlotCalib->redraw();
  }
}

void FormOptimization::pass_selected_in_table()
{
  selected_pass_ = -1;
  foreach (QModelIndex i, ui->tableResults->selectionModel()->selectedRows())
    selected_pass_ = i.row();
  FitSettings fitset = selected_fitter_.settings();
  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
    selected_fitter_ = experiment_.at(selected_pass_).spectrum;
    std::set<double> sel;
    sel.insert(experiment_.at(selected_pass_).independent_variable);
    ui->PlotCalib->set_selected_pts(sel);
    ui->PlotCalib->redraw();
  }
  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    selected_fitter_ = Qpx::Fitter();
    ui->PlotCalib->redraw();
  }
  selected_fitter_.apply_settings(fitset);


  if (!ui->plotSpectrum->busy() && (selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
//    DBG << "fitter not busy";
    ui->plotSpectrum->updateData();
    if (experiment_.at(selected_pass_).selected_peak != Qpx::Peak()) {
      std::set<double> selected;
      selected.insert(experiment_.at(selected_pass_).selected_peak.center.val);
      ui->plotSpectrum->set_selected_peaks(selected);
    }
    if ((selected_fitter_.metadata_.total_count > 0)
        && selected_fitter_.peaks().empty()
        && !ui->plotSpectrum->busy())
      ui->plotSpectrum->perform_fit();
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
//  DBG << "<FormOptimization> Reselecting";
  ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->reselect(selected_fitter_.peaks(), ui->plotSpectrum->get_selected_peaks()));
  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size()))
    experiment_[selected_pass_].spectrum = selected_fitter_;
  update_peak_selection(std::set<double>());
}

void FormOptimization::fitting_done()
{
  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size())) {
    experiment_[selected_pass_].spectrum = selected_fitter_;
    if (!selected_fitter_.peaks().empty() && (experiment_.at(selected_pass_).selected_peak == Qpx::Peak())) {
//      DBG << "<FormOptimization> Autoselecting";
      ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->autoselect(selected_fitter_.peaks(), ui->plotSpectrum->get_selected_peaks()));
      update_peak_selection(std::set<double>());
    }
  }
}

void FormOptimization::update_peak_selection(std::set<double> dummy) {
  Qpx::Metadata md = selected_fitter_.metadata_;
  Qpx::Setting pass = md.attributes.branches.get(Qpx::Setting("Pass"));
  if (!pass || (pass.value_int < 0) || (pass.value_int >= experiment_.size()))
    return;

  std::set<double> selected_peaks = ui->plotSpectrum->get_selected_peaks();
  if ((selected_peaks.size() != 1) || !selected_fitter_.peaks().count(*selected_peaks.begin()))
    experiment_[pass.value_int].selected_peak = Qpx::Peak();
  else
    experiment_[pass.value_int].selected_peak = selected_fitter_.peaks().at(*selected_peaks.begin());

  eval_dependent(experiment_[pass.value_int]);
  populate_display();

  if ((pass.value_int == current_pass_) && criterion_satisfied(experiment_[pass.value_int]))
    interruptor_.store(true);
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

  for (auto &q: project_.get_sinks()) {
    Qpx::Metadata md;
    if (q.second)
      md = q.second->metadata();

    Qpx::Setting pass = md.attributes.branches.get(Qpx::Setting("Pass"));

    if ((md.total_count > 0) && (pass) && (pass.value_int < experiment_.size())) {
      experiment_[pass.value_int].spectrum.setData(q.second);
      double variable = md.attributes.branches.get(current_setting_).value_dbl;
      experiment_[pass.value_int].independent_variable = variable;
      eval_dependent(experiment_[pass.value_int]);
      populate_display();
      if (pass.value_int == current_pass_) {
        if (criterion_satisfied(experiment_[pass.value_int]))
          interruptor_.store(true);
        if (current_pass_ == selected_pass_)
        {
          pass_selected_in_table();
          if (!ui->plotSpectrum->busy())
            ui->plotSpectrum->perform_fit();
        }
      }
    }
  }
}

void FormOptimization::eval_dependent(DataPoint &data)
{
  QString codomain = ui->comboCodomain->currentText();
  if (codomain == "Total count")
  {
    data.dependent_variable = data.spectrum.metadata_.total_count.convert_to<double>();
    return;
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
    return;
  }
  else if (codomain == "FWHM (selected peak)")
  {
    if (data.selected_peak != Qpx::Peak())
      data.dependent_variable = data.selected_peak.fwhm_hyp;
    else
      data.dependent_variable = std::numeric_limits<double>::quiet_NaN();
    return;
  }
  else if (codomain == "Counts (selected peak)")
  {
    if (data.selected_peak != Qpx::Peak())
      data.dependent_variable = data.selected_peak.area_best.val;
    else
      data.dependent_variable = std::numeric_limits<double>::quiet_NaN();
    return;
  }
  data.dependent_variable = std::numeric_limits<double>::quiet_NaN();
}

bool FormOptimization::criterion_satisfied(DataPoint &data)
{
  Qpx::Metadata md = data.spectrum.metadata_;

  if (ui->comboUntil->currentText() == "Real time >")
    return md.attributes.get_setting(Qpx::Setting("real_time"), Qpx::Match::id).value_duration > ui->timeDuration->get_duration();
  else if (ui->comboUntil->currentText() == "Live time >")
    return md.attributes.get_setting(Qpx::Setting("live_time"), Qpx::Match::id).value_duration > ui->timeDuration->get_duration();
  else if (ui->comboUntil->currentText() == "Total count >")
    return (md.total_count > ui->doubleCriterion->value());
  else if (ui->comboUntil->currentText() == "Peak area % error <")
    return ((data.selected_peak != Qpx::Peak())
             && (data.selected_peak.sum4_.peak_area.err() < ui->doubleCriterion->value()));
  else
  {
    WARN << "<FormOptimize> Bad criterion for DAQ termination";
    current_setting_.value_dbl = current_setting_.metadata.maximum + 1;
    return true;
  }
  return false;
}


void FormOptimization::on_comboTarget_currentIndexChanged(int index)
{
  int optchan = ui->comboTarget->currentData().toInt();

  std::vector<Qpx::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  all_settings_.clear();
  if (optchan < chans.size())
    for (auto &q : chans[optchan].settings_.branches.my_data_)
      if (q.metadata.flags.count("optimize") > 0)
        all_settings_[q.id_] = q;
  for (auto &q : manual_settings_.branches.my_data_)
    all_settings_[q.id_] = q;

  ui->comboSetting->clear();
  for (auto &q : all_settings_)
    ui->comboSetting->addItem(QString::fromStdString(q.first));
}

void FormOptimization::on_comboSetting_activated(const QString &arg1)
{
  int optchan = ui->comboTarget->currentData().toInt();
  current_setting_ = Qpx::Setting();
  if (all_settings_.count(ui->comboSetting->currentText().toStdString())) {
    current_setting_ = all_settings_.at( ui->comboSetting->currentText().toStdString() );
  }

  current_setting_.indices.clear();
  current_setting_.indices.insert(optchan);

  Qpx::Setting set = Qpx::Engine::getInstance().pull_settings().get_setting(current_setting_, Qpx::Match::id | Qpx::Match::indices);
  if (set) {
    current_setting_ = set;
  }

  ui->doubleSpinStart->setRange(current_setting_.metadata.minimum, current_setting_.metadata.maximum);
  ui->doubleSpinStart->setSingleStep(current_setting_.metadata.step);
  ui->doubleSpinStart->setValue(current_setting_.metadata.minimum);

  ui->doubleSpinEnd->setRange(current_setting_.metadata.minimum, current_setting_.metadata.maximum);
  ui->doubleSpinEnd->setSingleStep(current_setting_.metadata.step);
  ui->doubleSpinEnd->setValue(current_setting_.metadata.maximum);

  ui->doubleSpinDelta->setRange(current_setting_.metadata.step, current_setting_.metadata.maximum - current_setting_.metadata.minimum);
  ui->doubleSpinDelta->setSingleStep(current_setting_.metadata.step);
  ui->doubleSpinDelta->setValue(current_setting_.metadata.step);
}

void FormOptimization::on_pushAddCustom_clicked()
{
  bool ok;
  QString name = QInputDialog::getText(this, "New custom variable",
                                       "Name:", QLineEdit::Normal, "", &ok);
  if (ok) {
    Qpx::Setting newset(name.toStdString());
    if (manual_settings_.has(newset, Qpx::Match::id))
      QMessageBox::information(this, "Already exists",
                               "Manual setting \'" + name + "\'' already exists");
    else
    {
      newset.metadata.name = name.toStdString();
      newset.metadata.setting_type = Qpx::SettingType::floating;
      newset.metadata.flags.insert("manual");

      double d = 0;

      d = QInputDialog::getDouble(this, "Minimum",
                                  "Minimum value for " + name + " = ",
                                  0,
                                  std::numeric_limits<double>::min(),
                                  std::numeric_limits<double>::max(),
                                  0.001,
                                  &ok);
      if (ok)
        newset.metadata.minimum = d;
      else
        return;

      d = QInputDialog::getDouble(this, "Maximum",
                                  "Maximum value for " + name + " = ",
                                  std::max(100.0, newset.metadata.minimum),
                                  newset.metadata.minimum,
                                  std::numeric_limits<double>::max(),
                                  0.001,
                                  &ok);
      if (ok)
        newset.metadata.maximum = d;
      else
        return;

      d = QInputDialog::getDouble(this, "Step",
                                  "Step value for " + name + " = ",
                                  1,
                                  0,
                                  newset.metadata.maximum - newset.metadata.minimum,
                                  0.001,
                                  &ok);
      if (ok)
        newset.metadata.step = d;
      else
        return;


      manual_settings_.branches.add(newset);
      on_comboTarget_currentIndexChanged(ui->comboTarget->currentIndex());
    }
  }
}

void FormOptimization::on_comboUntil_activated(const QString &arg1)
{
  if ((arg1 == "Real time >") || (arg1 == "Live time >"))
  {
    ui->doubleCriterion->setVisible(false);
    ui->timeDuration->setVisible(true);
  }
  else if (arg1 == "Peak area % error <")
  {
    ui->doubleCriterion->setVisible(true);
    ui->timeDuration->setVisible(false);
    ui->doubleCriterion->setRange(0.01, 99.99);
    ui->doubleCriterion->setSingleStep(0.1);
    ui->doubleCriterion->setDecimals(2);
  }
  else if (arg1 == "Total count >")
  {
    ui->doubleCriterion->setVisible(true);
    ui->timeDuration->setVisible(false);
    ui->doubleCriterion->setRange(1, 10000000);
    ui->doubleCriterion->setSingleStep(100);
    ui->doubleCriterion->setDecimals(0);
  }
  else
  {
    ui->doubleCriterion->setVisible(false);
    ui->timeDuration->setVisible(false);
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
