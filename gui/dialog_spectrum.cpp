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

dialog_spectrum::dialog_spectrum(Pixie::Spectrum::Spectrum &spec, QWidget *parent) :
  QDialog(parent),
  my_spectrum_(spec),
  changed_(false),
  ui(new Ui::dialog_spectrum)
{
  ui->setupUi(this);
  ui->patternAdd->setEnabled(false);
  ui->patternMatch->setEnabled(false);
  ui->labelWarning->setVisible(false);

  ui->colPicker->setStandardColors();
  connect(ui->colPicker, SIGNAL(colorChanged(QColor)), this, SLOT(setNewColor(QColor)));

  attributes_ = my_spectrum_.generic_attributes();

  table_model_.eat(&attributes_);
  ui->tableGenericAttrs->setModel(&table_model_);
  ui->tableGenericAttrs->setItemDelegate(&special_delegate_);
  ui->tableGenericAttrs->verticalHeader()->hide();
  ui->tableGenericAttrs->horizontalHeader()->setStretchLastSection(true);
  ui->tableGenericAttrs->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::NoSelection);
  ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableGenericAttrs->show();

  updateData();

  ui->tableGenericAttrs->resizeColumnsToContents();
}

dialog_spectrum::~dialog_spectrum()
{
  delete ui;
}


void dialog_spectrum::updateData() {
  ui->groupSpectrum->setTitle(QString::fromStdString(my_spectrum_.name()));
  ui->lineType->setText(QString::fromStdString(my_spectrum_.type()));
  ui->lineBits->setText(QString::number(static_cast<int>(my_spectrum_.bits())));
  ui->lineChannels->setText(QString::number(my_spectrum_.resolution()));
  ui->colPicker->setCurrentColor(QColor::fromRgba(my_spectrum_.appearance()));
  ui->patternMatch->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(my_spectrum_.match_pattern()), 25, true, 16));
  ui->patternAdd->setQpxPattern(QpxPattern(QVector<int16_t>::fromStdVector(my_spectrum_.add_pattern()), 25, false, 16));
  ui->patternMatch->setMinimumSize(ui->patternMatch->sizeHint());
  ui->patternAdd->setMinimumSize(ui->patternAdd->sizeHint());

  ui->lineLiveTime->setText(QString::fromStdString(to_simple_string(my_spectrum_.live_time())));
  ui->lineRealTime->setText(QString::fromStdString(to_simple_string(my_spectrum_.real_time())));
  ui->lineTotalCount->setText(QString::number(my_spectrum_.total_count()));

  ui->dateTimeStart->setDateTime(fromBoostPtime(my_spectrum_.start_time()));

  ui->lineDescription->setText(QString::fromStdString(my_spectrum_.description()));

  QString dets;
  for (auto &q : my_spectrum_.get_detectors()) {
    dets += QString::fromStdString(q.name_);
    dets += ", ";
  }

  ui->lineDetectors->setText(dets);
  table_model_.update();
  open_close_locks();
}

void dialog_spectrum::open_close_locks() {
  bool lockit = !ui->pushLock->isChecked();
  ui->labelWarning->setVisible(lockit);
  ui->colPicker->setEnabled(lockit);
  ui->pushRandColor->setEnabled(lockit);
  ui->lineDescription->setEnabled(lockit);
  ui->dateTimeStart->setEnabled(lockit);
  if (!lockit) {
    ui->tableGenericAttrs->clearSelection();
    ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  } else {
    ui->tableGenericAttrs->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableGenericAttrs->setEditTriggers(QAbstractItemView::AllEditTriggers);
    changed_ = true;
  }
}

void dialog_spectrum::on_pushRandColor_clicked()
{
  my_spectrum_.set_appearance(generateColor().rgba());
  updateData();
}

void dialog_spectrum::setNewColor(QColor col) {
  my_spectrum_.set_appearance(col.rgba());
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
  updateData();
}

void dialog_spectrum::on_lineDescription_editingFinished()
{
  my_spectrum_.set_description(ui->lineDescription->text().toStdString());
  updateData();
}
