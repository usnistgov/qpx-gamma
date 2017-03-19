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
 *      DialogDetector - dialog for editing the definition of detector
 *      TableDetectors - table model for detector set
 *      WidgetDetectors - dialog for managing detector set
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "widget_detectors.h"
#include "ui_widget_detectors.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include "project.h"
#include "consumer_factory.h"
#include <boost/algorithm/string.hpp>

#include "dialog_detector.h"
#include "qt_util.h"

using namespace Qpx;


int TableDetectors::rowCount(const QModelIndex & /*parent*/) const
{    return myDB->size(); }

int TableDetectors::columnCount(const QModelIndex & /*parent*/) const
{    return 7; }

QVariant TableDetectors::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();
  std::stringstream dss;

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QString::fromStdString(myDB->get(row).name());
    case 1:
      return QString::fromStdString(myDB->get(row).type());
    case 2:
      if (myDB->get(row).optimizations().empty())
        return "none";
      else
        return "present";
    case 3:
      if (myDB->get(row).resolution().valid())
        return "valid";
      else
        return "none";
    case 4:
      if (myDB->get(row).efficiency().valid())
        return "valid";
      else
        return "none";
    case 5:
      dss.str(std::string());
      for (auto &q : myDB->get(row).energy_calibrations().my_data_)
        dss << q.bits() << " ";
      return QString::fromStdString(dss.str());
    case 6:
      dss.str(std::string());
      for (auto &q : myDB->get(row).gain_calibrations().my_data_)
        dss << q.to() << "/" << q.bits() << " ";
      return QString::fromStdString(dss.str());
    }
  }
  return QVariant();
}

QVariant TableDetectors::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return "Name";
      case 1:
        return "Type";
      case 2:
        return "Device settings";
      case 3:
        return "FWHM";
      case 4:
        return "Efficiency";
      case 5:
        return "Energy calibrations (bits)";
      case 6:
        return "Gain matching calibrations (Destination/bits)";
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableDetectors::update()
{
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1 );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableDetectors::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}





WidgetDetectors::WidgetDetectors(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::WidgetDetectors)
  , selection_model_(&table_model_)
{
  ui->setupUi(this);

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  connect(ui->tableDetectorDB, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(selection_double_clicked(QModelIndex)));

}

void WidgetDetectors::setData(XMLableDB<Detector> &newdb)
{
  table_model_.setDB(newdb);
  detectors_ = &newdb;

  QSettings settings;
  settings.beginGroup("Program");
  settings_directory_ = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();

  ui->tableDetectorDB->setModel(&table_model_);
  ui->tableDetectorDB->setSelectionModel(&selection_model_);
  ui->tableDetectorDB->verticalHeader()->hide();
  ui->tableDetectorDB->horizontalHeader()->setStretchLastSection(true);
  ui->tableDetectorDB->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableDetectorDB->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableDetectorDB->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableDetectorDB->show();
}

WidgetDetectors::~WidgetDetectors()
{
  delete ui;
}

void WidgetDetectors::selection_changed(QItemSelection, QItemSelection)
{
  toggle_push();
}

void WidgetDetectors::toggle_push()
{
  if (selection_model_.selectedIndexes().empty())
  {
    ui->pushEdit->setEnabled(false);
    ui->pushDelete->setEnabled(false);
  } else {
    ui->pushEdit->setEnabled(true);
    ui->pushDelete->setEnabled(true);
  }
  emit detectorsUpdated();
}

void WidgetDetectors::on_pushNew_clicked()
{
  DialogDetector* newDet = new DialogDetector(Detector(), true, this);
  connect(newDet, SIGNAL(newDetReady(Qpx::Detector)), this, SLOT(addNewDet(Qpx::Detector)));
  newDet->exec();
}

void WidgetDetectors::on_pushEdit_clicked()
{
  QModelIndexList ixl = ui->tableDetectorDB->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  DialogDetector* newDet = new DialogDetector(detectors_->get(i), false, this);
  connect(newDet, SIGNAL(newDetReady(Qpx::Detector)), this, SLOT(addNewDet(Qpx::Detector)));
  newDet->exec();
}

void WidgetDetectors::selection_double_clicked(QModelIndex)
{
  on_pushEdit_clicked();
}

void WidgetDetectors::on_pushDelete_clicked()
{
  QItemSelectionModel *selected = ui->tableDetectorDB->selectionModel();
  QModelIndexList rowList = selected->selectedRows();
  if (rowList.empty())
    return;
  detectors_->remove(rowList.front().row());
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void WidgetDetectors::on_pushImport_clicked()
{
  // ask delete old or append
  QString fileName = QFileDialog::getOpenFileName(this, "Load detector settings",
                                                  settings_directory_, "Detector settings (*.det)");
  if (validateFile(this, fileName, false))
  {
    LINFO << "Reading detector settings from xml file " << fileName.toStdString();
    detectors_->read_xml(fileName.toStdString());
    selection_model_.reset();
    table_model_.update();
    toggle_push();
  }
}

void WidgetDetectors::on_pushExport_clicked()
{
  QString fileName = CustomSaveFileDialog(this, "Save detector settings",
                                          settings_directory_, "Detector settings (*.det)");
  if (validateFile(this, fileName, true))
  {
    QFileInfo file(fileName);
    LINFO << "Writing detector settings to xml file " << fileName.toStdString();
    detectors_->write_xml(fileName.toStdString());
  }
}

void WidgetDetectors::addNewDet(Detector newDetector) {
  detectors_->add(newDetector);
  detectors_->replace(newDetector);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void WidgetDetectors::on_pushSetDefault_clicked()
{
  //ask sure?
  detectors_->write_xml(settings_directory_.toStdString() + "/default_detectors.det");
}

void WidgetDetectors::on_pushGetDefault_clicked()
{
  //ask sure?
  detectors_->clear();
  detectors_->read_xml(settings_directory_.toStdString() + "/default_detectors.det");
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}
