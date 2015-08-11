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
#include "ui_dialog_spectrum_template.h"
#include <QFileDialog>
#include <QMessageBox>

DialogSpectrumTemplate::DialogSpectrumTemplate(Pixie::Spectrum::Template newTemplate, bool edit, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSpectrumTemplate)
{
  ui->setupUi(this);
  for (auto &q : Pixie::Spectrum::Factory::getInstance().types())
    ui->comboType->addItem(QString::fromStdString(q));
  ui->colPicker->setStandardColors();
  connect(ui->colPicker, SIGNAL(colorChanged(QColor)), this, SLOT(colorChanged(QColor)));

  QRegExp rx("^\\w*$");
  QValidator *validator = new QRegExpValidator(rx, this);
  ui->lineName->setValidator(validator);

  if (edit) {
    myTemplate = newTemplate;
    ui->lineName->setEnabled(false);
    ui->comboType->setEnabled(false);
    Pixie::Spectrum::Template *newtemp = Pixie::Spectrum::Factory::getInstance().create_template(newTemplate.type);
    if (newtemp != nullptr) {
      myTemplate.description = newtemp->description;
      myTemplate.input_types = newtemp->input_types;
      myTemplate.output_types = newtemp->output_types;
    } else
      PL_WARN << "Problem with spectrum type. Factory cannot make template for " << newTemplate.type;
  } else {
    Pixie::Spectrum::Template *newtemp = Pixie::Spectrum::Factory::getInstance().create_template(ui->comboType->currentText().toStdString());
    if (newtemp != nullptr) {
      myTemplate = *newtemp;
    } else
      PL_WARN << "Problem with spectrum type. Factory cannot make template for " << ui->comboType->currentText().toStdString();
    myTemplate.appearance = generateColor().rgba();
  }

  table_model_.eat(&myTemplate.generic_attributes);
  ui->tableGenericAttrs->setModel(&table_model_);
  ui->tableGenericAttrs->setItemDelegate(&special_delegate_);
  ui->tableGenericAttrs->verticalHeader()->hide();
  ui->tableGenericAttrs->horizontalHeader()->setStretchLastSection(true);
  ui->tableGenericAttrs->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableGenericAttrs->show();

  updateData();

  ui->tableGenericAttrs->resizeColumnsToContents();

}

void DialogSpectrumTemplate::updateData() {

  ui->lineName->setText(QString::fromStdString(myTemplate.name_));
  ui->comboType->setCurrentText(QString::fromStdString(myTemplate.type));
  ui->checkBox->setChecked(myTemplate.visible);
  ui->spinBits->setValue(myTemplate.bits);
  ui->lineChannels->setText(QString::number(pow(2,myTemplate.bits)));

  ui->colPicker->setCurrentColor(QColor::fromRgba(myTemplate.appearance));

  ui->patternMatch->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(myTemplate.match_pattern), 25, true, 16));
  ui->patternAdd->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(myTemplate.add_pattern), 25, false, 16));

  ui->patternMatch->setMinimumSize(ui->patternMatch->sizeHint());
  ui->patternAdd->setMinimumSize(ui->patternAdd->sizeHint());

  QString descr = QString::fromStdString(myTemplate.description) + "\n";
  if (myTemplate.output_types.size()) {
    descr += "\nOutput file types: ";
    for (auto &q : myTemplate.output_types) {
      descr += "*." + QString::fromStdString(q);
      if (q != myTemplate.output_types.back())
        descr += ", ";
    }
  }

  ui->labelDescription->setText(descr);
  table_model_.update();
  ui->patternAdd->update();
  ui->patternMatch->update();
}

DialogSpectrumTemplate::~DialogSpectrumTemplate()
{
  delete ui;
}

void DialogSpectrumTemplate::on_buttonBox_accepted()
{
  myTemplate.match_pattern = ui->patternMatch->qpxPattern().pattern().toStdVector();
  myTemplate.add_pattern = ui->patternAdd->qpxPattern().pattern().toStdVector();

  if (myTemplate.name_ == Pixie::Spectrum::Template().name_) {
    QMessageBox msgBox;
    msgBox.setText("Please give it a proper name");
    msgBox.exec();
  } else {
    PL_INFO << "Type requested " << myTemplate.type;
    Pixie::Spectrum::Spectrum *newSpectrum = Pixie::Spectrum::Factory::getInstance().create_from_template(myTemplate);
    if (newSpectrum == nullptr) {
      QMessageBox msgBox;
      msgBox.setText("Failed to create spectrum from template. Check requirements.");
      msgBox.exec();
    } else {
      delete newSpectrum;
      emit(templateReady(myTemplate));
      accept();
    }
  }
}

void DialogSpectrumTemplate::on_buttonBox_rejected()
{
  reject();
}

void DialogSpectrumTemplate::on_lineName_editingFinished()
{
  myTemplate.name_ = ui->lineName->text().toStdString();
}

void DialogSpectrumTemplate::on_checkBox_clicked()
{
  myTemplate.visible = ui->checkBox->isChecked();
}

void DialogSpectrumTemplate::on_spinBits_valueChanged(int arg1)
{
  myTemplate.bits = arg1;
  ui->lineChannels->setText(QString::number(pow(2,arg1)));
}

void DialogSpectrumTemplate::colorChanged(const QColor &col)
{
  myTemplate.appearance = col.rgba();
}

void DialogSpectrumTemplate::on_comboType_activated(const QString &arg1)
{
  Pixie::Spectrum::Template *newtemp = Pixie::Spectrum::Factory::getInstance().create_template(arg1.toStdString());
  if (newtemp != nullptr) {
    myTemplate = *newtemp;

    //keep these from previous
    myTemplate.visible = ui->checkBox->isChecked();
    myTemplate.bits = ui->spinBits->value();
    myTemplate.name_ = ui->lineName->text().toStdString();
    myTemplate.appearance = ui->colPicker->currentColor().rgba();
    updateData();
  } else
    PL_WARN << "Problem with spectrum type. Factory refuses to make template for " << arg1.toStdString();
}

void DialogSpectrumTemplate::on_pushRandColor_clicked()
{
  myTemplate.appearance = generateColor().rgba();
  ui->colPicker->setCurrentColor(QColor::fromRgba(myTemplate.appearance));
}





TableSpectraTemplates::TableSpectraTemplates(XMLableDB<Pixie::Spectrum::Template>& templates, QObject *parent)
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
  return 8;
}

QVariant TableSpectraTemplates::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(templates_.get(row).name_);
    case 1:
      return QString::fromStdString(templates_.get(row).type);
    case 2:
      return QString::number(templates_.get(row).bits);
    case 3:
      return QString::number(pow(2,templates_.get(row).bits));
    case 4:
      return QVariant::fromValue(QpxPattern(QVector<int16_t>::fromStdVector(templates_.get(row).match_pattern), 20, true, 16));
    case 5:
      return QVariant::fromValue(QpxPattern(QVector<int16_t>::fromStdVector(templates_.get(row).add_pattern), 20, false, 16));
    case 6:
      return QColor::fromRgba(templates_.get(row).appearance);
    case 7:
      if (templates_.get(row).visible)
        return QString("yes");
      else
        return QString("no");
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
        return QString("bits");
      case 3:
        return QString("channels");
      case 4:
        return QString("match");
      case 5:
        return QString("add");
      case 6:
        return QString("appearance");
      case 7:
        return QString("default plot");
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






DialogSpectraTemplates::DialogSpectraTemplates(XMLableDB<Pixie::Spectrum::Template> &newdb, QString savedir, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSpectraTemplates),
  table_model_(newdb),
  selection_model_(&table_model_),
  templates_(newdb),
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
  ui->spectraSetupView->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->spectraSetupView->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

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
    ui->pushEdit->setEnabled(true);
    ui->pushDelete->setEnabled(true);
    QModelIndexList ixl = selection_model_.selectedRows();
    if (ixl.front().row() > 0)
      ui->pushUp->setEnabled(true);
    else
      ui->pushUp->setEnabled(false);
    if ((ixl.front().row() + 1) < templates_.size())
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
    PL_INFO << "Reading templates from xml file " << fileName.toStdString();
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
    QFileInfo file(fileName);
    if (file.suffix() != "tem")
      fileName += ".tem";
    PL_INFO << "Writing templates to xml file " << fileName.toStdString();
    templates_.write_xml(fileName.toStdString());
  }
}

void DialogSpectraTemplates::on_pushNew_clicked()
{
  Pixie::Spectrum::Template new_template;
  new_template.appearance = generateColor().rgba();
  DialogSpectrumTemplate* newDialog = new DialogSpectrumTemplate(new_template, false, this);
  connect(newDialog, SIGNAL(templateReady(Pixie::Spectrum::Template)), this, SLOT(add_template(Pixie::Spectrum::Template)));
  newDialog->exec();
}

void DialogSpectraTemplates::on_pushEdit_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  DialogSpectrumTemplate* newDialog = new DialogSpectrumTemplate(templates_.get(i), true, this);
  connect(newDialog, SIGNAL(templateReady(Pixie::Spectrum::Template)), this, SLOT(change_template(Pixie::Spectrum::Template)));
  newDialog->exec();
}

void DialogSpectraTemplates::on_pushDelete_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  templates_.remove(i);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}


void DialogSpectraTemplates::add_template(Pixie::Spectrum::Template newTemplate) {
  //notify if duplicate
  templates_.add(newTemplate);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::change_template(Pixie::Spectrum::Template newTemplate) {
  templates_.replace(newTemplate);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushSetDefault_clicked()
{
  //ask sure?
  templates_.write_xml(root_dir_.toStdString() + "/default_spectra.tem");
}

void DialogSpectraTemplates::on_pushUseDefault_clicked()
{
  //ask sure?
  templates_.clear();
  templates_.read_xml(root_dir_.toStdString() + "/default_spectra.tem");

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
