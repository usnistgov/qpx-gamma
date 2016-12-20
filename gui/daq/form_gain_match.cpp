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
 *      Gain matching interface
 *
 ******************************************************************************/

#include "form_gain_match.h"
#include "ui_form_gain_match.h"
#include "custom_logger.h"
#include "fityk.h"
#include "daq_sink_factory.h"
#include <QSettings>

#include "UncertainDouble.h"
#include "qt_util.h"
#include "dialog_spectrum.h"


using namespace Qpx;

FormGainMatch::FormGainMatch(ThreadRunner& thread, XMLableDB<Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGainMatch),
  gm_runner_thread_(thread),
  detectors_(detectors),
  project_(new Qpx::Project()),
  interruptor_(false),
  my_run_(false)
{
  ui->setupUi(this);
  this->setWindowTitle("Gain matching");

  connect(&gm_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));
  connect(&gm_plot_thread_, SIGNAL(plot_ready()), this, SLOT(new_daq_data()));

  current_pass_ = 0;
  selected_pass_ = -1;

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 10);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 10);
  style_fit.default_pen = QPen(Qt::darkCyan, 2);


  ui->plotRef->setFit(&fitter_ref_);
  connect(ui->plotRef, SIGNAL(data_changed()), this, SLOT(update_fit_ref()));
  connect(ui->plotRef, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotRef, SIGNAL(fitting_done()), this, SLOT(fitting_done_ref()));
  //  connect(ui->plotRef, SIGNAL(fitter_busy(bool)), this, SLOT(fitter_status(bool)));

  ui->plotOpt->setFit(&fitter_opt_);
  connect(ui->plotOpt, SIGNAL(data_changed()), this, SLOT(update_fit_opt()));
  connect(ui->plotOpt, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotOpt, SIGNAL(fitting_done()), this, SLOT(fitting_done_opt()));
  connect(ui->plotOpt, SIGNAL(fitter_busy(bool)), this, SLOT(fitter_status(bool)));

  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(pass_selected_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(pass_selected_in_plot()));


  loadSettings();
  update_settings();
  init_prototypes();

  gm_plot_thread_.monitor_source(project_);
  //  gm_plot_thread_.start();
}

FormGainMatch::~FormGainMatch()
{
  delete ui;
}

void FormGainMatch::update_settings() {
  ui->comboReference->clear();
  ui->comboTarget->clear();

  //should come from other thread?
  current_dets_ = Qpx::Engine::getInstance().get_detectors();

  for (size_t i=0; i < current_dets_.size(); ++i) {
    QString text = "[" + QString::number(i) + "] "
        + QString::fromStdString(current_dets_[i].name_);
    ui->comboReference->addItem(text, QVariant::fromValue(i));
    ui->comboTarget->addItem(text, QVariant::fromValue(i));
  }

  remake_source_domains();
  remake_domains();
}

void FormGainMatch::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    Qpx::Setting manset("manual_gain_settings");
    manset.metadata.setting_type = Qpx::SettingType::stem;

    std::string path = profile_directory.toStdString() + "/gainmatch.set";
    pugi::xml_document doc;
    if (doc.load_file(path.c_str())) {
      pugi::xml_node root = doc.child("GainMatch");
      if (root.child(manset.xml_element_name().c_str()))
        manset.from_xml(root.child(manset.xml_element_name().c_str()));
      FitSettings fs;
      if (root.child(fs.xml_element_name().c_str())) {
        fs.from_xml(root.child(fs.xml_element_name().c_str()));
        fitter_ref_.apply_settings(fs);
        fitter_opt_.apply_settings(fs);
      }
    }

    manual_settings_.clear();
    for (auto &s : manset.branches.my_data_)
      manual_settings_["[manual] " + s.id_] = s;
  }


  settings_.beginGroup("GainMatching");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  //  ui->spinRefChan->setValue(settings_.value("reference_channel", 0).toInt());
  //  ui->spinOptChan->setValue(settings_.value("optimized_channel", 1).toInt());
  ui->doubleError->setValue(settings_.value("error", 1.0).toDouble());
  ui->doubleThreshold->setValue(settings_.value("threshold", 1.0).toDouble());
  ui->widgetAutoselect->loadSettings(settings_);

  ui->plotRef->loadSettings(settings_);
  ui->plotOpt->loadSettings(settings_);

  settings_.endGroup();
}

void FormGainMatch::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    std::string path = profile_directory.toStdString() + "/gainmatch.set";
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    pugi::xml_node node = root.append_child("GainMatch");
    if (!manual_settings_.empty()) {
      Qpx::Setting manset("manual_gain_settings");
      manset.metadata.setting_type = Qpx::SettingType::stem;
      for (auto &q : manual_settings_)
        manset.branches.add_a(q.second);
      manset.to_xml(node);
    }
    fitter_opt_.settings().to_xml(node); //ref separately?
    doc.save_file(path.c_str());
  }

  settings_.beginGroup("GainMatching");
  settings_.setValue("bits", ui->spinBits->value());
  //  settings_.setValue("reference_channel", ui->spinRefChan->value());
  //  settings_.setValue("optimized_channel", ui->spinOptChan->value());
  settings_.setValue("error", ui->doubleError->value());
  settings_.setValue("threshold", ui->doubleThreshold->value());
  ui->widgetAutoselect->saveSettings(settings_);

  ui->plotRef->saveSettings(settings_);
  ui->plotOpt->saveSettings(settings_);

  settings_.endGroup();
}

void FormGainMatch::closeEvent(QCloseEvent *event) {
  if (my_run_ && gm_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      gm_runner_thread_.terminate();
      gm_runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  if (!project_->empty()) {
    int reply = QMessageBox::warning(this, "Gain matching data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      project_->clear();
    } else {
      event->ignore();
      return;
    }
  }

  //  project_->terminate();
  gm_plot_thread_.terminate_wait();
  saveSettings();

  event->accept();
}

void FormGainMatch::toggle_push(bool enable, SourceStatus status) {
  bool online = (status & SourceStatus::can_run);
  ui->pushStart->setEnabled(enable && online && !my_run_);

  ui->comboReference->setEnabled(enable && !my_run_);
  ui->comboTarget->setEnabled(enable && !my_run_);

  ui->doubleSpinStart->setEnabled(enable && !my_run_);
  ui->doubleSpinDeltaV->setEnabled(enable && !my_run_);

  ui->comboSetting->setEnabled(enable && !my_run_);
  ui->spinBits->setEnabled(enable && !my_run_);
  ui->pushAddCustom->setEnabled(enable && !my_run_);

  ui->pushEditPrototypeRef->setEnabled(enable && !my_run_);
  ui->pushEditPrototypeOpt->setEnabled(enable && !my_run_);

}

void FormGainMatch::init_prototypes()
{
  sink_prototype_ref_ = make_prototype(ui->spinBits->value(),
                                       ui->comboReference->currentData().toInt(),
                                       "Reference");

  sink_prototype_opt_ = make_prototype(ui->spinBits->value(),
                                       ui->comboReference->currentData().toInt(),
                                       "Optimizing");

  remake_source_domains();
  remake_domains();
  on_comboSetting_activated(0);
}

Qpx::Metadata FormGainMatch::make_prototype(uint16_t bits, uint16_t channel, std::string name)
{
  Qpx::Metadata    spectrum_prototype;
  spectrum_prototype = Qpx::SinkFactory::getInstance().create_prototype("1D");
  Setting nm = spectrum_prototype.get_attribute("name");
  nm.value_text = name;
  spectrum_prototype.set_attribute(nm);

  Setting res = spectrum_prototype.get_attribute("resolution");
  res.value_int = bits;
  spectrum_prototype.set_attribute(res);

  Qpx::Setting vis = spectrum_prototype.get_attribute("visible");
  vis.value_int = true;
  spectrum_prototype.set_attribute(vis);


  Qpx::Setting app = spectrum_prototype.get_attribute("appearance");
  QColor col;
  col.setHsv(QColor(Qt::blue).hsvHue(), 48, 160);
  app.value_text = col.name().toStdString();
  spectrum_prototype.set_attribute(app);

  std::vector<bool> gates(channel + 1, false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = spectrum_prototype.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  spectrum_prototype.set_attribute(pattern);
  pattern = spectrum_prototype.get_attribute("pattern_add");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  spectrum_prototype.set_attribute(pattern);

  return spectrum_prototype;
}

void FormGainMatch::start_new_pass()
{
  if (manual_settings_.count(ui->comboSetting->currentText().toStdString()))
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
    {
      update_name();
      return;
    }
  }
  else if (source_settings_.count(ui->comboSetting->currentText().toStdString()))
  {
    Qpx::Engine::getInstance().set_setting(current_setting_, Qpx::Match::id | Qpx::Match::indices);
    QThread::sleep(1);
    Qpx::Engine::getInstance().get_all_settings();
    current_setting_.value_dbl = Qpx::Engine::getInstance().pull_settings().get_setting(current_setting_, Qpx::Match::id | Qpx::Match::indices).value_dbl;
  }
  else
  {
    update_name();
    return;
  }

  ui->pushStop->setEnabled(true);
  my_run_ = true;
  emit toggleIO(false);

  LINFO << "<FormGainMatch> Starting pass #" << (current_pass_ + 1)
        << " with " << ui->comboSetting->currentText().toStdString() << " = "
        << current_setting_.value_dbl;

  Qpx::Setting set_pass("Pass");
  set_pass.value_int = current_pass_;
  sink_prototype_opt_.set_attribute(set_pass);
  sink_prototype_opt_.set_attribute(current_setting_);

  if (project_->empty()) {
    project_->add_sink(sink_prototype_ref_);
  } else {
    std::map<int64_t, SinkPtr> sinks = project_->get_sinks();
    for (auto &q : sinks)
      if (q.second->metadata().get_attribute("name").value_text == "Optimizing")
        project_->delete_sink(q.first);
  }

  project_->add_sink(sink_prototype_opt_);

  DataPoint newdata;
  newdata.spectrum.apply_settings(fitter_opt_.settings());
  newdata.independent_variable = current_setting_.value_dbl;
  newdata.dependent_variable = std::numeric_limits<double>::quiet_NaN();

  experiment_.push_back(newdata);
  display_data(); //before new data arrives?

  emit settings_changed();

  //  gm_plot_thread_.start();
  interruptor_.store(false);
  gm_runner_thread_.do_run(project_, interruptor_, 0);

}

void FormGainMatch::run_completed() {
  if (my_run_) {
    //    project_->terminate();
    gm_plot_thread_.terminate_wait();

    ui->pushStop->setEnabled(false);
    my_run_ = false;
    emit toggleIO(true);

    if (current_pass_ >= 0) {
      LINFO << "<FormGainMatch> Completed pass # " << (current_pass_ + 1);
      do_post_processing();
    }
  }
}

void FormGainMatch::do_post_processing() {

  //  new_daq_data();

  std::vector<double> gains;
  std::vector<double> positions;
  std::vector<double> position_sigmas;

  double latest_position = std::numeric_limits<double>::quiet_NaN();

  for (auto &q : experiment_)
  {
    if (!std::isnan(q.dependent_variable)) {
      gains.push_back(q.independent_variable);
      positions.push_back(q.dependent_variable);
      position_sigmas.push_back(q.dep_uncert);

      latest_position = q.dependent_variable;
    }
  }

  DBG << "<FormGainMatch> Valid data points " << gains.size() << " latest=" << latest_position
      << " refgood=" << (peak_ref_ != Qpx::Peak());

  if (!std::isnan(latest_position) && (peak_ref_ != Qpx::Peak())
      && (std::abs(latest_position - peak_ref_.center().value()) < ui->doubleThreshold->value()))
  {
    LINFO << "<FormGainMatch> Gain matching complete   |"
          << latest_position << " - " << peak_ref_.center().value()
          << "| < " << ui->doubleThreshold->value();
    update_name();
    emit optimization_complete();
    return;
  }

  double predicted = std::numeric_limits<double>::quiet_NaN();

  response_function_ = PolyBounded();
  response_function_.add_coeff(0, -50, 50, 1);
  response_function_.add_coeff(1, -50, 50, 1);
  if (gains.size() > 2)
    response_function_.add_coeff(2, -50, 50, 1);
  response_function_.fit_fityk(gains, positions, position_sigmas);
  predicted = response_function_.eval_inverse(peak_ref_.center().value() /*, ui->doubleThreshold->value() / 4.0*/);

  DBG << "<FormGainMatch> Prediction " << predicted;

  if ((gains.size() < 2) || std::isnan(predicted)){
    DBG << "<FormGainMatch> not enough data points or bad prediction";
    if (!std::isnan(latest_position) && (peak_ref_ != Qpx::Peak())
        && (latest_position > peak_ref_.center().value()))
      current_setting_.value_dbl -= current_setting_.metadata.step;
    else
      current_setting_.value_dbl += current_setting_.metadata.step;
  }
  else
  {
    current_setting_.value_dbl = predicted;
  }

  DBG << "<FormGainMatch> new value = " << current_setting_.value_dbl;

  current_pass_++;
  //  QThread::sleep(2);

  start_new_pass();
}

void FormGainMatch::update_fit_ref()
{
  ui->plotRef->set_selected_peaks(ui->widgetAutoselect->reselect(fitter_ref_.peaks(), ui->plotRef->get_selected_peaks()));
  //  if ((selected_pass_ >= 0) && (selected_pass_ < experiment_.size()))
  //    experiment_[selected_pass_].spectrum = selected_fitter_;
  update_peak_selection(std::set<double>());
}

void FormGainMatch::update_fit_opt()
{
  ui->plotOpt->set_selected_peaks(ui->widgetAutoselect->reselect(fitter_opt_.peaks(), ui->plotOpt->get_selected_peaks()));
  if ((selected_pass_ >= 0) && (selected_pass_ < static_cast<int>(experiment_.size())))
    experiment_[selected_pass_].spectrum = fitter_opt_;
  update_peak_selection(std::set<double>());
}

void FormGainMatch::update_peak_selection(std::set<double> dummy) {
  std::set<double> selected_peaks = ui->plotRef->get_selected_peaks();
  if ((selected_peaks.size() != 1) || !fitter_ref_.peaks().count(*selected_peaks.begin()))
    peak_ref_ = Qpx::Peak();
  else
    peak_ref_ = fitter_ref_.peaks().at(*selected_peaks.begin());

  Qpx::Metadata md = fitter_opt_.metadata_;
  Qpx::Setting pass = md.get_attribute("Pass");
  if (!pass || (pass.value_int < 0) || (pass.value_int >= static_cast<int>(experiment_.size())))
    return;

  selected_peaks = ui->plotOpt->get_selected_peaks();
  if ((selected_peaks.size() != 1) || !fitter_opt_.peaks().count(*selected_peaks.begin()))
    experiment_[pass.value_int].selected_peak = Qpx::Peak();
  else
    experiment_[pass.value_int].selected_peak = fitter_opt_.peaks().at(*selected_peaks.begin());


  eval_dependent(experiment_[pass.value_int]);
  display_data();

  if ((pass.value_int == current_pass_) && criterion_satisfied(experiment_[pass.value_int]))
    interruptor_.store(true);

  //  if ((pass == current_pass_) && (peaks_.back().sum4_.peak_area.err() < ui->doubleError->value()))
  //    interruptor_.store(true);

}


void FormGainMatch::new_daq_data() {
  for (auto &q: project_->get_sinks()) {
    Metadata md;
    if (q.second)
      md = q.second->metadata();

    if (md.get_attribute("name").value_text == "Reference") {
      fitter_ref_.setData(q.second); //not busy, etc...?
      if (!ui->plotRef->busy())
        ui->plotRef->perform_fit();
    }
    else
    {
      Qpx::Setting pass = md.get_attribute("Pass");
      if (pass && (pass.value_int < static_cast<int>(experiment_.size()))) {
        experiment_[pass.value_int].spectrum.setData(q.second);
        double variable = md.get_attribute(current_setting_).value_dbl;
        experiment_[pass.value_int].independent_variable = variable;
        eval_dependent(experiment_[pass.value_int]);
        display_data();
        if (pass.value_int == current_pass_) {
          if (criterion_satisfied(experiment_[pass.value_int]))
            interruptor_.store(true);
          if (current_pass_ == selected_pass_)
          {
            pass_selected_in_table();
            if (!ui->plotOpt->busy())
              ui->plotOpt->perform_fit();
          }
        }
      }
    }
  }

}

void FormGainMatch::on_pushStart_clicked()
{
  response_function_ = PolyBounded();
  project_->clear();

  peak_ref_ = Peak();
  experiment_.clear();
  FitSettings fitset = fitter_ref_.settings();
  fitter_ref_ = Qpx::Fitter();
  fitter_ref_.apply_settings(fitset);

  fitset = fitter_opt_.settings();
  fitter_opt_ = Qpx::Fitter();
  fitter_opt_.apply_settings(fitset);


  if (all_settings_.count(ui->comboSetting->currentText().toStdString())) {
    current_setting_ = all_settings_.at(ui->comboSetting->currentText().toStdString());

    current_setting_.value_dbl = ui->doubleSpinStart->value();
    current_setting_.metadata.step = ui->doubleSpinDeltaV->value();

    current_pass_ = 0;
    selected_pass_ = -1;

    display_data();
    start_new_pass();
  }
}

void FormGainMatch::on_pushStop_clicked()
{
  current_pass_ = -1;
  interruptor_.store(true);
}

void FormGainMatch::on_comboReference_currentIndexChanged(int index)
{
  uint16_t channel = ui->comboReference->currentData().toInt();
  sink_prototype_ref_.set_det_limit(current_dets_.size());

  std::vector<bool> gates(current_dets_.size(), false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = sink_prototype_ref_.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_ref_.set_attribute(pattern);
  pattern = sink_prototype_ref_.get_attribute("pattern_add");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_ref_.set_attribute(pattern);
}

void FormGainMatch::on_comboTarget_currentIndexChanged(int index)
{
  uint16_t channel = ui->comboTarget->currentData().toInt();
  sink_prototype_opt_.set_det_limit(current_dets_.size());

  std::vector<bool> gates(current_dets_.size(), false);
  gates[channel] = true;

  Qpx::Setting pattern;
  pattern = sink_prototype_opt_.get_attribute("pattern_coinc");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_opt_.set_attribute(pattern);
  pattern = sink_prototype_opt_.get_attribute("pattern_add");
  pattern.value_pattern.set_gates(gates);
  pattern.value_pattern.set_theshold(1);
  sink_prototype_opt_.set_attribute(pattern);

  remake_source_domains();
  remake_domains();
  on_comboSetting_activated(0);
}

void FormGainMatch::on_comboSetting_activated(int index)
{
  current_setting_ = Qpx::Setting();
  if (all_settings_.count(ui->comboSetting->currentText().toStdString()))
    current_setting_ = all_settings_.at( ui->comboSetting->currentText().toStdString() );

  ui->doubleSpinStart->setRange(current_setting_.metadata.minimum, current_setting_.metadata.maximum);
  ui->doubleSpinStart->setSingleStep(current_setting_.metadata.step);
  ui->doubleSpinStart->setValue(current_setting_.value_dbl);
  ui->doubleSpinStart->setSuffix(" " + QString::fromStdString(current_setting_.metadata.unit));

  ui->doubleSpinDeltaV->setRange(current_setting_.metadata.step, current_setting_.metadata.maximum - current_setting_.metadata.minimum);
  ui->doubleSpinDeltaV->setSingleStep(current_setting_.metadata.step);
  ui->doubleSpinDeltaV->setValue(current_setting_.metadata.step  * 20);
  ui->doubleSpinDeltaV->setSuffix(" " + QString::fromStdString(current_setting_.metadata.unit));

  bool manual = manual_settings_.count(ui->comboSetting->currentText().toStdString());
  ui->pushEditCustom->setVisible(manual);
  ui->pushDeleteCustom->setVisible(manual);
}

void FormGainMatch::display_data()
{
  int old_row_count = ui->tableResults->rowCount();

  ui->tableResults->blockSignals(true);

  if (ui->tableResults->rowCount() != static_cast<int>(experiment_.size()))
    ui->tableResults->setRowCount(experiment_.size());

  ui->tableResults->setColumnCount(5);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_.metadata.name), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Bin", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("%error", QTableWidgetItem::Type));

  QVector<double> xx, yy, yy_sigma;

  for (size_t i = 0; i < experiment_.size(); ++i)
  {
    const DataPoint &data = experiment_.at(i);

    add_to_table(ui->tableResults, i, 0, std::to_string(data.independent_variable));
    add_to_table(ui->tableResults, i, 1, data.selected_peak.center().to_string());
    add_to_table(ui->tableResults, i, 2, data.selected_peak.fwhm().to_string());
    add_to_table(ui->tableResults, i, 3, data.selected_peak.area_best().to_string());
    add_to_table(ui->tableResults, i, 4, data.selected_peak.area_best().error_percent());

    if (!std::isnan(data.dependent_variable)) {
      xx.push_back(data.independent_variable);
      yy.push_back(data.dependent_variable);
      if (!std::isnan(data.dep_uncert) && !std::isinf(data.dep_uncert))
        yy_sigma.push_back(data.dep_uncert);
      else
        yy_sigma.push_back(0);
    }
  }

  ui->tableResults->blockSignals(false);

  ui->PlotCalib->clearGraphs();
  if (!xx.isEmpty()) {

    double xmin = std::numeric_limits<double>::max();
    double xmax = - std::numeric_limits<double>::max();

    for (auto &q : xx) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }

    double x_margin = (xmax - xmin) / 10;
    xmax += x_margin;
    xmin -= x_margin;


    ui->PlotCalib->addPoints(style_pts, xx, yy, QVector<double>(), yy_sigma);

    xx.clear();
    yy.clear();


    double step = (xmax-xmin) / 50.0;

    for (double i=xmin; i < xmax; i+=step) {
      xx.push_back(i);
      yy.push_back(response_function_.eval(i));
    }

    std::set<double> chosen_peaks_chan;
    //if selected insert;

    //    if (response_function_ != PolyBounded())
    ui->PlotCalib->setFit(xx, yy, style_fit);

    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    ui->PlotCalib->setAxisLabels(QString::fromStdString(current_setting_.metadata.name),
                             "centroid");
  }
  //  ui->PlotCalib->redraw();

  if (experiment_.size() && (static_cast<int>(experiment_.size()) == (old_row_count + 1))) {
    ui->tableResults->selectRow(old_row_count);
    pass_selected_in_table();
  } else if ((selected_pass_ >= 0) && (selected_pass_ < static_cast<int>(experiment_.size()))) {
    std::set<double> sel;
    sel.insert(experiment_.at(selected_pass_).independent_variable);
    ui->PlotCalib->set_selected_pts(sel);
    ui->PlotCalib->replotAll();
  }
  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    ui->PlotCalib->replotAll();
  }

}

void FormGainMatch::pass_selected_in_table()
{
  selected_pass_ = -1;
  foreach (QModelIndex i, ui->tableResults->selectionModel()->selectedRows())
    selected_pass_ = i.row();
  FitSettings fitset = fitter_opt_.settings();
  if ((selected_pass_ >= 0) && (selected_pass_ < static_cast<int>(experiment_.size()))) {
    fitter_opt_ = experiment_.at(selected_pass_).spectrum;
    std::set<double> sel;
    sel.insert(experiment_.at(selected_pass_).independent_variable);
    ui->PlotCalib->set_selected_pts(sel);
    ui->PlotCalib->replotAll();
  }
  else
  {
    ui->PlotCalib->set_selected_pts(std::set<double>());
    fitter_opt_ = Qpx::Fitter();
    ui->PlotCalib->replotAll();
  }
  fitter_opt_.apply_settings(fitset);


  if (!ui->plotOpt->busy() && (selected_pass_ >= 0) && (selected_pass_ < static_cast<int>(experiment_.size()))) {
    //    DBG << "fitter not busy";
    ui->plotOpt->updateData();
    if (experiment_.at(selected_pass_).selected_peak != Qpx::Peak()) {
      std::set<double> selected;
      selected.insert(experiment_.at(selected_pass_).selected_peak.center().value());
      ui->plotOpt->set_selected_peaks(selected);
    }
    if ((fitter_opt_.metadata_.get_attribute("total_hits").value_precise > 0)
        && fitter_opt_.peaks().empty()
        && !ui->plotOpt->busy())
      ui->plotOpt->perform_fit();
  }
}

void FormGainMatch::pass_selected_in_plot()
{
  //allow only one point to be selected!!!
  std::set<double> selection = ui->PlotCalib->get_selected_pts();
  if (selection.size() < 1)
  {
    ui->tableResults->clearSelection();
    return;
  }

  double sel = *selection.begin();

  for (size_t i=0; i < experiment_.size(); ++i)
  {
    if (experiment_.at(i).independent_variable == sel) {
      ui->tableResults->selectRow(i);
      pass_selected_in_table();
      return;
    }
  }
}

void FormGainMatch::remake_source_domains()
{
  source_settings_.clear();
  if (!ui->comboTarget->count())
    return;

  int chan = ui->comboTarget->currentData().toInt();

  if ((chan < 0) || (chan >= static_cast<int>(current_dets_.size())))
    return;


  for (auto &q : current_dets_[chan].settings_.branches.my_data_)
    if (q.metadata.flags.count("gain") > 0)
      source_settings_["[DAQ] " + q.id_
          + " (" + std::to_string(chan) + ":" + current_dets_[chan].name_ + ")"] = q;

}


void FormGainMatch::remake_domains()
{

  all_settings_.clear();

  for (auto &s : source_settings_)
  {
    if (!s.second.metadata.writable || !s.second.metadata.visible)
      continue;

    if (s.second.metadata.setting_type == Qpx::SettingType::floating)
      all_settings_[s.first] = s.second;
    //also allow for integer, floating_precise?
  }

  for (auto &s : manual_settings_)
  {
    if (s.second.metadata.setting_type == Qpx::SettingType::floating)
      all_settings_[s.first] = s.second;
  }

  ui->comboSetting->clear();
  for (auto &q : all_settings_)
    ui->comboSetting->addItem(QString::fromStdString(q.first));
}

void FormGainMatch::fitter_status(bool busy)
{
  ui->tableResults->setEnabled(!busy);
  ui->PlotCalib->setEnabled(!busy);
}

void FormGainMatch::fitting_done_ref()
{
  if (!fitter_ref_.peaks().empty() && (peak_ref_ == Qpx::Peak())) {
    //      DBG << "<FormOptimization> Autoselecting";
    ui->plotRef->set_selected_peaks(ui->widgetAutoselect->autoselect(fitter_ref_.peaks(), ui->plotRef->get_selected_peaks()));
    update_peak_selection(std::set<double>());
  }
}


void FormGainMatch::fitting_done_opt()
{
  if ((selected_pass_ >= 0) && (selected_pass_ < static_cast<int>(experiment_.size()))) {
    experiment_[selected_pass_].spectrum = fitter_opt_;
    if (!fitter_opt_.peaks().empty() && (experiment_.at(selected_pass_).selected_peak == Qpx::Peak())) {
      //      DBG << "<FormOptimization> Autoselecting";
      ui->plotOpt->set_selected_peaks(ui->widgetAutoselect->autoselect(fitter_opt_.peaks(), ui->plotOpt->get_selected_peaks()));
      update_peak_selection(std::set<double>());
    }
  }
}


void FormGainMatch::eval_dependent(DataPoint &data)
{
  if (data.selected_peak != Qpx::Peak()) {
    data.dependent_variable = data.selected_peak.center().value(); //with uncert?
    data.dep_uncert = data.selected_peak.center().uncertainty(); //with uncert?
  } else {
    data.dependent_variable = std::numeric_limits<double>::quiet_NaN();
    data.dep_uncert = std::numeric_limits<double>::quiet_NaN();
  }
}

bool FormGainMatch::criterion_satisfied(DataPoint &data)
{
  return ((data.selected_peak != Qpx::Peak())
          && (data.selected_peak.area_best().error() < ui->doubleError->value()));
}

void FormGainMatch::update_name()
{
  QString name = "Gain matching";
  if (my_run_)
    name += QString::fromUtf8("  \u25b6");
  //  else if (spectra_.changed())
  //    name += QString::fromUtf8(" \u2731");

  if (name != this->windowTitle()) {
    this->setWindowTitle(name);
    emit toggleIO(true);
  }
}


void FormGainMatch::on_pushAddCustom_clicked()
{
  bool ok;
  QString name = QInputDialog::getText(this, "New custom variable",
                                       "Name:", QLineEdit::Normal, "", &ok);
  if (ok) {
    for (auto &s : manual_settings_)
      if (s.second.id_ == name.toStdString())
      {
        QMessageBox::information(this, "Already exists",
                                 "Manual setting \'" + name + "\'' already exists");
        return;
      }

    Qpx::Setting newset(name.toStdString());
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


    manual_settings_["[manual] " + newset.id_] = newset;
    remake_domains();
  }
}

void FormGainMatch::on_pushEditCustom_clicked()
{
  QString name = ui->comboSetting->currentText();
  if (!manual_settings_.count(name.toStdString()))
    return;
  Qpx::Setting newset = manual_settings_.at(name.toStdString());

  double d = 0;
  bool ok;

  d = QInputDialog::getDouble(this, "Minimum",
                              "Minimum value for " + name + " = ",
                              newset.metadata.minimum,
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
                              std::max(newset.metadata.maximum, newset.metadata.minimum),
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
                              newset.metadata.step,
                              0,
                              newset.metadata.maximum - newset.metadata.minimum,
                              0.001,
                              &ok);
  if (ok)
    newset.metadata.step = d;
  else
    return;

  manual_settings_[name.toStdString()] = newset;
  remake_domains();
  ui->comboSetting->setCurrentText(name);
  on_comboSetting_activated(0);
}

void FormGainMatch::on_pushDeleteCustom_clicked()
{
  QString name = ui->comboSetting->currentText();
  if (!manual_settings_.count(name.toStdString()))
    return;

  int ret = QMessageBox::question(this,
                                  "Delete setting?",
                                  "Are you sure you want to delete custom manual setting \'"
                                  + name + "\'?");

  if (!ret)
    return;

  manual_settings_.erase(name.toStdString());
  remake_domains();
  on_comboSetting_activated(0);
}



void FormGainMatch::on_pushEditPrototypeRef_clicked()
{
  DialogSpectrum* newDialog = new DialogSpectrum(sink_prototype_ref_, current_dets_, detectors_, false, false, this);
  if (newDialog->exec()) {
    sink_prototype_ref_ = newDialog->product();
    remake_source_domains();
    remake_domains();
  }
}

void FormGainMatch::on_pushEditPrototypeOpt_clicked()
{
  DialogSpectrum* newDialog = new DialogSpectrum(sink_prototype_opt_, current_dets_, detectors_, false, false, this);
  if (newDialog->exec()) {
    sink_prototype_opt_ = newDialog->product();
    remake_source_domains();
    remake_domains();
  }
}
