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
 *      Module and channel settings
 *
 ******************************************************************************/

#include "gui/form_pixie_settings.h"
#include "ui_form_pixie_settings.h"

FormPixieSettings::FormPixieSettings(ThreadRunner& thread, XMLableDB<Pixie::Detector>& detectors, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  detectors_(detectors),
  settings_(settings),
  pixie_(Pixie::Wrapper::getInstance()),
  ui(new Ui::FormPixieSettings)
{
  ui->setupUi(this);
  default_detectors_.resize(4);

  connect(&runner_thread_, SIGNAL(settingsUpdated()), this, SLOT(refresh()));

  //PixieSettings
  ui->comboFilterSamples->blockSignals(true);
  ui->boxCoincWait->blockSignals(true);
  for (int i=1; i < 7; i++)
    ui->comboFilterSamples->addItem(QIcon(), QString::number(pow(2,i)), i);

  loadSettings();

  ui->viewChanSettings->setModel(&channel_settings_model_);
  ui->viewChanSettings->setItemDelegate(&settings_delegate_);
  ui->viewChanSettings->horizontalHeader()->setStretchLastSection(true);
  ui->viewChanSettings->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  settings_delegate_.eat_detectors(detectors_);
  channel_settings_model_.update();
  ui->viewChanSettings->show();

  connect(&channel_settings_model_, SIGNAL(detectors_changed()), this, SLOT(updateDetChoices()));
}

void FormPixieSettings::update() {
  ui->boxCoincWait->setValue(pixie_.settings().get_mod("ACTUAL_COINCIDENCE_WAIT"));
  ui->comboFilterSamples->setCurrentText(QString::number(pow(2,pixie_.settings().get_mod("FILTER_RANGE"))));
  channel_settings_model_.update();
  ui->viewChanSettings->resizeColumnsToContents();
  ui->viewChanSettings->horizontalHeader()->setStretchLastSection(true);
}

void FormPixieSettings::refresh() {
  update();
  emit toggleIO(true);
}

void FormPixieSettings::apply_settings() {
  pixie_.settings().set_mod("FILTER_RANGE", ui->comboFilterSamples->currentData().toDouble());
  pixie_.settings().set_mod("ACTUAL_COINCIDENCE_WAIT", ui->boxCoincWait->value());
  for (int i =0; i < 4; i++)
    pixie_.settings().set_detector(Pixie::Channel(i), detectors_.get(Pixie::Detector(default_detectors_[i])));
}

void FormPixieSettings::closeEvent(QCloseEvent *event) {

  saveSettings();
  event->accept();
}

void FormPixieSettings::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (live == Pixie::LiveStatus::online);
  bool offline = (live == Pixie::LiveStatus::offline);

  ui->boxCoincWait->setEnabled(enable && online);
  ui->comboFilterSamples->setEnabled(enable && online);
  ui->buttonCompTau->setEnabled(enable && online);
  ui->buttonCompBLC->setEnabled(enable && online);
  ui->pushSettingsRefresh->setEnabled(enable && online);

  ui->comboFilterSamples->blockSignals(!enable);
  ui->boxCoincWait->blockSignals(!enable);
  if (enable)
    ui->viewChanSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
  else
    ui->viewChanSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->pushOptimizeAll->setEnabled(enable && (online || offline));
}

void FormPixieSettings::loadSettings() {
  settings_.beginGroup("Pixie");
  ui->comboFilterSamples->setCurrentText(QString::number(pow(2, settings_.value("filter_samples", 4).toInt())));
  ui->boxCoincWait->setValue(settings_.value("coinc_wait", 1).toDouble());
  default_detectors_[0] = settings_.value("detector_0", QString("N/A")).toString().toStdString();
  default_detectors_[1] = settings_.value("detector_1", QString("N/A")).toString().toStdString();
  default_detectors_[2] = settings_.value("detector_2", QString("N/A")).toString().toStdString();
  default_detectors_[3] = settings_.value("detector_3", QString("N/A")).toString().toStdString();
  settings_.endGroup();
}

void FormPixieSettings::saveSettings() {
  settings_.beginGroup("Pixie");
  settings_.setValue("filter_samples", ui->comboFilterSamples->currentData().toInt());
  settings_.setValue("coinc_wait", ui->boxCoincWait->value());
  settings_.setValue("detector_0", QString::fromStdString(default_detectors_[0]));
  settings_.setValue("detector_1", QString::fromStdString(default_detectors_[1]));
  settings_.setValue("detector_2", QString::fromStdString(default_detectors_[2]));
  settings_.setValue("detector_3", QString::fromStdString(default_detectors_[3]));
  settings_.endGroup();
}

void FormPixieSettings::updateDetChoices() {
  std::vector<Pixie::Detector> dets = pixie_.settings().get_detectors();
  default_detectors_.clear();
  for (auto &q : dets)
    default_detectors_.push_back(q.name_);
}

void FormPixieSettings::updateDetDB() {
  for (std::size_t i = 0; i < pixie_.settings().get_detectors().size(); i++) {
    std::string det_old = pixie_.settings().get_detector(Pixie::Channel(i)).name_;
    pixie_.settings().set_detector(Pixie::Channel(i), detectors_.get(det_old));
  }
  settings_delegate_.eat_detectors(detectors_);
  channel_settings_model_.update();
  ui->viewChanSettings->horizontalHeader()->setStretchLastSection(true);
  ui->viewChanSettings->resizeColumnsToContents();
}

FormPixieSettings::~FormPixieSettings()
{
  delete ui;
}

void FormPixieSettings::on_buttonCompTau_clicked()
{
  emit statusText("Calculating Tau...");
  emit toggleIO(false);
  runner_thread_.do_tau();
}


void FormPixieSettings::on_buttonCompBLC_clicked()
{
  emit statusText("Calculating baselines cutoffs...");
  emit toggleIO(false);
  runner_thread_.do_BLcut();
}

void FormPixieSettings::on_pushOptimizeAll_clicked()
{
  emit statusText("Applying detector optimizations...");
  emit toggleIO(false);
  runner_thread_.do_optimize();
}

void FormPixieSettings::on_pushSettingsRefresh_clicked()
{
  emit statusText("Refreshing settings_...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormPixieSettings::on_comboFilterSamples_currentIndexChanged(int index)
{
  int val = ui->comboFilterSamples->currentData().toInt();
  pixie_.settings().set_mod("FILTER_RANGE", static_cast<double>(val));
  runner_thread_.do_refresh_settings();
}

void FormPixieSettings::on_boxCoincWait_editingFinished()
{
  double val = ui->boxCoincWait->value();
  pixie_.settings().set_mod("ACTUAL_COINCIDENCE_WAIT", val);
  runner_thread_.do_refresh_settings();
}
