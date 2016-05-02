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
 * Description:
 *      DialogProfile - dialog for editing the definition of detector
 *      TableProfiles - table model for detector set
 *      WidgetProfiles - dialog for managing detector set
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "widget_profiles.h"
#include "ui_widget_profiles.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include "project.h"
#include <boost/algorithm/string.hpp>
#include "form_daq_settings.h"
#include "qt_util.h"


int TableProfiles::rowCount(const QModelIndex & /*parent*/) const
{    return profiles_.size(); }

int TableProfiles::columnCount(const QModelIndex & /*parent*/) const
{    return 1; }

QVariant TableProfiles::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();
  std::stringstream dss;

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QString::fromStdString(profiles_[row].get_setting(Qpx::Setting("Profile description"), Qpx::Match::id).value_text);
//    case 1:
//      return QString::fromStdString(profiles_[row].value_text);
    }
  }
  return QVariant();
}

QVariant TableProfiles::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return "Decription";
//      case 1:
//        return "Path";
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableProfiles::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1 );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableProfiles::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}




WidgetProfiles::WidgetProfiles(QWidget *parent) :
  QDialog(parent),
  table_model_(profiles_),
  selection_model_(&table_model_),
  ui(new Ui::WidgetProfiles)
{
  ui->setupUi(this);

  update_profiles();

  ui->tableProfiles->setModel(&table_model_);
  ui->tableProfiles->setSelectionModel(&selection_model_);
  ui->tableProfiles->verticalHeader()->hide();
  ui->tableProfiles->horizontalHeader()->setStretchLastSection(true);
  ui->tableProfiles->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableProfiles->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableProfiles->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableProfiles->show();


  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));
  connect(ui->tableProfiles, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(selection_double_clicked(QModelIndex)));

}

WidgetProfiles::~WidgetProfiles()
{
  delete ui;
}

void WidgetProfiles::update_profiles()
{

  selection_model_.clearSelection();
  profiles_.clear();

  QSettings settings;
  settings.beginGroup("Program");
  QString settings_directory = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();

  QDir directory(settings_directory + "/profiles");
  directory.setFilter(QDir::Dirs);
  QStringList subdirs = directory.entryList();
  for (auto &q : subdirs) {

    QDir dir2(settings_directory + "/profiles/" + q);
    QStringList nameFilter("profile.set");
    QStringList profile_files = dir2.entryList(nameFilter);

    if (profile_files.empty()) {
      PL_DBG << "<WidgetProfiles> no profile found in " << dir2.absolutePath().toStdString();
      continue;
    }

    std::string path = dir2.absolutePath().toStdString() + "/profile.set";

    pugi::xml_document doc;
    if (!doc.load_file(path.c_str())) {
      PL_DBG << "<WidgetProfiles> Cannot parse XML in " << path;
      continue;
    }

    pugi::xml_node root = doc.child(Qpx::Setting().xml_element_name().c_str());
    if (!root) {
      PL_DBG << "<WidgetProfiles> Profile root element invalid in " << path;
      continue;
    }

//    PL_DBG << "<WidgetProfiles> valid profile at " << path;

    Qpx::Setting tree(root);
    tree.value_text = dir2.absolutePath().toStdString();
    profiles_.push_back(tree);
  }

  Qpx::Setting tree("Empty");
  tree.metadata.setting_type = Qpx::SettingType::stem;
  Qpx::Setting descr("Profile description");
  descr.metadata.setting_type = Qpx::SettingType::text;
  descr.value_text = "Offline analysis mode";
  tree.branches.add_a(descr);
  profiles_.push_back(tree);

  table_model_.update();
}

void WidgetProfiles::selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
}

void WidgetProfiles::toggle_push() {
    ui->pushApply->setEnabled(!selection_model_.selectedIndexes().empty());
    ui->pushApplyBoot->setEnabled(!selection_model_.selectedIndexes().empty());
    ui->pushEdit->setEnabled(!selection_model_.selectedIndexes().empty());
}


void WidgetProfiles::on_pushEdit_clicked()
{
  QModelIndexList ixl = ui->tableProfiles->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  FormDaqSettings *DaqInfo = new FormDaqSettings(profiles_[i], this);
  DaqInfo->setWindowTitle(QString::fromStdString(profiles_[i].value_text));
  DaqInfo->exec();

}

void WidgetProfiles::selection_double_clicked(QModelIndex idx) {
  int i = idx.row();

  QSettings settings;
  settings.beginGroup("Program");
  settings.setValue("profile_directory", QString::fromStdString(profiles_[i].value_text));
  settings.setValue("boot_on_startup", true);

  emit profileChosen();
  accept();
}


void WidgetProfiles::on_pushApply_clicked()
{
  QModelIndexList ixl = ui->tableProfiles->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  QSettings settings;
  settings.beginGroup("Program");
  settings.setValue("profile_directory", QString::fromStdString(profiles_[i].value_text));
  settings.setValue("boot_on_startup", false);

  emit profileChosen();
  accept();
}

void WidgetProfiles::on_pushApplyBoot_clicked()
{
  QModelIndexList ixl = ui->tableProfiles->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  QSettings settings;
  settings.beginGroup("Program");
  settings.setValue("profile_directory", QString::fromStdString(profiles_[i].value_text));
  settings.setValue("boot_on_startup", true);

  emit profileChosen();
  accept();
}

void WidgetProfiles::on_OutputDirFind_clicked()
{
  QSettings settings;
  settings.beginGroup("Program");
  QString settings_directory = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();

  QString dirName = QFileDialog::getExistingDirectory(this, "Open Directory", settings_directory,
                                                      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!dirName.isEmpty()) {
    settings_directory = QDir(dirName).absolutePath();
    settings.setValue("settings_directory", settings_directory);
    update_profiles();
  }
}
