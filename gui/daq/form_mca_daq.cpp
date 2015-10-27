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
 *      Mca interface
 *
 ******************************************************************************/

#include "form_mca_daq.h"
#include "ui_form_mca_daq.h"
#include "dialog_spectra_templates.h"
#include "dialog_save_spectra.h"
#include "custom_logger.h"
#include "custom_timer.h"
#include "form_daq_settings.h"

#include "sorter.h"

FormMcaDaq::FormMcaDaq(ThreadRunner &thread, QSettings &settings, XMLableDB<Gamma::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  settings_(settings),
  spectra_templates_("SpectrumTemplates"),
  interruptor_(false),
  spectra_(),
  my_analysis_(nullptr),
  my_analysis_2d_(nullptr),
  my_symmetrization_2d_(nullptr),
  runner_thread_(thread),
  plot_thread_(spectra_),
  detectors_(detectors),
  my_run_(false),
  ui(new Ui::FormMcaDaq)
{
  ui->setupUi(this);

  loadSettings();

  //connect with runner
  connect(&runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  //file formats for opening mca spectra
  std::vector<std::string> spectypes = Qpx::Spectrum::Factory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    Qpx::Spectrum::Template* type_template = Qpx::Spectrum::Factory::getInstance().create_template(q);
    if (!type_template->input_types.empty())
      filetypes.push_back("Spectrum " + QString::fromStdString(q) + "(" + catExtensions(type_template->input_types) + ")");
    delete type_template;
  }
  mca_load_formats_ = catFileTypes(filetypes);

  //1d
  ui->Plot1d->setSpectra(spectra_);
  connect(ui->Plot1d, SIGNAL(requestAnalysis(QString)), this, SLOT(reqAnal(QString)));
  connect(&plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  //2d
  ui->Plot2d->setSpectra(spectra_);
  connect(ui->Plot1d, SIGNAL(marker_set(Marker)), ui->Plot2d, SLOT(set_marker(Marker)));
  connect(ui->Plot2d, SIGNAL(markers_set(Marker,Marker)), ui->Plot1d, SLOT(set_markers2d(Marker,Marker)));
  connect(ui->Plot2d, SIGNAL(requestAnalysis(QString)), this, SLOT(reqAnal2D(QString)));
  connect(ui->Plot2d, SIGNAL(requestSymmetrize(QString)), this, SLOT(reqSym2D(QString)));

  plot_thread_.start();
}

FormMcaDaq::~FormMcaDaq()
{
}

void FormMcaDaq::closeEvent(QCloseEvent *event) {
  if (my_run_ && runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      runner_thread_.terminate();
      runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  } else {
    runner_thread_.terminate();
    runner_thread_.wait();
  }


  if (my_analysis_ != nullptr) {
    my_analysis_->close(); //assume always successful
  }

  if (my_analysis_2d_ != nullptr) {
    my_analysis_2d_->close(); //assume always successful
  }

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Spectra still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      //spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  spectra_.terminate();
  plot_thread_.wait();
  saveSettings();

  event->accept();
}

void FormMcaDaq::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  spectra_templates_.read_xml(data_directory_.toStdString() + "/default_spectra.tem");

  settings_.beginGroup("Lab");
  ui->timeDuration->set_total_seconds(settings_.value("mca_secs", 0).toULongLong());
  ui->pushEnable2d->setChecked(settings_.value("2d_visible", true).toBool());

  settings_.beginGroup("McaPlot");
  ui->Plot1d->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->Plot1d->set_plot_style(settings_.value("plot_style", "Step").toString());
  settings_.endGroup();

  settings_.beginGroup("MatrixPlot");
  ui->Plot2d->set_zoom(settings_.value("zoom", 50).toDouble());
  ui->Plot2d->set_gradient(settings_.value("gradient", "hot").toString());
  ui->Plot2d->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->Plot2d->set_show_legend(settings_.value("show_legend", false).toBool());
  settings_.endGroup();

  settings_.endGroup();

  on_pushEnable2d_clicked();
}

void FormMcaDaq::saveSettings() {
  settings_.beginGroup("Lab");
  settings_.setValue("mca_secs", QVariant::fromValue(ui->timeDuration->total_seconds()));
  settings_.setValue("2d_visible", ui->pushEnable2d->isChecked());

  settings_.beginGroup("McaPlot");
  settings_.setValue("scale_type", ui->Plot1d->scale_type());
  settings_.setValue("plot_style", ui->Plot1d->plot_style());
  settings_.endGroup();

  settings_.beginGroup("MatrixPlot");
  settings_.setValue("zoom", ui->Plot2d->zoom());
  settings_.setValue("gradient", ui->Plot2d->gradient());
  settings_.setValue("scale_type", ui->Plot2d->scale_type());
  settings_.setValue("show_legend", ui->Plot2d->show_legend());
  settings_.endGroup();

  settings_.endGroup();
}

void FormMcaDaq::toggle_push(bool enable, Qpx::DeviceStatus status) {
  bool online = (status & Qpx::DeviceStatus::can_run);
  bool offline = online || (status & Qpx::DeviceStatus::loaded);
  bool nonempty = !spectra_.empty();

  ui->pushMcaLoad->setEnabled(enable);
  ui->pushMcaReload->setEnabled(enable);

  ui->pushMcaStart->setEnabled(enable && online);

  ui->timeDuration->setEnabled(enable && offline);
  ui->toggleIndefiniteRun->setEnabled(enable && offline);
  ui->pushMcaSimulate->setEnabled(enable && offline);

  ui->pushMcaClear->setEnabled(enable && nonempty);
  ui->pushMcaSave->setEnabled(enable && nonempty);
  ui->pushBuildFromList->setEnabled(enable);
}

void FormMcaDaq::on_pushTimeNow_clicked()
{
  PL_INFO << "Time now!";
}

void FormMcaDaq::clearGraphs() //rename this
{
  spectra_.clear();
  newProject();
  ui->Plot1d->reset_content();

  ui->Plot2d->reset_content(); //is this necessary?

  spectra_.activate();
}


void FormMcaDaq::update_plots() {
  //ui->statusBar->showMessage("Updating plots");

  CustomTimer guiside(true);

  if (ui->Plot2d->isVisible()) {
    this->setCursor(Qt::WaitCursor);
    ui->Plot2d->update_plot();
  }

  if (ui->Plot1d->isVisible()) {
    this->setCursor(Qt::WaitCursor);
    ui->Plot1d->update_plot();
  }

  //ui->statusBar->showMessage("Spectra acquisition in progress...");
  //PL_DBG << "Gui-side plotting " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}


void FormMcaDaq::on_pushMcaSave_clicked()
{
  this->setCursor(Qt::WaitCursor);
  DialogSaveSpectra* newDialog = new DialogSaveSpectra(spectra_, data_directory_);
  connect(newDialog, SIGNAL(accepted()), this, SLOT(update_plots())); //needs its own slot in case not saved
  newDialog->exec();
}

void FormMcaDaq::on_pushMcaStart_clicked()
{
  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Spectra already open. Clear existing before opening?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
      spectra_.clear();
    else
      return;
  }

  emit statusText("Spectra acquisition in progress...");

  emit toggleIO(false);
  ui->pushMcaStop->setEnabled(true);

  clearGraphs();
  spectra_.set_spectra(spectra_templates_);
  newProject();
//  spectra_.activate();

  my_run_ = true;
  ui->Plot1d->reset_content();
  uint64_t duration = ui->timeDuration->total_seconds();
  if (ui->toggleIndefiniteRun->isChecked())
    duration = 0;
  runner_thread_.do_run(spectra_, interruptor_, duration);
}

void FormMcaDaq::on_pushMcaSimulate_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load simulation data", data_directory_,
                                                  "qpx project file (*.qpx)");
  if (!validateFile(this, fileName, false)) {
    return;
  }

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Spectra already open. Clear existing before opening?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
      spectra_.clear();
    else
      return;
  }

  emit statusText("Simulated spectra acquisition in progress...");

  emit toggleIO(false);
  PL_INFO << "Reading spectra for simulation " << fileName.toStdString();
  //make popup to warn of delay

  clearGraphs();
  spectra_.set_spectra(spectra_templates_);
  newProject();
//  spectra_.activate();

  ui->pushMcaStop->setEnabled(true);
  my_run_ = true;

  //hardcoded precision and channels
  ui->Plot1d->reset_content();
  uint64_t duration = ui->timeDuration->total_seconds();
  if (ui->toggleIndefiniteRun->isChecked())
    duration = 0;
  runner_thread_.do_fake(spectra_, interruptor_, fileName, {0,1}, 14, 12, duration);
}

void FormMcaDaq::on_pushMcaReload_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load project", data_directory_, "qpx project file (*.qpx);;Radware spn (*.spn)");
  if (!validateFile(this, fileName, false))
    return;

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Spectra already open. Clear existing before opening?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
      spectra_.clear();
    else
      return;
  }

  //toggle_push(false, false);
  PL_INFO << "Reading spectra from file " << fileName.toStdString();
  this->setCursor(Qt::WaitCursor);
  clearGraphs();

  std::string ext(boost::filesystem::extension(fileName.toStdString()));
  if (ext.size())
    ext = ext.substr(1, ext.size()-1);
  boost::algorithm::to_lower(ext);

  if (ext == "qpx")
    spectra_.read_xml(fileName.toStdString(), true);
  else if (ext == "spn") {
    spectra_.read_spn(fileName.toStdString());
    for (auto &q : spectra_.spectra())
      q->set_appearance(generateColor().rgba());
  }

  newProject();
  spectra_.activate();

  emit toggleIO(true);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::on_pushMcaLoad_clicked()
{
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load spectra", data_directory_, mca_load_formats_);

  if (fileNames.empty())
    return;

  for (int i=0; i<fileNames.size(); i++)
    if (!validateFile(this, fileNames.at(i), false))
      return;

  if ((!spectra_.empty()) && (QMessageBox::warning(this, "Append?", "Spectra already open. Append to existing?",
                                                    QMessageBox::Yes|QMessageBox::Cancel) != QMessageBox::Yes))
    return;

  //toggle_push(false, false);
  this->setCursor(Qt::WaitCursor);

  for (int i=0; i<fileNames.size(); i++) {
    PL_INFO << "Constructing spectrum from " << fileNames.at(i).toStdString();
    Qpx::Spectrum::Spectrum* newSpectrum = Qpx::Spectrum::Factory::getInstance().create_from_file(fileNames.at(i).toStdString());
    if (newSpectrum != nullptr) {
      newSpectrum->set_appearance(generateColor().rgba());
      spectra_.add_spectrum(newSpectrum);
    } else {
      PL_INFO << "Spectrum construction did not succeed. Aborting";
    }
  }
  newProject(); //NOT ALWAYS TRUE!!!
  spectra_.activate();

  emit toggleIO(true);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::on_pushMcaClear_clicked()
{
  int reply = QMessageBox::warning(this, "Clear spectra?",
                                   "Clear all spectra?",
                                   QMessageBox::Yes|QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    clearGraphs();
    ui->pushMcaSave->setEnabled(false);
    ui->pushMcaClear->setEnabled(false);
    //spectra_.activate();
  }
}

void FormMcaDaq::updateSpectraUI() {
  ui->Plot2d->updateUI();
  ui->Plot1d->setSpectra(spectra_);
}

void FormMcaDaq::newProject() {
  ui->Plot2d->setSpectra(spectra_);
  ui->Plot2d->update_plot(true);
  ui->Plot1d->setSpectra(spectra_);
}

void FormMcaDaq::on_pushMcaStop_clicked()
{
  ui->pushMcaStop->setEnabled(false);
  //PL_INFO << "MCA acquisition interrupted by user";
  interruptor_.store(true);
}

void FormMcaDaq::run_completed() {
  if (my_run_) {
    //PL_INFO << "FormMcaDaq received signal for run completed";
    update_plots();
    ui->pushMcaStop->setEnabled(false);
    emit toggleIO(true);
    my_run_ = false;
  }
}

void FormMcaDaq::on_pushEditSpectra_clicked()
{
  DialogSpectraTemplates* newDialog = new DialogSpectraTemplates(spectra_templates_, data_directory_, this);
  newDialog->exec();
}

void FormMcaDaq::reqAnal(QString name) {
  this->setCursor(Qt::WaitCursor);

  if (my_analysis_ == nullptr) {
    my_analysis_ = new FormAnalysis1D(settings_, detectors_);
    connect(&plot_thread_, SIGNAL(plot_ready()), my_analysis_, SLOT(update_spectrum()));
    connect(my_analysis_, SIGNAL(destroyed()), this, SLOT(analysis_destroyed()));
  }
  my_analysis_->setWindowTitle("1D: " + name);
  my_analysis_->setSpectrum(&spectra_, name);
  emit requestAnalysis(my_analysis_);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::analysis_destroyed() {
  my_analysis_ = nullptr;
}

void FormMcaDaq::reqAnal2D(QString name) {
  this->setCursor(Qt::WaitCursor);

  if (my_analysis_2d_ == nullptr) {
    my_analysis_2d_ = new FormAnalysis2D(settings_, detectors_);
    //connect(&plot_thread_, SIGNAL(plot_ready()), my_analysis_2d_, SLOT(update_spectrum()));
    connect(my_analysis_2d_, SIGNAL(destroyed()), this, SLOT(analysis2d_destroyed()));
    connect(my_analysis_2d_, SIGNAL(spectraChanged()), this, SLOT(updateSpectraUI()));
  }
  my_analysis_2d_->setWindowTitle("2D: " + name);
  my_analysis_2d_->setSpectrum(&spectra_, name);
  my_analysis_2d_->reset();
  emit requestAnalysis2D(my_analysis_2d_);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::reqSym2D(QString name) {
  this->setCursor(Qt::WaitCursor);

  if (my_symmetrization_2d_ == nullptr) {
    my_symmetrization_2d_ = new FormSymmetrize2D(settings_, detectors_);
    //connect(&plot_thread_, SIGNAL(plot_ready()), my_symmetrization_2d_, SLOT(update_spectrum()));
    connect(my_symmetrization_2d_, SIGNAL(destroyed()), this, SLOT(sym2d_destroyed()));
    connect(my_symmetrization_2d_, SIGNAL(spectraChanged()), this, SLOT(updateSpectraUI()));
  }
  my_symmetrization_2d_->setWindowTitle("2D: " + name);
  my_symmetrization_2d_->setSpectrum(&spectra_, name);
  my_symmetrization_2d_->reset();
  emit requestSymmetriza2D(my_symmetrization_2d_);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::analysis2d_destroyed() {
  my_analysis_2d_ = nullptr;
}

void FormMcaDaq::sym2d_destroyed() {
  my_symmetrization_2d_ = nullptr;
}

void FormMcaDaq::replot() {
  update_plots();
}

void FormMcaDaq::on_pushEnable2d_clicked()
{
  if (ui->pushEnable2d->isChecked()) {
    ui->Plot2d->show();
    update_plots();
  } else
    ui->Plot2d->hide();
  ui->line_sep2d->setVisible(ui->pushEnable2d->isChecked());
}

void FormMcaDaq::on_pushForceRefresh_clicked()
{
  updateSpectraUI();
  update_plots();
}

void FormMcaDaq::on_pushBuildFromList_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load list output", data_directory_, "qpx list output manifest (*.txt)");
  if (!validateFile(this, fileName, false))
    return;

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Clear existing?",
                                     "Spectra already open. Clear existing before building new?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
      spectra_.clear();
    else
      return;
  }

  PL_INFO << "Reading list output metadata from " << fileName.toStdString();

  emit statusText("Simulated spectra acquisition in progress...");
  emit toggleIO(false);

  clearGraphs();
  spectra_.set_spectra(spectra_templates_);
  newProject();

  ui->pushMcaStop->setEnabled(true);
  my_run_ = true;

  ui->Plot1d->reset_content();
  runner_thread_.do_from_list(spectra_, interruptor_, fileName);
}

void FormMcaDaq::on_pushDetails_clicked()
{
  FormDaqSettings *DaqInfo = new FormDaqSettings(spectra_.runInfo().state, this);
  DaqInfo->setWindowTitle("System settings at the time of acquisition");
  DaqInfo->exec();
}

void FormMcaDaq::on_toggleIndefiniteRun_clicked()
{
   ui->timeDuration->setEnabled(!ui->toggleIndefiniteRun->isChecked());
}
