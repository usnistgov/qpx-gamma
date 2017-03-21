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

#include "dialog_detector.h"
#include "ui_dialog_detector.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include "project.h"
#include "consumer_factory.h"
#include <boost/algorithm/string.hpp>
#include "qt_util.h"

using namespace Qpx;

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
      return QString::number(myDB.get(row).bits());
    case 1:
      if (gain_)
        return QString::fromStdString(myDB.get(row).to());
      else
        return QString::fromStdString(myDB.get(row).units());
    case 2:
      return QString::fromStdString(boost::gregorian::to_simple_string(myDB.get(row).calib_date().date()));
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


void TableCalibrations::update(const XMLableDB<Calibration> &db, bool gain)
{
  myDB = db;
  gain_ = gain;
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, 3 );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableCalibrations::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}




DialogDetector::DialogDetector(Detector mydet, bool editName, QWidget *parent) :
  QDialog(parent),
  my_detector_(mydet),
  table_nrgs_(this),
  selection_nrgs_(&table_nrgs_),
  table_gains_(this),
  selection_gains_(&table_gains_),
  table_settings_model_(this),
  ui(new Ui::DialogDetector)
{
  ui->setupUi(this);

  QSettings settings;
  settings.beginGroup("Program");
  settings_directory_ = settings.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();

  //file formats, should be in detector db widget
  std::vector<std::string> spectypes = ConsumerFactory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    ConsumerMetadata type_template = ConsumerFactory::getInstance().create_prototype(q);
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
  table_nrgs_.update(mydet.energy_calibrations(), false);

  ui->tableGainCalibrations->setModel(&table_gains_);
  ui->tableGainCalibrations->setSelectionModel(&selection_gains_);
  ui->tableGainCalibrations->verticalHeader()->hide();
  ui->tableGainCalibrations->horizontalHeader()->setStretchLastSection(true);
  ui->tableGainCalibrations->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGainCalibrations->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGainCalibrations->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableGainCalibrations->show();
  table_gains_.update(mydet.gain_calibrations(), true);

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
  if (my_detector_.name() != Detector().name())
    ui->lineName->setText(QString::fromStdString(my_detector_.name()));
  else
    ui->lineName->clear();

  ui->comboType->setCurrentText(QString::fromStdString(my_detector_.type()));

  std::vector<Detector> tablesets;
  tablesets.push_back(my_detector_);
  table_settings_model_.update(tablesets);


  if (my_detector_.resolution().valid())
    ui->labelFWHM->setText(QString::fromStdString(my_detector_.resolution().fancy_equation()));
  else
    ui->labelFWHM->setText("");
  ui->pushClearFWHM->setEnabled(my_detector_.resolution().valid());

  if (my_detector_.efficiency().valid())
    ui->labelEfficiency->setText(QString::fromStdString(my_detector_.efficiency().fancy_equation()));
  else
    ui->labelEfficiency->setText("");
  ui->pushClearEfficiency->setEnabled(my_detector_.efficiency().valid());

  table_nrgs_.update(my_detector_.energy_calibrations(), false);
  table_gains_.update(my_detector_.gain_calibrations(), true);
}

void DialogDetector::on_lineName_editingFinished()
{
  my_detector_.set_name(ui->lineName->text().toStdString());
}

void DialogDetector::on_comboType_currentIndexChanged(const QString &)
{
  my_detector_.set_type(ui->comboType->currentText().toStdString());
}

void DialogDetector::on_buttonBox_accepted()
{
  if (my_detector_.name() == Detector().name())
  {
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
  optiSource.open(fileName.toStdString(), false);
  std::set<Spill> spills = optiSource.spills();
  if (spills.size()) {
    Spill sp = *spills.rbegin();
    for (auto &q : sp.detectors)
      if (q.name() == my_detector_.name())
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

  SinkPtr newSpectrum = ConsumerFactory::getInstance().create_from_file(fileName.toStdString());
  if (newSpectrum != nullptr) {
    std::vector<Detector> dets = newSpectrum->metadata().detectors;
    for (auto &q : dets)
      if (q != Detector())
        for (auto &p : q.energy_calibrations().my_data_)
          if (p.bits())
            my_detector_.set_energy_calibration(p);
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
  my_detector_.remove_energy_calibration(ixl.front().row());
  table_nrgs_.update(my_detector_.energy_calibrations(), false);
}

void DialogDetector::on_pushRemoveGain_clicked()
{
  QModelIndexList ixl = ui->tableGainCalibrations->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  my_detector_.remove_gain_calibration(ixl.front().row());
  table_gains_.update(my_detector_.gain_calibrations(), true);
}

void DialogDetector::on_pushClearFWHM_clicked()
{
  my_detector_.set_resolution_calibration(Calibration());
  updateDisplay();
}

void DialogDetector::on_pushClearEfficiency_clicked()
{
  my_detector_.set_efficiency_calibration(Calibration());
  updateDisplay();
}


