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
#include "ui_dialog_detector.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include "project.h"
#include "daq_sink_factory.h"
#include <boost/algorithm/string.hpp>

using namespace Qpx;

DialogDetector::DialogDetector(Detector mydet, bool editName, QWidget *parent) :
  QDialog(parent),
  my_detector_(mydet),
  table_nrgs_(my_detector_.energy_calibrations_, false),
  selection_nrgs_(&table_nrgs_),
  table_gains_(my_detector_.gain_match_calibrations_, true),
  selection_gains_(&table_gains_),
  table_settings_model_(this),
  ui(new Ui::DialogDetector)
{
  ui->setupUi(this);

  QSettings settings;
  settings.beginGroup("Program");
  settings_directory_ = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();

  //file formats, should be in detector db widget
  std::vector<std::string> spectypes = SinkFactory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    Metadata type_template = SinkFactory::getInstance().create_prototype(q);
    if (!type_template.input_types().empty())
      filetypes.push_back("Spectrum " + QString::fromStdString(q) + "(" + catExtensions(type_template.input_types()) + ")");
  }
  mca_formats_ = catFileTypes(filetypes);

  QRegExp rx("^\\w*$");
  QValidator *validator = new QRegExpValidator(rx, this);
  ui->lineName->setValidator(validator);
  ui->lineName->setEnabled(editName);


  ui->comboType->insertItem(0, QString::fromStdString("none"), QString::fromStdString("none"));
  ui->comboType->insertItem(1, QString::fromStdString("HPGe"), QString::fromStdString("HPGe"));
  ui->comboType->insertItem(2, QString::fromStdString("NaI"), QString::fromStdString("NaI"));
  ui->comboType->insertItem(3, QString::fromStdString("LaBr"), QString::fromStdString("LaBr"));
  ui->comboType->insertItem(4, QString::fromStdString("BGO"), QString::fromStdString("BGO"));

  my_detector_ = mydet;

  ui->tableEnergyCalibrations->setModel(&table_nrgs_);
  ui->tableEnergyCalibrations->setSelectionModel(&selection_nrgs_);
  ui->tableEnergyCalibrations->verticalHeader()->hide();
  ui->tableEnergyCalibrations->horizontalHeader()->setStretchLastSection(true);
  ui->tableEnergyCalibrations->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableEnergyCalibrations->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableEnergyCalibrations->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableEnergyCalibrations->show();

  ui->tableGainCalibrations->setModel(&table_gains_);
  ui->tableGainCalibrations->setSelectionModel(&selection_gains_);
  ui->tableGainCalibrations->verticalHeader()->hide();
  ui->tableGainCalibrations->horizontalHeader()->setStretchLastSection(true);
  ui->tableGainCalibrations->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGainCalibrations->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGainCalibrations->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableGainCalibrations->show();

  ui->tableSettings->setModel(&table_settings_model_);
  ui->tableSettings->setItemDelegate(&table_settings_delegate_);
  ui->tableSettings->horizontalHeader()->setStretchLastSection(true);
  ui->tableSettings->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  table_settings_model_.set_show_read_only(true);
  ui->tableSettings->setEditTriggers(QAbstractItemView::AllEditTriggers);
  ui->tableSettings->show();


  connect(&selection_nrgs_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  connect(&selection_gains_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  updateDisplay();
}

DialogDetector::~DialogDetector()
{
  delete ui;
}

void DialogDetector::updateDisplay()
{
  if (my_detector_.name_ != Detector().name_)
    ui->lineName->setText(QString::fromStdString(my_detector_.name_));
  else
    ui->lineName->clear();

  ui->comboType->setCurrentText(QString::fromStdString(my_detector_.type_));

  std::vector<Detector> tablesets;
  tablesets.push_back(my_detector_);
  table_settings_model_.update(tablesets);


  if (my_detector_.fwhm_calibration_.valid())
    ui->labelFWHM->setText(QString::fromStdString(my_detector_.fwhm_calibration_.fancy_equation()));
  else
    ui->labelFWHM->setText("");
  ui->pushClearFWHM->setEnabled(my_detector_.fwhm_calibration_.valid());

  if (my_detector_.efficiency_calibration_.valid())
    ui->labelEfficiency->setText(QString::fromStdString(my_detector_.efficiency_calibration_.fancy_equation()));
  else
    ui->labelEfficiency->setText("");
  ui->pushClearEfficiency->setEnabled(my_detector_.efficiency_calibration_.valid());

  table_nrgs_.update();
  table_gains_.update();
}

void DialogDetector::on_lineName_editingFinished()
{
  my_detector_.name_ = ui->lineName->text().toStdString();
}

void DialogDetector::on_comboType_currentIndexChanged(const QString &)
{
  my_detector_.type_ = ui->comboType->currentText().toStdString();
}

void DialogDetector::on_buttonBox_accepted()
{
  if (my_detector_.name_ == Detector().name_) {
    QMessageBox msgBox;
    msgBox.setText("Please give it a proper name");
    msgBox.exec();
  } else
    emit newDetReady(my_detector_);
}


void DialogDetector::on_pushReadOpti_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", settings_directory_.absolutePath(),
                                                  "Qpx project file (*.qpx)");
  if (!validateFile(this, fileName, false))
    return;

  Project optiSource;
  optiSource.read_xml(fileName.toStdString(), false);
  std::set<Spill> spills = optiSource.spills();
  if (spills.size()) {
    Spill sp = *spills.rbegin();
    for (auto &q : sp.detectors)
      if (q.name_ == my_detector_.name_)
        my_detector_ = q;
  }
  updateDisplay();
}

void DialogDetector::on_pushRead1D_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load spectrum", settings_directory_.absolutePath(), mca_formats_);

  if (!validateFile(this, fileName, false))
    return;

  this->setCursor(Qt::WaitCursor);

  SinkPtr newSpectrum = SinkFactory::getInstance().create_from_file(fileName.toStdString());
  if (newSpectrum != nullptr) {
    std::vector<Detector> dets = newSpectrum->metadata().detectors;
    for (auto &q : dets)
      if (q != Detector()) {
        LINFO << "Looking at calibrations from detector " << q.name_;
        for (auto &p : q.energy_calibrations_.my_data_) {
          if (p.bits_) {
            my_detector_.energy_calibrations_.add(p);
            my_detector_.energy_calibrations_.replace(p); //ask user
          }
        }
      }
    updateDisplay();
  } else
    LINFO << "Spectrum construction did not succeed. Aborting";

  this->setCursor(Qt::ArrowCursor);
}

void DialogDetector::selection_changed(QItemSelection, QItemSelection)
{
  if (selection_nrgs_.selectedIndexes().empty())
    ui->pushRemove->setEnabled(false);
  else
    ui->pushRemove->setEnabled(true);

  if (selection_gains_.selectedIndexes().empty())
    ui->pushRemoveGain->setEnabled(false);
  else
    ui->pushRemoveGain->setEnabled(true);
}

void DialogDetector::on_pushRemove_clicked()
{
  QModelIndexList ixl = ui->tableEnergyCalibrations->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  my_detector_.energy_calibrations_.remove(i);
  table_nrgs_.update();
}

void DialogDetector::on_pushRemoveGain_clicked()
{
  QModelIndexList ixl = ui->tableGainCalibrations->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  my_detector_.gain_match_calibrations_.remove(i);
  table_gains_.update();
}

void DialogDetector::on_pushClearFWHM_clicked()
{
  my_detector_.fwhm_calibration_ = Calibration();
  updateDisplay();
}

void DialogDetector::on_pushClearEfficiency_clicked()
{
  my_detector_.efficiency_calibration_ = Calibration();
  updateDisplay();
}





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
      return QString::fromStdString(myDB->get(row).name_);
    case 1:
      return QString::fromStdString(myDB->get(row).type_);
    case 2:
      if (myDB->get(row).settings_.branches.empty())
        return "none";
      else
        return "valid";
    case 3:
      if (myDB->get(row).fwhm_calibration_.valid())
        return "valid";
      else
        return "none";
    case 4:
      if (myDB->get(row).efficiency_calibration_.valid())
        return "valid";
      else
        return "none";
    case 5:
      dss.str(std::string());
      for (auto &q : myDB->get(row).energy_calibrations_.my_data_) {
        dss << q.bits_ << " ";
      }
      return QString::fromStdString(dss.str());
    case 6:
      dss.str(std::string());
      for (auto &q : myDB->get(row).gain_match_calibrations_.my_data_) {
        dss << q.to_ << "/" << q.bits_ << " ";
      }
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




int TableCalibrations::rowCount(const QModelIndex & /*parent*/) const
{
  return myDB.size();
}

int TableCalibrations::columnCount(const QModelIndex & /*parent*/) const
{
  return 4;
}

QVariant TableCalibrations::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();
  std::stringstream dss;

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QString::number(myDB.get(row).bits_);
    case 1:
      if (gain_)
        return QString::fromStdString(myDB.get(row).to_);
      else
        return QString::fromStdString(myDB.get(row).units_);
    case 2:
      return QString::fromStdString(boost::gregorian::to_simple_string(myDB.get(row).calib_date_.date()));
    case 3:
      return QString::fromStdString(myDB.get(row).fancy_equation());
    }
  }
  return QVariant();
}

QVariant TableCalibrations::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return "Bits";
      case 1:
        if (gain_)
          return "Transforms to";
        else
          return "Units";
      case 2:
        return "Date";
      case 3:
        return "Coefficients";
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}


void TableCalibrations::update()
{
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, 3 );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableCalibrations::flags(const QModelIndex &index) const
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
