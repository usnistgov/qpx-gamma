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
 *
 ******************************************************************************/

#include "dialog_spectrum.h"
#include "ui_dialog_spectrum.h"
#include "qt_util.h"
#include "custom_logger.h"
#include <QInputDialog>
#include <QMessageBox>

dialog_spectrum::dialog_spectrum(Qpx::Spectrum::Spectrum &spec, QWidget *parent) :
  QDialog(parent),
  my_spectrum_(spec),
  det_selection_model_(&det_table_model_),
  changed_(false),
  detectors_("Detectors"),
  attributes_("Attributes"),
  ui(new Ui::dialog_spectrum)
{
  ui->setupUi(this);
  ui->patternAdd->setEnabled(false);
  ui->patternMatch->setEnabled(false);
  ui->labelWarning->setVisible(false);
  ui->durationLive->setVisible(false);
  ui->durationReal->setVisible(false);

  ui->colPicker->setStandardColors();
  connect(ui->colPicker, SIGNAL(colorChanged(QColor)), this, SLOT(setNewColor(QColor)));

  connect(&det_selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(det_selection_changed(QItemSelection,QItemSelection)));

  connect(ui->durationLive, SIGNAL(editingFinished()), this, SLOT(durationLiveChanged()));
  connect(ui->durationReal, SIGNAL(editingFinished()), this, SLOT(durationRealChanged()));

  md_ = my_spectrum_.metadata();

  attributes_ = md_.attributes;

  table_model_.eat(&attributes_);
  ui->tableGenericAttrs->setModel(&table_model_);
  ui->tableGenericAttrs->setItemDelegate(&special_delegate_);
  ui->tableGenericAttrs->verticalHeader()->hide();
  ui->tableGenericAttrs->horizontalHeader()->setStretchLastSection(true);
  ui->tableGenericAttrs->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::NoSelection);
  ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableGenericAttrs->show();


  det_table_model_.setDB(detectors_);

  ui->tableDetectors->setModel(&det_table_model_);
  ui->tableDetectors->setSelectionModel(&det_selection_model_);
  ui->tableDetectors->verticalHeader()->hide();
  ui->tableDetectors->horizontalHeader()->setStretchLastSection(true);
  ui->tableDetectors->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableDetectors->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableDetectors->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableDetectors->show();

  updateData();

  ui->tableGenericAttrs->resizeColumnsToContents();
}

dialog_spectrum::~dialog_spectrum()
{
  delete ui;
}

void dialog_spectrum::det_selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
}

void dialog_spectrum::updateData() {
  md_ = my_spectrum_.metadata();

  ui->groupSpectrum->setTitle(QString::fromStdString(md_.name));
  ui->lineType->setText(QString::fromStdString(my_spectrum_.type()));
  ui->lineBits->setText(QString::number(static_cast<int>(md_.bits)));
  ui->lineChannels->setText(QString::number(md_.resolution));
  ui->colPicker->setCurrentColor(QColor::fromRgba(md_.appearance));
  ui->patternMatch->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(md_.match_pattern), 25, true, 16));
  ui->patternAdd->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(md_.add_pattern), 25, false, 16));
  ui->patternMatch->setMinimumSize(ui->patternMatch->sizeHint());
  ui->patternAdd->setMinimumSize(ui->patternAdd->sizeHint());
  ui->doubleRescaleFactor->setValue(md_.rescale_factor.convert_to<double>());

  ui->durationLive->set_total_seconds(md_.live_time.total_seconds());
  ui->durationReal->set_total_seconds(md_.real_time.total_seconds());
  ui->lineLiveTime->setText(QString::fromStdString(to_simple_string(md_.live_time)));
  ui->lineRealTime->setText(QString::fromStdString(to_simple_string(md_.real_time)));
  ui->lineTotalCount->setText(QString::number(md_.total_count.convert_to<double>()));

  ui->dateTimeStart->setDateTime(fromBoostPtime(md_.start_time));

  ui->lineDescription->setText(QString::fromStdString(md_.description));

  detectors_.clear();
  for (auto &q: md_.detectors)
    detectors_.add_a(q);
  det_table_model_.update();

  table_model_.update();
  open_close_locks();
}

void dialog_spectrum::open_close_locks() {
  bool lockit = !ui->pushLock->isChecked();
  ui->labelWarning->setVisible(lockit);
  ui->dateTimeStart->setEnabled(lockit);
  ui->pushDelete->setEnabled(lockit);
  ui->durationLive->setEnabled(lockit);
  ui->durationReal->setEnabled(lockit);

  ui->durationLive->setVisible(lockit);
  ui->durationReal->setVisible(lockit);
  ui->lineLiveTime->setVisible(!lockit);
  ui->lineRealTime->setVisible(!lockit);

  if (!lockit) {
    ui->tableGenericAttrs->clearSelection();
    ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableDetectors->clearSelection();
    ui->tableDetectors->setSelectionMode(QAbstractItemView::NoSelection);
  } else {
    ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->tableDetectors->setSelectionMode(QAbstractItemView::SingleSelection);
    changed_ = true;
  }
  toggle_push();
}

void dialog_spectrum::toggle_push()
{
  bool unlocked = !ui->pushLock->isChecked();
  ui->pushDetEdit->setEnabled(unlocked);
  ui->pushDetRename->setEnabled(unlocked);
}

void dialog_spectrum::on_pushRandColor_clicked()
{
  my_spectrum_.set_appearance(generateColor().rgba());
  changed_ = true;
  updateData();
}

void dialog_spectrum::setNewColor(QColor col) {
  my_spectrum_.set_appearance(col.rgba());
  changed_ = true;
  updateData();
}

void dialog_spectrum::durationLiveChanged() {
  my_spectrum_.set_live_time(boost::posix_time::seconds(ui->durationLive->total_seconds()));
  changed_ = true;
  updateData();
}

void dialog_spectrum::durationRealChanged() {
  my_spectrum_.set_real_time(boost::posix_time::seconds(ui->durationReal->total_seconds()));
  changed_ = true;
  updateData();
}

void dialog_spectrum::on_pushLock_clicked()
{
  open_close_locks();
}

void dialog_spectrum::on_buttonBox_rejected()
{
  emit finished(changed_);
  accept();
}

void dialog_spectrum::on_dateTimeStart_editingFinished()
{
  my_spectrum_.set_start_time(fromQDateTime(ui->dateTimeStart->dateTime()));
  changed_ = true;
  updateData();
}

void dialog_spectrum::on_lineDescription_editingFinished()
{
  my_spectrum_.set_description(ui->lineDescription->text().toStdString());
  changed_ = true;
  updateData();
}

void dialog_spectrum::on_pushDetEdit_clicked()
{
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  DialogDetector* newDet = new DialogDetector(detectors_.get(i), QDir("~/"), false, this);
  connect(newDet, SIGNAL(newDetReady(Gamma::Detector)), this, SLOT(changeDet(Gamma::Detector)));
  newDet->exec();
}

void dialog_spectrum::changeDet(Gamma::Detector newDetector) {
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  if (i < md_.detectors.size()) {
    md_.detectors[i] = newDetector;
    my_spectrum_.set_detectors(md_.detectors);
  }

  detectors_.replace(newDetector);
  det_table_model_.update();
  toggle_push();
}

void dialog_spectrum::on_pushDetRename_clicked()
{
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  bool ok;
  QString text = QInputDialog::getText(this, "Rename Detector",
                                       "Detector name:", QLineEdit::Normal,
                                       QString::fromStdString(detectors_.get(i).name_),
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (i < md_.detectors.size()) {
      md_.detectors[i].name_ = text.toStdString();
      my_spectrum_.set_detectors(md_.detectors);
      changed_ = true;
      updateData();
      toggle_push();
    }
  }
}

void dialog_spectrum::on_pushDelete_clicked()
{
  int ret = QMessageBox::question(this, "Delete spectrum?", "Are you sure you want to delete this spectrum?");
  if (ret == QMessageBox::Yes) {
    emit delete_spectrum();
    accept();
  }
}

void dialog_spectrum::on_pushAnalyse_clicked()
{
  emit analyse();
  accept();
}

void dialog_spectrum::on_doubleRescaleFactor_valueChanged(double arg1)
{
  my_spectrum_.set_rescale_factor(PreciseFloat(arg1));
  changed_ = true;
  updateData();
}
