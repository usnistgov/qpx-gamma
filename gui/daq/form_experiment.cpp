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
 *      FormExperiment -
 *
 ******************************************************************************/

#include "form_experiment.h"
#include "widget_detectors.h"
#include "ui_form_experiment.h"
#include "fitter.h"
#include "qt_util.h"
#include <QInputDialog>
#include <QSettings>


using namespace Qpx;

FormExperiment::FormExperiment(ThreadRunner& runner, XMLableDB<Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormExperiment),
  runner_thread_(runner),
  interruptor_(false),
  my_run_(false),
  continue_(false),
  selected_sink_(-1),
  detectors_(newDetDB)
{
  ui->setupUi(this);
  this->setWindowTitle("Experiment");

  loadSettings();

  connect(&runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));
  connect(&exp_plot_thread_, SIGNAL(plot_ready()), this, SLOT(new_daq_data()));

  form_experiment_setup_ = new FormExperimentSetup(detectors_, exp_project_);
  ui->tabs->addTab(form_experiment_setup_, "Setup");
  connect(form_experiment_setup_, SIGNAL(selectedProject(int64_t)), this, SLOT(selectProject(int64_t)));
  connect(form_experiment_setup_, SIGNAL(selectedSink(int64_t)), this, SLOT(selectSink(int64_t)));

  form_experiment_1d_ = new FormExperiment1D(detectors_, exp_project_);
  ui->tabs->addTab(form_experiment_1d_, "Results in 1D");

  ui->plotSpectrum->setFit(&selected_fitter_);
  connect(ui->plotSpectrum, SIGNAL(data_changed()), this, SLOT(update_fits()));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(fitting_done()), this, SLOT(fitting_done()));
  connect(ui->plotSpectrum, SIGNAL(fitter_busy(bool)), this, SLOT(fitter_status(bool)));


  ui->tabs->setCurrentWidget(form_experiment_setup_);
  form_experiment_setup_->update_exp_project();
  form_experiment_1d_->update_exp_project();
}

FormExperiment::~FormExperiment()
{
  delete ui;
}

void FormExperiment::closeEvent(QCloseEvent *event) {
  if (my_run_ && runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      continue_ = false;
      runner_thread_.terminate();
      runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  exp_plot_thread_.terminate_wait();

  saveSettings();
  event->accept();
}

void FormExperiment::loadSettings() {
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

  settings_.beginGroup("Experiment");
  //  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  //  ui->comboCodomain->setCurrentText(settings_.value("co-domain", ui->comboCodomain->currentText()).toString());
  //  ui->checkAutofit->setChecked(settings_.value("autofit", true).toBool());
  //  ui->widgetAutoselect->loadSettings(settings_);

  ui->plotSpectrum->loadSettings(settings_);

  settings_.endGroup();
}

void FormExperiment::saveSettings() {
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

  settings_.beginGroup("Experiment");
  //  settings_.setValue("bits", ui->spinBits->value());
  //  settings_.setValue("co-domain", ui->comboCodomain->currentText());
  //  settings_.setValue("autofit", ui->checkAutofit->isChecked());
  //  ui->widgetAutoselect->saveSettings(settings_);

  ui->plotSpectrum->saveSettings(settings_);

  settings_.endGroup();
}

void FormExperiment::save_report()
{
  QString fileName = CustomSaveFileDialog(this, "Save analysis report",
                                          data_directory_, "Plain text (*.txt)");
  if (validateFile(this, fileName, true)) {
    INFO << "Writing report to " << fileName.toStdString();
    selected_fitter_.save_report(fileName.toStdString());
  }
}


void FormExperiment::run_completed() {
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

void FormExperiment::new_daq_data() {
  update_name();

  if (ui->plotSpectrum->busy())
    return;

  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  if (!sink)
  {
    FitSettings fitset = selected_fitter_.settings();
    selected_fitter_ = Qpx::Fitter();
    selected_fitter_.apply_settings(fitset);
    ui->plotSpectrum->updateData();
    return;
  }

  Qpx::Metadata md = sink->metadata();

  if (p->has_fitter(selected_sink_))
  {
    selected_fitter_ = p->get_fitter(selected_sink_);
    if (selected_fitter_.metadata_.total_count < md.total_count)
      selected_fitter_.setData(sink);
  }
  else
  {
    FitSettings fitset = selected_fitter_.settings();
    selected_fitter_ = Qpx::Fitter();
    selected_fitter_.setData(sink);
    selected_fitter_.apply_settings(fitset);
  }

  ui->plotSpectrum->updateData();

  std::set<double> sel = selected_fitter_.get_selected_peaks();
  if (!sel.empty())
    ui->plotSpectrum->set_selected_peaks(sel);

  if (!selected_fitter_.peak_count())
    ui->plotSpectrum->perform_fit();

}

void FormExperiment::update_name()
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

void FormExperiment::start_new_pass()
{
  std::pair<Qpx::ProjectPtr, uint64_t> pr = get_next_point();

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

  interruptor_.store(false);
  runner_thread_.do_run(pr.first, interruptor_, pr.second);
}

std::pair<Qpx::ProjectPtr, uint64_t> FormExperiment::get_next_point()
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

  form_experiment_setup_->retro_push(ret.second);

  if (ret.second->data_idx >= 0)
    return std::pair<Qpx::ProjectPtr, uint64_t>(exp_project_.get_data(ret.second->data_idx),
                                                ret.second->domain.criterion);
  else
    return get_next_point();
}


void FormExperiment::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);
  bool empty = true; //filtered_data_points_.empty();
  bool enable_domain_edit = (enable && !my_run_ && empty);

  ui->pushStart->setEnabled(online && enable_domain_edit);

  //  ui->comboTarget->setEnabled(enable_domain_edit);
  //  ui->spinBits->setEnabled(enable_domain_edit);
  //  ui->comboSinkType->setEnabled(enable_domain_edit);
  //  ui->pushEditPrototype->setEnabled(enable_domain_edit);

  //  ui->comboCodomain->setEnabled(enable);

  //  ui->pushClear->setEnabled(enable && !my_run_ && !empty);
  //  ui->pushSaveCsv->setEnabled(enable && !empty);
}


void FormExperiment::update_fits() {

  update_peak_selection(std::set<double>());
}

void FormExperiment::fitting_done()
{
  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  if (!sink)
    return;

  Qpx::Metadata md = sink->metadata();

  selected_fitter_.set_selected_peaks(ui->plotSpectrum->get_selected_peaks());
  p->update_fitter(selected_sink_, selected_fitter_);

  exp_project_.gather_results();
  form_experiment_1d_->update_exp_project();
}


void FormExperiment::update_peak_selection(std::set<double> dummy) {
  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  if (!sink)
    return;


  Qpx::Metadata md = sink->metadata();

  selected_fitter_.set_selected_peaks(ui->plotSpectrum->get_selected_peaks());
  p->update_fitter(selected_sink_, selected_fitter_);

  exp_project_.gather_results();
  form_experiment_1d_->update_exp_project();
}


void FormExperiment::fitter_status(bool busy)
{
  //  ui->tableResults->setEnabled(!busy);
  //  ui->PlotCalib->setEnabled(!busy);
  //  ui->treeViewExperiment->setEnabled(!busy);
}

void FormExperiment::selectProject(int64_t idx)
{
  Qpx::ProjectPtr p = exp_project_.get_data(idx);
  if (!p)
    return;

  exp_plot_thread_.monitor_source(p);
  p->activate();
}

void FormExperiment::selectSink(int64_t idx)
{
  selected_sink_ = idx;
  new_daq_data();
}

void FormExperiment::on_pushNewExperiment_clicked()
{
  selected_sink_ = -1;
  new_daq_data();
  exp_plot_thread_.terminate_wait();

  exp_project_ = Qpx::ExperimentProject();
  form_experiment_setup_->update_exp_project();

  exp_project_.gather_results();
  form_experiment_1d_->update_exp_project();
}

void FormExperiment::on_pushLoadExperiment_clicked()
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

      selected_sink_ = -1;
      new_daq_data();
      exp_plot_thread_.terminate_wait();

      form_experiment_setup_->update_exp_project();

      exp_project_.gather_results();
      form_experiment_1d_->update_exp_project();
    }
  }
}

void FormExperiment::on_pushSaveExperiment_clicked()
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

void FormExperiment::on_pushStart_clicked()
{
  form_experiment_1d_->update_exp_project();
  FitSettings fitset = selected_fitter_.settings();
  selected_fitter_ = Qpx::Fitter();
  selected_fitter_.apply_settings(fitset);

  continue_ = true;
  start_new_pass();
}

void FormExperiment::on_pushStop_clicked()
{
  continue_ = false;
  interruptor_.store(true);
}
