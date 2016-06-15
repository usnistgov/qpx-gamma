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

#include "form_daq_settings.h"
#include "dialog_spectrum.h"

using namespace Qpx;

FormExperiment::FormExperiment(ThreadRunner& runner, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormExperiment),
  runner_thread_(runner),
  interruptor_(false),
  my_run_(false),
  continue_(false),
  selected_sink_(-1)
{
  ui->setupUi(this);
  this->setWindowTitle("Experiment");

  loadSettings();

  ui->spectrumSelector->set_only_one(true);
  connect(ui->spectrumSelector, SIGNAL(itemSelected(SelectorItem)), this, SLOT(choose_spectrum(SelectorItem)));
  connect(ui->spectrumSelector, SIGNAL(itemDoubleclicked(SelectorItem)), this, SLOT(spectrumDoubleclicked(SelectorItem)));

  connect(&runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  connect(&exp_plot_thread_, SIGNAL(plot_ready()), this, SLOT(new_daq_data()));

  form_experiment_setup_ = new FormExperimentSetup(exp_project_);
  ui->tabs->addTab(form_experiment_setup_, "Setup");
  connect(form_experiment_setup_, SIGNAL(selectedProject(int64_t)), this, SLOT(selectProject(int64_t)));
  connect(form_experiment_setup_, SIGNAL(prototypesChanged()), this, SLOT(populate_selector()));
  connect(form_experiment_setup_, SIGNAL(toggleIO()), this, SLOT(toggle_from_setup()));

  connect(&runner_thread_, SIGNAL(settingsUpdated(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::SourceStatus)),
          form_experiment_setup_, SLOT(update_settings(Qpx::Setting,std::vector<Qpx::Detector>,Qpx::SourceStatus)));


  form_experiment_1d_ = new FormExperiment1D(exp_project_, data_directory_, selected_sink_);
  ui->tabs->addTab(form_experiment_1d_, "Results in 1D");

  form_experiment_2d_ = new FormExperiment2D(exp_project_, data_directory_, selected_sink_);
  ui->tabs->addTab(form_experiment_2d_, "Results in 2D");

  ui->plotSpectrum->setFit(&selected_fitter_);
  connect(ui->plotSpectrum, SIGNAL(data_changed()), this, SLOT(update_fits()));
  connect(ui->plotSpectrum, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(fitting_done()), this, SLOT(fitting_done()));
  connect(ui->plotSpectrum, SIGNAL(fitter_busy(bool)), this, SLOT(fitter_status(bool)));


  ui->tabs->setCurrentWidget(form_experiment_setup_);
  form_experiment_setup_->update_exp_project();
  form_experiment_1d_->update_exp_project();
  form_experiment_2d_->update_exp_project();

  runner_thread_.do_refresh_settings();
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

  if (exp_project_.changed()) {
    int reply = QMessageBox::warning(this, "Project contents changed",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
    {
      event->ignore();
      return;
    }
  }

  exp_plot_thread_.terminate_wait();
  saveSettings();
  event->accept();
}

void FormExperiment::loadSettings() {
  QSettings settings;

  settings.beginGroup("Program");
  QString profile_directory = settings.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings.endGroup();

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

  settings.beginGroup("Experiment");
  ui->checkAutofit->setChecked(settings.value("autofit", false).toBool());
  ui->plotSpectrum->loadSettings(settings);
  ui->widgetAutoselect->loadSettings(settings);
  settings.endGroup();
}

void FormExperiment::saveSettings() {
  QSettings settings;

  settings.beginGroup("Program");
  settings.setValue("save_directory", data_directory_);
  QString profile_directory = settings.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings.endGroup();

  if (!profile_directory.isEmpty()) {
    std::string path = profile_directory.toStdString() + "/optimize.set";
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    pugi::xml_node node = root.append_child("Optimizer");
    selected_fitter_.settings().to_xml(node);
    doc.save_file(path.c_str());
  }

  settings.beginGroup("Experiment");
  settings.setValue("autofit", ui->checkAutofit->isChecked());
  ui->plotSpectrum->saveSettings(settings);
  ui->widgetAutoselect->saveSettings(settings);
  settings.endGroup();
}

void FormExperiment::run_completed() {
  if (my_run_) {
    ui->pushStop->setEnabled(false);
    my_run_ = false;
    emit toggleIO(true);

    if (continue_)
    {
      LINFO << "<FormExperiment> Completed one run";
      start_new_pass();
    }
  }
}

void FormExperiment::new_daq_data() {
  update_name();
  display_time();

  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  ui->pushRunInfo->setEnabled(p.operator bool());
  ui->pushSinkInfo->setEnabled(sink.operator bool());
  ui->pushExportProject->setEnabled(p.operator bool());

  if (ui->plotSpectrum->busy())
    return;

  if (!sink)
  {
    FitSettings fitset = selected_fitter_.settings();
    selected_fitter_ = Qpx::Fitter();
    selected_fitter_.apply_settings(fitset);
    ui->plotSpectrum->updateData();
    return;
  }

  Qpx::Metadata md = sink->metadata();

  bool refit = false;
  if (p->has_fitter(selected_sink_))
  {
    selected_fitter_ = p->get_fitter(selected_sink_);
    selected_fitter_.setData(sink);
    if (selected_fitter_.metadata_.total_count < md.total_count)
      refit = true;
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

  if (ui->checkAutofit->isChecked() &&
      ((selected_fitter_.peak_count() < 1) || refit))
    ui->plotSpectrum->perform_fit();
}

void FormExperiment::update_name()
{
  QString name = "Experiment";
  if (my_run_)
    name += QString::fromUtf8("  \u25b6");
  else if (exp_project_.changed())
    name += QString::fromUtf8(" \u2731");

  if (name != this->windowTitle()) {
    this->setWindowTitle(name);
    emit toggleIO(true);
  }
}

void FormExperiment::toggle_from_setup()
{
  emit toggleIO(true);
  display_time();
}

void FormExperiment::start_new_pass()
{
  Qpx::TrajectoryPtr pr = get_next_point();
  Qpx::TrajectoryPtr parent;

  if (pr)
    parent = pr->getParent();

  if (!parent || (pr->data_idx < 0))
  {
    LINFO << "<FormExperiment> No valid next point. Terminating experiment.";
    return;
  }

  Qpx::ProjectPtr proj = exp_project_.get_data(pr->data_idx);
  uint64_t duration = parent->domain.criterion;

  if (!proj)
  {
    LINFO << "<FormExperiment> Experiment failed to provide daq project. Terminating.";
    return;
  }

  if (!duration)
  {
    LINFO << "<FormExperiment> Next run duration = 0. Terminating experiment.";
    return;
  }

  ui->pushStop->setEnabled(true);
  my_run_ = true;
  emit toggleIO(false);

  interruptor_.store(false);
  runner_thread_.do_run(proj, interruptor_, duration);
}

Qpx::TrajectoryPtr FormExperiment::get_next_point()
{
  Qpx::TrajectoryPtr ret = exp_project_.next_setting();
  if (!apply_setting(ret))
    return nullptr;

  form_experiment_setup_->retro_push(ret);

  if (ret->data_idx >= 0)
    return ret;
  else
    return get_next_point();
}

bool FormExperiment::apply_setting(Qpx::TrajectoryPtr node)
{
  if (!node)
    return false;

  Qpx::TrajectoryPtr parent = node->getParent();
  if (!parent)
    return false;

  LINFO << "<FormExperiment> Applying setting " << node->type() << " " << node->to_string();

  if (parent->domain.type == Qpx::DomainType::manual)
  {
    Qpx::Setting set = node->domain_value;
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
      node->domain_value.set_number(d);
    else
    {
      LINFO << "<FormExperiment> User failed to confirm manual setting. Aborting";
      update_name();
      return false;
    }
  }
  else if (parent->domain.type == Qpx::DomainType::source)
  {
    Qpx::Engine::getInstance().get_all_settings();
    if (!Qpx::Engine::getInstance().pull_settings().has(node->domain_value, Qpx::Match::id | Qpx::Match::indices))
    {
      LINFO << "<FormExperiment> Source does not have this setting. Aborting";
      update_name();
      return false;
    }
    Qpx::Engine::getInstance().set_setting(node->domain_value, Qpx::Match::id | Qpx::Match::indices);
    QThread::sleep(0.5);
    Qpx::Engine::getInstance().get_all_settings();

    double newval = Qpx::Engine::getInstance().pull_settings().get_setting(node->domain_value, Qpx::Match::id | Qpx::Match::indices).number();
    node->domain_value.set_number(newval);

    emit settings_changed();
  }
  return true;
}



void FormExperiment::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);
  bool empty = exp_project_.empty();
  bool done = exp_project_.done();
  bool hasprototypes = exp_project_.get_prototypes().size();

  ui->pushStart->setEnabled(online && enable && !my_run_ && !empty && !done && hasprototypes);

  ui->pushNewExperiment->setEnabled(enable && !my_run_ && !empty);
  ui->pushSaveExperiment->setEnabled(enable && !my_run_ && !empty);
  ui->pushLoadExperiment->setEnabled(enable && !my_run_);

  form_experiment_setup_->toggle_push(enable && !my_run_);
  update_name();
  display_time();
}


void FormExperiment::update_fits() {
  ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->reselect(selected_fitter_.peaks(), ui->plotSpectrum->get_selected_peaks()));

  update_peak_selection(std::set<double>());
}

void FormExperiment::fitting_done()
{
  std::set<double> cursel = selected_fitter_.get_selected_peaks();
  if (selected_fitter_.peak_count() && (cursel.empty() || (cursel.size() > 1)))
    ui->plotSpectrum->set_selected_peaks(ui->widgetAutoselect->autoselect(selected_fitter_.peaks(), cursel));
  save_propagate_fit();
}

void FormExperiment::update_peak_selection(std::set<double> dummy) {
  save_propagate_fit();
}

void FormExperiment::save_propagate_fit()
{
  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  if (!sink)
    return;

  selected_fitter_.set_selected_peaks(ui->plotSpectrum->get_selected_peaks());
  p->update_fitter(selected_sink_, selected_fitter_);

  exp_project_.gather_results();
  form_experiment_1d_->update_exp_project();
  form_experiment_2d_->update_exp_project();
}

void FormExperiment::fitter_status(bool busy)
{
  ui->widgetFileOps->setEnabled(!busy);
  form_experiment_setup_->setEnabled(!busy);
  ui->spectrumSelector->setEnabled(!busy);
}

void FormExperiment::selectProject(int64_t idx)
{
  Qpx::ProjectPtr p = exp_project_.get_data(idx);
  //  if (!p)
  //    return;

  exp_plot_thread_.monitor_source(p);
  new_daq_data();
  //  p->activate();
}

void FormExperiment::on_pushNewExperiment_clicked()
{
  if (exp_project_.changed()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Project non-empty. Clear data?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
      return;
  }

  selected_sink_ = -1;
  new_daq_data();
  exp_plot_thread_.terminate_wait();

  exp_project_ = Qpx::ExperimentProject();
  form_experiment_setup_->update_exp_project();

  exp_project_.gather_results();
  form_experiment_1d_->update_exp_project();
  form_experiment_2d_->update_exp_project();
  populate_selector();
  emit toggleIO(true);
}

void FormExperiment::on_pushLoadExperiment_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load experiment",
                                                  data_directory_, "qpx multi-experiment (*.qmx)");
  if (!validateFile(this, fileName, false))
    return;

  data_directory_ = path_of_file(fileName);

  if (fileName.isEmpty())
    return;

  if (exp_project_.changed()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Project non-empty. Discard data?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
      return;
  }

  pugi::xml_document doc;
  if (doc.load_file(fileName.toStdString().c_str())) {
    this->setCursor(Qt::WaitCursor);
    if (doc.child(exp_project_.xml_element_name().c_str()))
      exp_project_.from_xml(doc.child(exp_project_.xml_element_name().c_str()));

    selected_sink_ = -1;
    new_daq_data();
    exp_plot_thread_.terminate_wait();

    form_experiment_setup_->update_exp_project();

    exp_project_.gather_results();
    form_experiment_1d_->update_exp_project();
    form_experiment_2d_->update_exp_project();
    populate_selector();
    emit toggleIO(true);
    this->setCursor(Qt::ArrowCursor);
  }
}

void FormExperiment::on_pushSaveExperiment_clicked()
{
  QString fileName = CustomSaveFileDialog(this, "Save experiment",
                                          data_directory_, "Qpx multi-experiment (*.qmx)");
  if (!validateFile(this, fileName, true))
    return;

  data_directory_ = path_of_file(fileName);

  if (!fileName.isEmpty()) {
    this->setCursor(Qt::WaitCursor);
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    exp_project_.to_xml(root);
    doc.save_file(fileName.toStdString().c_str());
    this->setCursor(Qt::ArrowCursor);
  }
}

void FormExperiment::on_pushStart_clicked()
{
  form_experiment_1d_->update_exp_project();
  form_experiment_2d_->update_exp_project();
  FitSettings fitset = selected_fitter_.settings();
  selected_fitter_ = Qpx::Fitter();
  selected_fitter_.apply_settings(fitset);


  if (exp_project_.has_results())
  {
    DBG << " <FormExperiment> Setting all settings prior to resuming";
    Qpx::TrajectoryPtr ret = exp_project_.next_setting();
    if (!ret)
      return;
    if (!apply_all_recursive(ret))
      return;
    //maybe add to tree to avoid redundant setting?
  }

  continue_ = true;
  start_new_pass();
}

bool FormExperiment::apply_all_recursive(Qpx::TrajectoryPtr node)
{
  Qpx::TrajectoryPtr parent = node->getParent();
  if (!parent || (parent->domain.type == Qpx::DomainType::none))
    return true;
  if (!apply_all_recursive(parent))
    return false;
  return apply_setting(node);
}

void FormExperiment::on_pushStop_clicked()
{
  continue_ = false;
  interruptor_.store(true);
}

void FormExperiment::choose_spectrum(SelectorItem item)
{
  SelectorItem itm = ui->spectrumSelector->selected();

  if (itm.data.toLongLong() == selected_sink_)
    return;

  selected_sink_ = itm.data.toLongLong();
  form_experiment_1d_->update_exp_project();
  form_experiment_2d_->update_exp_project();
  new_daq_data();
}

void FormExperiment::spectrumDoubleclicked(SelectorItem item)
{
  //  on_pushDetails_clicked();
}

void FormExperiment::populate_selector()
{
  XMLableDB<Qpx::Metadata> ptp = exp_project_.get_prototypes();

  QString sel;
  QVector<SelectorItem> items;
  int i=1;
  for (auto &md : ptp.my_data_) {
    SelectorItem new_spectrum;
    new_spectrum.visible = md.attributes.branches.get(Qpx::Setting("visible")).value_int;
    new_spectrum.text = QString::fromStdString(md.name);
    new_spectrum.data = QVariant::fromValue(i);
    new_spectrum.color = QColor(QString::fromStdString(md.attributes.branches.get(Qpx::Setting("appearance")).value_text));
    items.push_back(new_spectrum);
    i++;

    if (sel.isEmpty())
      sel = new_spectrum.text;
  }

  ui->spectrumSelector->setItems(items);
  ui->spectrumSelector->setSelected(sel);
  choose_spectrum(SelectorItem());
}

void FormExperiment::display_time()
{
  Qpx::Setting time;
  time.metadata.setting_type = Qpx::SettingType::time_duration;

  double totals = exp_project_.estimate_total_time();
  double dones = exp_project_.total_real_time();
  double remaining = std::abs(totals - dones);

  time.value_duration = boost::posix_time::milliseconds(totals * 1000.0);
  std::string total = time.val_to_pretty_string();

  time.value_duration = boost::posix_time::milliseconds(dones * 1000.0);
  std::string done = time.val_to_pretty_string();

  time.value_duration = boost::posix_time::milliseconds(remaining * 1000.0);
  std::string eta = time.val_to_pretty_string();

  std::string all;
  if (!totals)
    ;
  else if ((dones > totals) && continue_)
    all = "Cumulative time: " + done + "   ETA: -" + eta + " past due";
  else if (!dones)
    all = "Estimated total run time = " + total;
  else
    all = "Cumulative time: " + done + "   ETA: " + eta;

  ui->labelTime->setText(QString::fromStdString(all));
}

void FormExperiment::on_pushRunInfo_clicked()
{
  if (ui->plotSpectrum->busy())
    return;

  FormDaqSettings *DaqInfo = new FormDaqSettings(exp_plot_thread_.current_source(), this);
  DaqInfo->setWindowTitle("System settings at the time of acquisition");
  DaqInfo->exec();
}

void FormExperiment::on_pushSinkInfo_clicked()
{
  if (ui->plotSpectrum->busy())
    return;

  Qpx::SinkPtr sink;

  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (p)
    sink = p->get_sink(selected_sink_);

  if (sink)
  {
    XMLableDB<Qpx::Detector> dets("detectors"); //HACK

    DialogSpectrum* newSpecDia = new DialogSpectrum(*sink, dets, false, this);
//    connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
    // should go to new_daq_data
    newSpecDia->exec();

  }
}

void FormExperiment::on_pushExportProject_clicked()
{
  Qpx::ProjectPtr p = exp_plot_thread_.current_source();
  if (!p)
    return;

  Qpx::ProjectPtr newproj(new Qpx::Project(*p));
//  newproj->mark_changed();

  emit extract_project(newproj);
}
