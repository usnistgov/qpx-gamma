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

#include "form_system_settings.h"
#include "ui_form_system_settings.h"
#include "widget_detectors.h"

FormSystemSettings::FormSystemSettings(ThreadRunner& thread, XMLableDB<Gamma::Detector>& detectors, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  detectors_(detectors),
  settings_(settings),
  tree_settings_model_(this),
  table_settings_model_(this),
  ui(new Ui::FormSystemSettings)
{
  ui->setupUi(this);
  connect(&runner_thread_, SIGNAL(settingsUpdated(Gamma::Setting, std::vector<Gamma::Detector>)), this, SLOT(update(Gamma::Setting, std::vector<Gamma::Detector>)));
  connect(&runner_thread_, SIGNAL(bootComplete()), this, SLOT(post_boot()));

  tree_settings_model_.update(dev_settings_);

  viewTreeSettings = new QTreeView(this);
  ui->tabsSettings->addTab(viewTreeSettings, "Device tree");
  viewTreeSettings->setModel(&tree_settings_model_);
  viewTreeSettings->setItemDelegate(&tree_delegate_);
  viewTreeSettings->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tree_delegate_.eat_detectors(detectors_);

  viewTableSettings = new QTableView(this);
  ui->tabsSettings->addTab(viewTableSettings, "Settings table");
  viewTableSettings->setModel(&table_settings_model_);
  viewTableSettings->setItemDelegate(&table_settings_delegate_);
  viewTableSettings->horizontalHeader()->setStretchLastSection(true);
  viewTableSettings->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  table_settings_delegate_.eat_detectors(detectors_);
  table_settings_model_.update(channels_);
  viewTableSettings->show();

  connect(&tree_settings_model_, SIGNAL(tree_changed()), this, SLOT(push_settings()));
  connect(&tree_settings_model_, SIGNAL(execute_command()), this, SLOT(execute_command()));
  connect(&tree_settings_model_, SIGNAL(detector_chosen(int, std::string)), this, SLOT(chose_detector(int,std::string)));

  connect(&table_settings_model_, SIGNAL(setting_changed(int, Gamma::Setting)), this, SLOT(push_from_table(int, Gamma::Setting)));
  connect(&table_settings_model_, SIGNAL(detector_chosen(int, std::string)), this, SLOT(chose_detector(int,std::string)));

  loadSettings();
}

void FormSystemSettings::update(const Gamma::Setting &tree, const std::vector<Gamma::Detector> &channels) {
  dev_settings_ = tree;
  channels_ = channels;

  viewTreeSettings->clearSelection();
  //viewTableSettings->clearSelection();

  tree_settings_model_.update(dev_settings_);
  table_settings_model_.update(channels_);

  //update dets in DB as well?

  viewTableSettings->resizeColumnsToContents();
  viewTableSettings->horizontalHeader()->setStretchLastSection(true);
}

void FormSystemSettings::push_settings() {
  dev_settings_ = tree_settings_model_.get_tree();

  emit statusText("Updating settings...");
  emit toggleIO(false);
  runner_thread_.do_push_settings(dev_settings_);
}

void FormSystemSettings::execute_command() {
  dev_settings_ = tree_settings_model_.get_tree();

  emit statusText("Executing command...");
  emit toggleIO(false);
  runner_thread_.do_execute_command(dev_settings_);
}

void FormSystemSettings::push_from_table(int chan, Gamma::Setting setting) {
  setting.index = chan;

  emit statusText("Updating settings...");
  emit toggleIO(false);
  runner_thread_.do_set_setting(setting, Gamma::Match::id | Gamma::Match::indices);
}

void FormSystemSettings::chose_detector(int chan, std::string name) {
  Gamma::Detector det = detectors_.get(Gamma::Detector(name));
  PL_DBG << "det " <<  det.name_ << " with cali " << det.energy_calibrations_.size() << " has sets " << det.settings_.branches.size();
  for (auto &q : det.settings_.branches.my_data_)
    q.index = chan;

  emit statusText("Applying detector settings...");
  emit toggleIO(false);
  runner_thread_.do_set_detector(chan, det);
}


void FormSystemSettings::refresh() {

  emit statusText("Updating settings...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormSystemSettings::closeEvent(QCloseEvent *event) {

  saveSettings();
  event->accept();
}

void FormSystemSettings::toggle_push(bool enable, Qpx::LiveStatus live) {
  bool online = (live == Qpx::LiveStatus::online);
  bool offline = (live == Qpx::LiveStatus::offline);

  ui->pushSettingsRefresh->setEnabled(enable && (online || offline));

  if (enable) {
    viewTableSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
    viewTreeSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
  } else {
    viewTableSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);
    viewTreeSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }

  ui->bootButton->setEnabled(enable);
  ui->pushOptimizeAll->setEnabled(enable && (online || offline));

  if ((live == Qpx::LiveStatus::online) || (live == Qpx::LiveStatus::offline)) {
    ui->bootButton->setText("Reset system");
    ui->bootButton->setIcon(QIcon(":/new/icons/oxy/start.png"));
  } else {
    ui->bootButton->setText("Boot system");
    ui->bootButton->setIcon(QIcon(":/new/icons/Cowboy-Boot.png"));
  }
}

void FormSystemSettings::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();

  ui->checkShowRO->setChecked(settings_.value("settings_table_show_readonly", true).toBool());
  on_checkShowRO_clicked();

  if (settings_.value("settings_tab_tree", true).toBool())
    ui->tabsSettings->setCurrentWidget(viewTreeSettings);
  else
    ui->tabsSettings->setCurrentWidget(viewTableSettings);

  settings_.endGroup();
}

void FormSystemSettings::saveSettings() {
  for (auto &q : channels_) {
    if (q.name_ == "none")
      continue;
    Gamma::Detector det = detectors_.get(q);
    if (det != Gamma::Detector()) {
      for (auto &p : q.settings_.branches.my_data_) {
        if (p.metadata.writable) {
          det.settings_.branches.replace(p);
        }
      }
      if (!det.settings_.branches.empty())
        det.settings_.metadata.setting_type = Gamma::SettingType::stem;
      det.settings_.strip_metadata();
      detectors_.replace(det);
    }
  }
  
  settings_.beginGroup("Program");
  settings_.setValue("settings_table_show_readonly", ui->checkShowRO->isChecked());
  settings_.setValue("settings_tab_tree", (ui->tabsSettings->currentWidget() == viewTreeSettings));
  settings_.endGroup();
}

void FormSystemSettings::updateDetDB() {
  tree_delegate_.eat_detectors(detectors_);
  table_settings_delegate_.eat_detectors(detectors_);

  tree_settings_model_.update(dev_settings_);
  table_settings_model_.update(channels_);

  viewTableSettings->horizontalHeader()->setStretchLastSection(true);
  viewTableSettings->resizeColumnsToContents();
}

FormSystemSettings::~FormSystemSettings()
{
  delete ui;
}

void FormSystemSettings::post_boot()
{
  on_pushOptimizeAll_clicked();
}


void FormSystemSettings::on_pushOptimizeAll_clicked()
{
  emit statusText("Applying detector optimizations...");
  emit toggleIO(false);

  std::map<int, Gamma::Detector> update;
  for (int i=0; i < channels_.size(); ++i)
    if (detectors_.has_a(channels_[i]))
      update[i] = detectors_.get(channels_[i]);
    
  runner_thread_.do_set_detectors(update);
}

void FormSystemSettings::on_pushSettingsRefresh_clicked()
{
  emit statusText("Refreshing settings_...");
  emit toggleIO(false);
  runner_thread_.do_refresh_settings();
}

void FormSystemSettings::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(updateDetDB()));
  det_widget->exec();
}

void FormSystemSettings::on_checkShowRO_clicked()
{
  //viewTreeSettings->clearSelection();
  //viewTableSettings->clearSelection();

  table_settings_model_.set_show_read_only(ui->checkShowRO->isChecked());
  table_settings_model_.update(channels_);

  tree_settings_model_.set_show_read_only(ui->checkShowRO->isChecked());
  tree_settings_model_.update(dev_settings_);
}

void FormSystemSettings::on_bootButton_clicked()
{
  if (dev_settings_.value_int == 0) {
    emit toggleIO(false);
    emit statusText("Booting...");
    PL_INFO << "Booting system...";

    runner_thread_.do_boot();
  } else {
    emit toggleIO(false);
    emit statusText("Shutting down...");

    PL_INFO << "Shutting down";
    runner_thread_.do_shutdown();
  }
}
