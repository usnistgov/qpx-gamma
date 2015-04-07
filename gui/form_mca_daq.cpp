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

#include "gui/form_mca_daq.h"
#include "ui_form_mca_daq.h"
#include "dialog_spectra_templates.h"
#include "dialog_save_spectra.h"
#include "custom_logger.h"
#include "custom_timer.h"

FormMcaDaq::FormMcaDaq(ThreadRunner &thread, QSettings &settings, XMLableDB<Pixie::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  settings_(settings),
  spectra_templates_("SpectrumTemplates"),
  interruptor_(false),
  spectra_(),
  my_calib_(nullptr),
  runner_thread_(thread),
  plot_thread_(spectra_),
  detectors_(detectors),
  ui(new Ui::FormMcaDaq)
{
  ui->setupUi(this);

  loadSettings();

  //connect with runner
  connect(&runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  //file formats for opening mca spectra
  std::vector<std::string> spectypes = Pixie::Spectrum::Factory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    Pixie::Spectrum::Template* type_template = Pixie::Spectrum::Factory::getInstance().create_template(q);
    if (!type_template->input_types.empty())
      filetypes.push_back("Spectrum " + QString::fromStdString(q) + "(" + catExtensions(type_template->input_types) + ")");
    delete type_template;
  }
  mca_load_formats_ = catFileTypes(filetypes);

  //plots
  ui->Plot1d->setSpectra(spectra_);
  ui->Plot2d->setSpectra(spectra_);
  connect(ui->Plot2d, SIGNAL(markers_set(double,double)), ui->Plot1d, SLOT(set_markers2d(double,double)));
  connect(ui->Plot1d, SIGNAL(marker_set(double)), ui->Plot2d, SLOT(set_marker(double)));
  connect(ui->Plot1d, SIGNAL(requestCalibration(QString)), this, SLOT(reqCalib(QString)));
  connect(&plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  plot_thread_.start();
}

FormMcaDaq::~FormMcaDaq()
{
}

void FormMcaDaq::closeEvent(QCloseEvent *event) {
  if (runner_thread_.isRunning()) {
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

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Spectra still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  if (my_calib_ != nullptr) {
    my_calib_->close(); //assume always successful
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
  ui->boxMcaMins->setValue(settings_.value("mca_mins", 5).toInt());
  ui->boxMcaSecs->setValue(settings_.value("mca_secs", 0).toInt());
  settings_.endGroup();
}

void FormMcaDaq::saveSettings() {
  settings_.beginGroup("Lab");
  settings_.setValue("mca_mins", ui->boxMcaMins->value());
  settings_.setValue("mca_secs", ui->boxMcaSecs->value());
  settings_.endGroup();
}

void FormMcaDaq::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (live == Pixie::LiveStatus::online);
  bool offline = online || (live == Pixie::LiveStatus::offline);
  bool nonempty = !spectra_.empty();

  ui->pushMcaLoad->setEnabled(enable);
  ui->pushMcaReload->setEnabled(enable);

  ui->pushMcaStart->setEnabled(enable && online);
  ui->checkDoubleBuffer->setEnabled(enable && online);

  ui->boxMcaMins->setEnabled(enable && offline);
  ui->boxMcaSecs->setEnabled(enable && offline);
  ui->pushMcaSimulate->setEnabled(enable && offline);

  ui->pushMcaClear->setEnabled(enable && nonempty);
  ui->pushMcaSave->setEnabled(enable && nonempty);
}

void FormMcaDaq::on_pushTimeNow_clicked()
{
  PL_INFO << "Time now!";
}

void FormMcaDaq::clearGraphs() //rename this
{
  spectra_.clear();
  ui->Plot2d->reset(); //is this necessary?

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
  PL_DBG << "Gui-side plotting " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}


void FormMcaDaq::on_pushMcaSave_clicked()
{
  DialogSaveSpectra* newDialog = new DialogSaveSpectra(spectra_, data_directory_);
  connect(newDialog, SIGNAL(accepted()), this, SLOT(enableIObuttons())); //needs its own slot in case not saved
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
  updateSpectraUI();
  spectra_.activate();

  runner_thread_.do_run(spectra_, interruptor_, Pixie::RunType::compressed, ui->boxMcaMins->value() * 60 + ui->boxMcaSecs->value(), ui->checkDoubleBuffer->isChecked());
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
  updateSpectraUI();
  spectra_.activate();

  ui->pushMcaStop->setEnabled(true);

  //hardcoded precision and channels
  runner_thread_.do_fake(spectra_, interruptor_, fileName, {0,1}, 14, 12, ui->boxMcaMins->value() * 60 + ui->boxMcaSecs->value());
}

void FormMcaDaq::on_pushMcaReload_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load project", data_directory_, "qpx project file (*.qpx)");
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
  PL_INFO << "Reading spectra from complete acquisition record in xml file " << fileName.toStdString();
  this->setCursor(Qt::WaitCursor);
  clearGraphs();

  spectra_.read_xml(fileName.toStdString(), true);

  updateSpectraUI();
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
    Pixie::Spectrum::Spectrum* newSpectrum = Pixie::Spectrum::Factory::getInstance().create_from_file(fileNames.at(i).toStdString());
    if (newSpectrum != nullptr) {
      newSpectrum->set_appearance(generateColor().rgba());
      spectra_.add_spectrum(newSpectrum);
    } else {
      PL_INFO << "Spectrum construction did not succeed. Aborting";
    }
  }
  updateSpectraUI();
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
    spectra_.clear();
    updateSpectraUI();
    ui->pushMcaSave->setEnabled(false);
    ui->pushMcaClear->setEnabled(false);
    //spectra_.activate();
  }
}

void FormMcaDaq::updateSpectraUI() {
  ui->Plot2d->setSpectra(spectra_);
  ui->Plot1d->setSpectra(spectra_);
//  ui->formCalibration->clear();
}

void FormMcaDaq::on_pushMcaStop_clicked()
{
  ui->pushMcaStop->setEnabled(false);
  PL_INFO << "MCA acquisition interrupted by user";
  interruptor_.store(true);
}

void FormMcaDaq::run_completed() {

  PL_INFO << "FormMcaDaq received signal for run completed";
  this->setCursor(Qt::WaitCursor);
  ui->Plot1d->update_plot();
  ui->Plot2d->update_plot();
  ui->pushMcaStop->setEnabled(false);
  emit toggleIO(true);
  this->setCursor(Qt::ArrowCursor);
}

void FormMcaDaq::on_pushEditSpectra_clicked()
{
  DialogSpectraTemplates* newDialog = new DialogSpectraTemplates(spectra_templates_, data_directory_, this);
  newDialog->exec();
}

void FormMcaDaq::reqCalib(QString name) {
  if (my_calib_ == nullptr) {
    my_calib_ = new FormCalibration();
    connect(&plot_thread_, SIGNAL(plot_ready()), my_calib_, SLOT(update_plot()));
    connect(my_calib_, SIGNAL(destroyed()), this, SLOT(calib_destroyed()));
  }
  my_calib_->setData(detectors_, settings_);
  my_calib_->setSpectrum(&spectra_, name);
  emit requestCalibration(my_calib_);
}

void FormMcaDaq::calib_destroyed() {
  PL_INFO << "calibration dstrd";
  my_calib_ = nullptr;
}

void FormMcaDaq::replot() {
  update_plots();
}
