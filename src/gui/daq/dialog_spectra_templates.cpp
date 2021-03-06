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
 *      DialogSpectrumTemplate - edit dialog for spectrum template
 *      TableSpectraTemplates - table of spectrum templates
 *      DialogSpectraTemplates - dialog for managing spectrum templates
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "dialog_spectra_templates.h"
#include "ui_dialog_spectra_templates.h"
#include "dialog_spectrum.h"
#include <QFileDialog>
#include <QMessageBox>

using namespace Qpx;

TableSpectraTemplates::TableSpectraTemplates(XMLableDB<ConsumerMetadata>& templates, QObject *parent)
  : QAbstractTableModel(parent),
    templates_(templates)
{
}

int TableSpectraTemplates::rowCount(const QModelIndex & /*parent*/) const
{
  return templates_.size();
}

int TableSpectraTemplates::columnCount(const QModelIndex & /*parent*/) const
{
  return 6;
}

QVariant TableSpectraTemplates::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::BackgroundColorRole)
  {
    if (col == 0)
    {
      if (templates_.get(row).get_attribute("visible").value_int)
        return QColor(QString::fromStdString(templates_.get(row).get_attribute("appearance").value_text));
      else
        return QColor(Qt::black);
    }
    else
      return QVariant();
  }
  else if (role == Qt::ForegroundRole)
  {
    if (col == 0)
      return QColor(Qt::white);
    else
      return QVariant();
  }
  else if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(" " + templates_.get(row).get_attribute("name").value_text + " ");
    case 1:
      return QString::fromStdString(templates_.get(row).type());
    case 2:
      return QVariant::fromValue(templates_.get(row).get_attribute("resolution"));
    case 3:
      return QVariant::fromValue(templates_.get(row).get_attribute("pattern_coinc"));
    case 4:
      return QVariant::fromValue(templates_.get(row).get_attribute("pattern_anti"));
    case 5:
      return QVariant::fromValue(templates_.get(row).get_attribute("pattern_add"));
    }
  }
  return QVariant();
}

QVariant TableSpectraTemplates::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("name");
      case 1:
        return QString("type");
      case 2:
        return QString("resolution");
      case 3:
        return QString("coinc");
      case 4:
        return QString("anti");
      case 5:
        return QString("add");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableSpectraTemplates::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex(templates_.size(), columnCount());
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableSpectraTemplates::flags(const QModelIndex &index) const
{
  Qt::ItemFlags myflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index) & ~Qt::ItemIsSelectable;
  return myflags;
}






DialogSpectraTemplates::DialogSpectraTemplates(XMLableDB<ConsumerMetadata> &newdb,
                                               std::vector<Detector> current_dets,
                                               QString savedir, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSpectraTemplates),
  table_model_(newdb),
  selection_model_(&table_model_),
  templates_(newdb),
  current_dets_(current_dets),
  root_dir_(savedir)
{
  ui->setupUi(this);

  ui->spectraSetupView->setModel(&table_model_);
  ui->spectraSetupView->setItemDelegate(&special_delegate_);
  ui->spectraSetupView->setSelectionModel(&selection_model_);
  ui->spectraSetupView->verticalHeader()->hide();
  ui->spectraSetupView->horizontalHeader()->setStretchLastSection(true);
  ui->spectraSetupView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->spectraSetupView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->spectraSetupView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->spectraSetupView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->spectraSetupView->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));
  connect(ui->spectraSetupView, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(selection_double_clicked(QModelIndex)));

  table_model_.update();
  ui->spectraSetupView->resizeColumnsToContents();
  ui->spectraSetupView->resizeRowsToContents();
}

DialogSpectraTemplates::~DialogSpectraTemplates()
{
  delete ui;
}

void DialogSpectraTemplates::selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
}

void DialogSpectraTemplates::toggle_push() {
  if (selection_model_.selectedIndexes().empty()) {
    ui->pushEdit->setEnabled(false);
    ui->pushDelete->setEnabled(false);
    ui->pushUp->setEnabled(false);
    ui->pushDown->setEnabled(false);
  } else {
    ui->pushDelete->setEnabled(true);
  }

  if (selection_model_.selectedRows().size() == 1) {
    ui->pushEdit->setEnabled(true);
    QModelIndexList ixl = selection_model_.selectedRows();
    if (ixl.front().row() > 0)
      ui->pushUp->setEnabled(true);
    else
      ui->pushUp->setEnabled(false);
    if ((ixl.front().row() + 1) < static_cast<int>(templates_.size()))
      ui->pushDown->setEnabled(true);
    else
      ui->pushDown->setEnabled(false);
  }

  if (templates_.empty())
    ui->pushExport->setEnabled(false);
  else
    ui->pushExport->setEnabled(true);

}

void DialogSpectraTemplates::on_pushImport_clicked()
{
  //ask clear or append?
  QString fileName = QFileDialog::getOpenFileName(this, "Load template spectra",
                                                  root_dir_, "Template set (*.tem)");
  if (validateFile(this, fileName, false)) {
    LINFO << "Reading templates from xml file " << fileName.toStdString();
    templates_.read_xml(fileName.toStdString());
    selection_model_.reset();
    table_model_.update();
    toggle_push();

    ui->spectraSetupView->horizontalHeader()->setStretchLastSection(true);
    ui->spectraSetupView->resizeColumnsToContents();
  }
}

void DialogSpectraTemplates::on_pushExport_clicked()
{
  QString fileName = CustomSaveFileDialog(this, "Save template spectra",
                                          root_dir_, "Template set (*.tem)");
  if (validateFile(this, fileName, true)) {
    LINFO << "Writing templates to xml file " << fileName.toStdString();
    templates_.write_xml(fileName.toStdString());
  }
}

void DialogSpectraTemplates::on_pushNew_clicked()
{
  XMLableDB<Qpx::Detector> fakeDetDB("Detectors");
  DialogSpectrum* newDialog = new DialogSpectrum(ConsumerMetadata(), current_dets_, fakeDetDB, false, true, this);
  if (newDialog->exec()) {
    templates_.add_a(newDialog->product());
    selection_model_.reset();
    table_model_.update();
    toggle_push();
  }
}

void DialogSpectraTemplates::on_pushEdit_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  XMLableDB<Qpx::Detector> fakeDetDB("Detectors");
  DialogSpectrum* newDialog = new DialogSpectrum(templates_.get(i), current_dets_, fakeDetDB, false, true, this);
  if (newDialog->exec())
  {
    templates_.replace(i, newDialog->product());
    selection_model_.reset();
    table_model_.update();
    toggle_push();
  }
}

void DialogSpectraTemplates::selection_double_clicked(QModelIndex)
{
  on_pushEdit_clicked();
}

void DialogSpectraTemplates::on_pushDelete_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;

  std::list<ConsumerMetadata> torem;

  for (auto &ix : ixl) {
    int i = ix.row();
    torem.push_back(templates_.get(i));
  }

  for (auto &q : torem)
    templates_.remove_a(q);

  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushSetDefault_clicked()
{
  //ask sure?
  templates_.write_xml(root_dir_.toStdString() + "/default_sinks.tem");
}

void DialogSpectraTemplates::on_pushUseDefault_clicked()
{
  //ask sure?
  templates_.clear();
  templates_.read_xml(root_dir_.toStdString() + "/default_sinks.tem");

  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushClear_clicked()
{
  templates_.clear();
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushUp_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  templates_.up(ixl.front().row());
  selection_model_.setCurrentIndex(ixl.front().sibling(ixl.front().row()-1, ixl.front().column()),
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushDown_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  templates_.down(ixl.front().row());
  selection_model_.setCurrentIndex(ixl.front().sibling(ixl.front().row()+1, ixl.front().column()),
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  table_model_.update();
  toggle_push();
}
