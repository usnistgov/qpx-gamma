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
#include "qt_util.h"


FormMcaDaq::FormMcaDaq(ThreadRunner &thread, QSettings &settings, XMLableDB<Qpx::Detector>& detectors,
                       std::vector<Qpx::Detector>& current_dets, QWidget *parent) :
  QWidget(parent),
  settings_(settings),
  spectra_templates_("SpectrumTemplates"),
  interruptor_(false),
  spectra_(),
  my_analysis_(nullptr),
  my_analysis_2d_(nullptr),
  my_eff_cal_(nullptr),
  my_symmetrization_2d_(nullptr),
  runner_thread_(thread),
  plot_thread_(spectra_),
  detectors_(detectors),
  current_dets_(current_dets),
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
  mca_load_formats_ = catFileTypes(filetypes) + ";;Radware spn (*.spn)";

  //1d
  ui->Plot1d->setSpectra(spectra_);
  connect(ui->Plot1d, SIGNAL(requestAnalysis(QString)), this, SLOT(reqAnal(QString)));
  connect(ui->Plot1d, SIGNAL(requestEffCal(QString)), this, SLOT(reqEffCal(QString)));
  connect(&plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));
  ui->Plot1d->setDetDB(detectors_);

  //2d
  ui->Plot2d->setSpectra(spectra_);
  connect(ui->Plot1d, SIGNAL(marker_set(Marker)), ui->Plot2d, SLOT(set_marker(Marker)));
  connect(ui->Plot2d, SIGNAL(markers_set(Marker,Marker)), ui->Plot1d, SLOT(set_markers2d(Marker,Marker)));
  connect(ui->Plot2d, SIGNAL(requestAnalysis(QString)), this, SLOT(reqAnal2D(QString)));
  connect(ui->Plot2d, SIGNAL(requestSymmetrize(QString)), this, SLOT(reqSym2D(QString)));
  ui->Plot2d->setDetDB(detectors_);

  menuLoad.addAction(QIcon(":/icons/oxy/16/document_open.png"), "Open qpx project", this, SLOT(projectOpen()));
  menuLoad.addAction(QIcon(":/icons/oxy/16/document_open_folder.png"), "Import spectra", this, SLOT(projectImport()));
  ui->toolOpen->setMenu(&menuLoad);

  menuSave.addAction(QIcon(":/icons/oxy/16/document_save.png"), "Save project", this, SLOT(projectSave()));
  menuSave.addAction(QIcon(":/icons/oxy/16/document_save_as.png"), "Save project as...", this, SLOT(projectSaveAs()));
  menuSave.addAction(QIcon(":/icons/oxy/16/document_save_all.png"), "Export spectra...", this, SLOT(projectExport()));
  ui->toolSave->setMenu(&menuSave);

  this->setWindowTitle(QString::fromStdString(spectra_.identity()));

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
  }

  if (my_analysis_ != nullptr) {
    PL_DBG << "will kill assoc analysis1D";
    my_analysis_->close(); //assume always successful
    emit requestClose(my_analysis_);
  }

  if (my_analysis_2d_ != nullptr) {
    PL_DBG << "will kill assoc analysis2D";
    my_analysis_2d_->close(); //assume always successful
    emit requestClose(my_analysis_2d_);
  }

  if (my_symmetrization_2d_!= nullptr) {
    PL_DBG << "will kill assoc sym2D";
    my_symmetrization_2d_->close(); //assume always successful
    emit requestClose(my_symmetrization_2d_);
  }

  if (my_eff_cal_ != nullptr) {
    PL_DBG << "will kill assoc effcal";
    my_eff_cal_->close(); //assume always successful
    emit requestClose(my_eff_cal_);
  }

  if (spectra_.changed()) {
    int reply = QMessageBox::warning(this, "Project contents changed",
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
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  spectra_templates_.read_xml(settings_directory_.toStdString() + "/default_spectra.tem");

  settings_.beginGroup("McaDaq");
  ui->timeDuration->set_total_seconds(settings_.value("run_secs", 60).toULongLong());
  ui->toggleIndefiniteRun->setChecked(settings_.value("run_indefinite", false).toBool());
  ui->timeDuration->setEnabled(!ui->toggleIndefiniteRun->isChecked());
  ui->pushEnable2d->setChecked(settings_.value("2d_visible", true).toBool());

  settings_.beginGroup("McaPlot");
  ui->Plot1d->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->Plot1d->set_plot_style(settings_.value("plot_style", "Step center").toString());
  settings_.endGroup();

  settings_.beginGroup("MatrixPlot");
  ui->Plot2d->set_zoom(settings_.value("zoom", 50).toDouble());
  ui->Plot2d->set_gradient(settings_.value("gradient", "Hot").toString());
  ui->Plot2d->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->Plot2d->set_show_legend(settings_.value("show_legend", false).toBool());
  settings_.endGroup();

  settings_.endGroup();

  on_pushEnable2d_clicked();
}

void FormMcaDaq::saveSettings() {
  settings_.beginGroup("McaDaq");
  settings_.setValue("run_secs", QVariant::fromValue(ui->timeDuration->total_seconds()));
  settings_.setValue("run_indefinite", ui->toggleIndefiniteRun->isChecked());
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
  bool nonempty = !spectra_.empty();

  ui->pushMcaStart->setEnabled(enable && online && !my_run_);

  ui->timeDuration->setEnabled(enable && online && !ui->toggleIndefiniteRun->isChecked());
  ui->toggleIndefiniteRun->setEnabled(enable && online);

  ui->toolOpen->setEnabled(enable && !my_run_);
  ui->toolSave->setEnabled(enable && nonempty && !my_run_);
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

  QString name = QString::fromStdString(spectra_.identity());
  if (name != "New project") {
    QStringList slist = name.split("/");
    if (!slist.empty())
      name = slist.back();
  }

  if (my_run_)
    name += QString::fromUtf8("  \u25b6");
  else if (spectra_.changed())
    name += QString::fromUtf8(" \u2731");

  if (name != this->windowTitle()) {
    this->setWindowTitle(name);
    emit toggleIO(true);
  }

  ui->pushEditSpectra->setVisible(spectra_.empty());

  if (ui->Plot2d->isVisible()) {
    this->setCursor(Qt::WaitCursor);
    ui->Plot2d->update_plot();
  }

  if (ui->Plot1d->isVisible()) {
    this->setCursor(Qt::WaitCursor);
    ui->Plot1d->update_plot();
  }

  //ui->statusBar->showMessage("Spectra acquisition in progress...");
//  PL_DBG << "<FormMcaDaq> Gui-side plotting " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}


void FormMcaDaq::projectSave()
{
  if (spectra_.changed() && (spectra_.identity() != "New project")) {
    int reply = QMessageBox::warning(this, "Save?",
                                     "Save changes to existing project: " + QString::fromStdString(spectra_.identity()),
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      this->setCursor(Qt::WaitCursor);
      spectra_.save();
      update_plots();
    }
  } else
    projectSaveAs();
}

void FormMcaDaq::projectSaveAs()
{
  QString fileName = CustomSaveFileDialog(this, "Save project",
                                          data_directory_, "qpx project file (*.qpx)");
  if (validateFile(this, fileName, true)) {
    PL_INFO << "Writing project to " << fileName.toStdString();
    this->setCursor(Qt::WaitCursor);
    spectra_.save_as(fileName.toStdString());
    update_plots();
  }

  update_data_dir(fileName);
}

void FormMcaDaq::projectExport()
{
  this->setCursor(Qt::WaitCursor);
  DialogSaveSpectra* newDialog = new DialogSaveSpectra(spectra_, data_directory_);
  connect(newDialog, SIGNAL(accepted()), this, SLOT(after_export()));
  connect(newDialog, SIGNAL(rejected()), this, SLOT(after_export()));
  newDialog->exec();
}

void FormMcaDaq::after_export() {
  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::on_pushEditSpectra_clicked()
{
  DialogSpectraTemplates* newDialog = new DialogSpectraTemplates(spectra_templates_, current_dets_,
                                                                 settings_directory_, this);
  newDialog->exec();
}

void FormMcaDaq::on_pushMcaStart_clicked()
{
  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Continue?",
                                     "Non-empty spectra in project. Acquire and append to existing data?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
      return;
    else
      start_DAQ();
  } else {
    DialogSpectraTemplates* newDialog = new DialogSpectraTemplates(spectra_templates_, current_dets_,
                                                                   settings_directory_, this);
    connect(newDialog, SIGNAL(accepted()), this, SLOT(start_DAQ()));
    newDialog->exec();
  }
}

void FormMcaDaq::start_DAQ() {
  if (spectra_.empty() && spectra_templates_.empty())
    return;

  emit statusText("Spectra acquisition in progress...");

  emit toggleIO(false);
  ui->pushMcaStop->setEnabled(true);

  if (spectra_.empty()) {
    clearGraphs();
    spectra_.set_spectra(spectra_templates_);
    newProject();
  }
//  spectra_.activate();

  my_run_ = true;
  ui->Plot1d->reset_content();
  uint64_t duration = ui->timeDuration->total_seconds();
  if (ui->toggleIndefiniteRun->isChecked())
    duration = 0;
  runner_thread_.do_run(spectra_, interruptor_, duration);
}


void FormMcaDaq::projectOpen()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load project", data_directory_, "qpx project file (*.qpx)");
  if (!validateFile(this, fileName, false))
    return;

  update_data_dir(fileName);

  //save first?
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

  spectra_.read_xml(fileName.toStdString(), true);

  newProject();
  spectra_.activate();

  emit toggleIO(true);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::projectImport()
{
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load spectra", data_directory_, mca_load_formats_);

  if (fileNames.empty())
    return;

  update_data_dir(fileNames.front());

  for (int i=0; i<fileNames.size(); i++)
    if (!validateFile(this, fileNames.at(i), false))
      return;

  if ((!spectra_.empty()) && (QMessageBox::warning(this, "Append?", "Spectra already open. Append to existing?",
                                                    QMessageBox::Yes|QMessageBox::Cancel) != QMessageBox::Yes))
    return;

  //toggle_push(false, false);
  this->setCursor(Qt::WaitCursor);

  for (int i=0; i<fileNames.size(); i++) {
    PL_INFO << "Importing from " << fileNames.at(i).toStdString();

    std::string ext(boost::filesystem::extension(fileNames.at(i).toStdString()));
    if (ext.size())
      ext = ext.substr(1, ext.size()-1);
    boost::algorithm::to_lower(ext);

    if (ext == "spn") {
      spectra_.import_spn(fileNames.at(i).toStdString());
      for (auto &q : spectra_.spectra())
        q->set_appearance(generateColor().rgba());
    } else {
      Qpx::Spectrum::Spectrum* newSpectrum = Qpx::Spectrum::Factory::getInstance().create_from_file(fileNames.at(i).toStdString());
      if (newSpectrum != nullptr) {
        newSpectrum->set_appearance(generateColor().rgba());
        spectra_.add_spectrum(newSpectrum);
      } else {
        PL_INFO << "Spectrum construction did not succeed.";
      }
    }
  }
  newProject(); //NOT ALWAYS TRUE!!!
  spectra_.activate();

  emit toggleIO(true);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::update_data_dir(QString filename) {
  QFileInfo file(filename);
  if (file.absoluteDir().isReadable()) {
    data_directory_ = file.absoluteDir().absolutePath();
    settings_.beginGroup("Program");
    settings_.setValue("save_directory", data_directory_);
    settings_.endGroup();
  }
  //propagate to other gui elements?
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
    ui->pushMcaStop->setEnabled(false);
    my_run_ = false;

    update_plots();

    emit toggleIO(true);
  }
}


void FormMcaDaq::reqAnal(QString name) {
  this->setCursor(Qt::WaitCursor);

  if (my_analysis_ == nullptr) {
    my_analysis_ = new FormAnalysis1D(settings_, detectors_, this);
    connect(&plot_thread_, SIGNAL(plot_ready()), my_analysis_, SLOT(update_spectrum()));
    connect(my_analysis_, SIGNAL(destroyed()), this, SLOT(analysis_destroyed()));
  }
  my_analysis_->setWindowTitle("1D: " + name);
  my_analysis_->setSpectrum(&spectra_, name);
  emit requestAnalysis(my_analysis_);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::analysis_destroyed() {
  PL_DBG << "analysis destroyed";
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
    my_symmetrization_2d_ = new FormSymmetrize2D(settings_, detectors_, this);
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

void FormMcaDaq::reqEffCal(QString name)
{
  this->setCursor(Qt::WaitCursor);

  if (my_eff_cal_ == nullptr) {
    my_eff_cal_ = new FormEfficiencyCalibration(settings_, detectors_, this);
    connect(&plot_thread_, SIGNAL(plot_ready()), my_eff_cal_, SLOT(update_spectrum()));
    connect(my_eff_cal_, SIGNAL(destroyed()), this, SLOT(analysis_destroyed()));
  }
  my_eff_cal_->setWindowTitle("1D: " + name);
  my_eff_cal_->setDetector(&spectra_, name);
  emit requestEfficiencyCal(my_eff_cal_);

  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::analysis2d_destroyed() {
  my_analysis_2d_ = nullptr;
}

void FormMcaDaq::sym2d_destroyed() {
  my_symmetrization_2d_ = nullptr;
}

void FormMcaDaq::effcal_destroyed() {
  my_eff_cal_ = nullptr;
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
