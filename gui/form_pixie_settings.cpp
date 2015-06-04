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
  tree_settings_model_(pixie_.settings().pull_settings()),
  ui(new Ui::FormPixieSettings)
{
  ui->setupUi(this);
  default_detectors_.resize(Pixie::kNumChans);

  connect(&runner_thread_, SIGNAL(settingsUpdated()), this, SLOT(refresh()));

  loadSettings();

  ui->treeSettings->setModel(&tree_settings_model_);
  ui->treeSettings->setItemDelegate(&tree_delegate_);
  tree_delegate_.eat_detectors(detectors_);

  connect(&tree_settings_model_, SIGNAL(detectors_changed()), this, SLOT(updateDetChoices()));
  connect(&tree_settings_model_, SIGNAL(tree_changed()), this, SLOT(push_settings()));

}

void FormPixieSettings::update() {
  tree_settings_model_.update(Pixie::Wrapper::getInstance().settings().pull_settings());
}

void FormPixieSettings::push_settings() {
  pixie_.settings().push_settings(tree_settings_model_.get_tree());
  emit statusText("Refreshing settings_...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormPixieSettings::refresh() {
  update();
  emit toggleIO(true);
}

void FormPixieSettings::apply_settings() {
  for (int i =0; i < Pixie::kNumChans; i++)
    pixie_.settings().set_detector(Pixie::Channel(i), detectors_.get(Pixie::Detector(default_detectors_[i])));
}

void FormPixieSettings::closeEvent(QCloseEvent *event) {
  saveSettings();
  event->accept();
}

void FormPixieSettings::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (live == Pixie::LiveStatus::online);
  bool offline = (live == Pixie::LiveStatus::offline);

  ui->buttonCompTau->setEnabled(enable && online);
  ui->buttonCompBLC->setEnabled(enable && online);
  ui->pushSettingsRefresh->setEnabled(enable && online);

  if (enable)
    ui->treeSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
  else
    ui->treeSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->pushOptimizeAll->setEnabled(enable && (online || offline));
}

void FormPixieSettings::loadSettings() {
  settings_.beginGroup("Pixie");
  default_detectors_[0] = settings_.value("detector_0", QString("N/A")).toString().toStdString();
  default_detectors_[1] = settings_.value("detector_1", QString("N/A")).toString().toStdString();
  default_detectors_[2] = settings_.value("detector_2", QString("N/A")).toString().toStdString();
  default_detectors_[3] = settings_.value("detector_3", QString("N/A")).toString().toStdString();
  settings_.endGroup();
}

void FormPixieSettings::saveSettings() {
  settings_.beginGroup("Pixie");
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
  tree_delegate_.eat_detectors(detectors_);
  tree_settings_model_.update(Pixie::Wrapper::getInstance().settings().pull_settings());
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
