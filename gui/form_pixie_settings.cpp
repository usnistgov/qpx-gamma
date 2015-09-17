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
#include "widget_detectors.h"

FormPixieSettings::FormPixieSettings(ThreadRunner& thread, XMLableDB<Gamma::Detector>& detectors, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  detectors_(detectors),
  settings_(settings),
  pixie_(Pixie::Wrapper::getInstance()),
  ui(new Ui::FormPixieSettings)
{
  ui->setupUi(this);
  connect(&runner_thread_, SIGNAL(settingsUpdated()), this, SLOT(refresh()));

  //PixieSettings
  ui->comboFilterSamples->blockSignals(true);
  ui->boxCoincWait->blockSignals(true);
  for (int i=1; i < 7; i++)
    ui->comboFilterSamples->addItem(QIcon(), QString::number(pow(2,i)), i);

  loadSettings();

  dev_settings_ = pixie_.settings().pull_settings();

  tree_settings_model_.set_structure(dev_settings_);

  ui->treeSettings->setModel(&tree_settings_model_);
  ui->treeSettings->setItemDelegate(&tree_delegate_);
  tree_delegate_.eat_detectors(detectors_);

  ui->viewChanSettings->setModel(&channel_settings_model_);
  ui->viewChanSettings->setItemDelegate(&settings_delegate_);
  ui->viewChanSettings->horizontalHeader()->setStretchLastSection(true);
  ui->viewChanSettings->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  settings_delegate_.eat_detectors(detectors_);
  channel_settings_model_.update(pixie_.settings().get_detectors());
  ui->viewChanSettings->show();

  connect(&tree_settings_model_, SIGNAL(tree_changed()), this, SLOT(push_settings()));
  connect(&tree_settings_model_, SIGNAL(detector_chosen(int, std::string)), this, SLOT(chose_detector(int,std::string)));

  connect(&channel_settings_model_, SIGNAL(setting_changed(int, Gamma::Setting)), this, SLOT(push_from_table(int, Gamma::Setting)));
  connect(&channel_settings_model_, SIGNAL(detector_chosen(int, std::string)), this, SLOT(chose_detector(int,std::string)));

}

void FormPixieSettings::update() {
  dev_settings_ = pixie_.settings().pull_settings();
  tree_settings_model_.update(dev_settings_);
  PL_DBG << "pulled dev settings have " << dev_settings_.branches.size() << " branches";

  ui->treeSettings->resizeColumnToContents(0);
  ui->boxCoincWait->setValue(pixie_.settings().get_mod("ACTUAL_COINCIDENCE_WAIT"));
  ui->comboFilterSamples->setCurrentText(QString::number(pow(2,pixie_.settings().get_mod("FILTER_RANGE"))));
  channel_settings_model_.update(pixie_.settings().get_detectors());
  ui->viewChanSettings->resizeColumnsToContents();
  ui->viewChanSettings->horizontalHeader()->setStretchLastSection(true);
}

void FormPixieSettings::push_settings() {
  dev_settings_ = tree_settings_model_.get_tree();

  PL_DBG << "from model dev settings have " << dev_settings_.branches.size() << " branches";

  pixie_.settings().push_settings(dev_settings_);
  emit statusText("Refreshing settings_...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormPixieSettings::push_from_table(int chan, Gamma::Setting setting) {
  pixie_.settings().set_setting(setting, chan);
  emit statusText("Refreshing settings_...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormPixieSettings::chose_detector(int chan, std::string name) {
  PL_DBG << "chose_detector " << name;
  Gamma::Detector det = detectors_.get(Gamma::Detector(name));
  PL_DBG << "det " <<  det.name_ << " with cali " << det.energy_calibrations_.size() << " has sets " << det.settings_.branches.size();
  for (auto &q : det.settings_.branches.my_data_)
    q.index = chan;
  pixie_.settings().set_detector(chan, det);
  pixie_.settings().write_settings_bulk();
  runner_thread_.do_refresh_settings();
}


void FormPixieSettings::refresh() {
  update();
  emit toggleIO(true);
}

void FormPixieSettings::apply_settings() {
  pixie_.settings().set_mod("FILTER_RANGE", ui->comboFilterSamples->currentData().toDouble());
  pixie_.settings().set_mod("ACTUAL_COINCIDENCE_WAIT", ui->boxCoincWait->value());
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
  ui->pushSettingsRefresh->setEnabled(enable && (online || offline));

  ui->comboFilterSamples->blockSignals(!enable);
  ui->boxCoincWait->blockSignals(!enable);
  if (enable) {
    ui->viewChanSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->treeSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
  } else {
    ui->viewChanSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }

  ui->pushOptimizeAll->setEnabled(enable && (online || offline));
}

void FormPixieSettings::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  QString filename = data_directory_ + "/qpx_settings.set";

  FILE* myfile;
  myfile = fopen (filename.toStdString().c_str(), "r");
  if (myfile != nullptr) {
    tinyxml2::XMLDocument docx;
    docx.LoadFile(myfile);
    tinyxml2::XMLElement* root = docx.FirstChildElement(Gamma::Setting().xml_element_name().c_str());
    if (root != nullptr) {
      dev_settings_ = Gamma::Setting(root);
      pixie_.settings().push_settings(dev_settings_);
      //tree_settings_model_.set_structure(newtree);
    }
    fclose(myfile);
  }

  settings_.beginGroup("Pixie");
  ui->comboFilterSamples->setCurrentText(QString::number(pow(2, settings_.value("filter_samples", 4).toInt())));
  ui->boxCoincWait->setValue(settings_.value("coinc_wait", 1).toDouble());
  settings_.endGroup();
}

void FormPixieSettings::saveSettings() {

  pixie_.settings().save_optimization();
  std::vector<Gamma::Detector> dets = pixie_.settings().get_detectors();
  for (int i=0; i < detectors_.size(); ++i) {
    for (auto &q : dets)
      if (q.shallow_equals(detectors_.get(i)))
        detectors_.replace(q);
  }

  QString filename = data_directory_ + "/qpx_settings.set";
  dev_settings_ = pixie_.settings().pull_settings();

  PL_DBG << "dev settings have " << dev_settings_.branches.size() << " branches";

  FILE* myfile;
  myfile = fopen (filename.toStdString().c_str(), "w");
  if (myfile != nullptr) {
    tinyxml2::XMLPrinter printer(myfile);
    printer.PushHeader(true, true);
    dev_settings_.to_xml(printer);
    fclose(myfile);
  }

  //should replace only optimizations, not touch the rest of detector definition

  updateDetChoices();

  settings_.beginGroup("Pixie");
  settings_.setValue("filter_samples", ui->comboFilterSamples->currentData().toInt());
  settings_.setValue("coinc_wait", ui->boxCoincWait->value());
  settings_.endGroup();
}

void FormPixieSettings::updateDetChoices() {

}

void FormPixieSettings::updateDetDB() {
  for (std::size_t i = 0; i < pixie_.settings().get_detectors().size(); i++) {
    std::string det_old = pixie_.settings().get_detectors()[i].name_;
    pixie_.settings().set_detector(i, detectors_.get(det_old));
  }
  tree_delegate_.eat_detectors(detectors_);
  settings_delegate_.eat_detectors(detectors_);

  tree_settings_model_.update(Pixie::Wrapper::getInstance().settings().pull_settings());
  channel_settings_model_.update(pixie_.settings().get_detectors());

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

void FormPixieSettings::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  //connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}
