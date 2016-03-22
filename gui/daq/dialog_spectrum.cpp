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

dialog_spectrum::dialog_spectrum(Qpx::Spectrum::Spectrum &spec, XMLableDB<Qpx::Detector>& detDB, QWidget *parent) :
  QDialog(parent),
  my_spectrum_(spec),
  det_selection_model_(&det_table_model_),
  changed_(false),
  attr_model_(this),
  detectors_(detDB),
  spectrum_detectors_("Detectors"),
  ui(new Ui::dialog_spectrum)
{
  ui->setupUi(this);
  ui->labelWarning->setVisible(false);

  connect(&det_selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(det_selection_changed(QItemSelection,QItemSelection)));

  connect(&attr_model_, SIGNAL(tree_changed()), this, SLOT(push_settings()));

  md_ = my_spectrum_.metadata();

  ui->treeAttribs->setModel(&attr_model_);
  ui->treeAttribs->setItemDelegate(&attr_delegate_);
  ui->treeAttribs->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

  det_table_model_.setDB(spectrum_detectors_);

  ui->tableDetectors->setModel(&det_table_model_);
  ui->tableDetectors->setSelectionModel(&det_selection_model_);
  ui->tableDetectors->verticalHeader()->hide();
  ui->tableDetectors->horizontalHeader()->setStretchLastSection(true);
  ui->tableDetectors->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableDetectors->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableDetectors->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableDetectors->show();

  attr_model_.set_show_address_(false);

  Qpx::Setting pat = md_.attributes.branches.get(Qpx::Setting("pattern_add"));
  ui->spinDets->setValue(pat.value_pattern.gates().size());

  updateData();
}

dialog_spectrum::~dialog_spectrum()
{
  delete ui;
}

void dialog_spectrum::det_selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
}

void dialog_spectrum::updateData() {
//  md_ = my_spectrum_.metadata();

  ui->labelName->setText(QString::fromStdString(md_.name));
  ui->lineType->setText(QString::fromStdString(md_.type()));
  ui->lineBits->setText(QString::number(static_cast<int>(md_.bits)));
  ui->lineChannels->setText(QString::number(pow(2,md_.bits)));

  ui->lineTotalCount->setText(QString::number(md_.total_count.convert_to<double>()));

  spectrum_detectors_.clear();
  for (auto &q: md_.detectors)
    spectrum_detectors_.add_a(q);
  det_table_model_.update();

  QString descr = "[dim:" + QString::number(md_.dimensions()) + "] " + QString::fromStdString(md_.type_description()) + "\n";

  if (md_.output_types().size()) {
    descr += "\t\tOutput file types: ";
    for (auto &q : md_.output_types()) {
      descr += "*." + QString::fromStdString(q);
      if (q != md_.output_types().back())
        descr += ", ";
    }
  }

  ui->labelDescription->setText(descr);

  attr_model_.update(md_.attributes);
  open_close_locks();
}

void dialog_spectrum::open_close_locks() {
  bool lockit = !ui->pushLock->isChecked();
  ui->labelWarning->setVisible(lockit);
  ui->pushDelete->setEnabled(lockit);
  ui->spinDets->setEnabled(lockit);

  ui->treeAttribs->clearSelection();
  ui->tableDetectors->clearSelection();
  if (!lockit) {
    attr_model_.set_edit_read_only(false);
    ui->tableDetectors->setSelectionMode(QAbstractItemView::NoSelection);
  } else {
    attr_model_.set_edit_read_only(true);
    ui->tableDetectors->setSelectionMode(QAbstractItemView::SingleSelection);
    changed_ = true;
  }
  toggle_push();
}

void dialog_spectrum::push_settings() {
  md_.attributes = attr_model_.get_tree();
  changed_ = true;
}

void dialog_spectrum::toggle_push()
{
  ui->pushDetEdit->setEnabled(false);
  ui->pushDetRename->setEnabled(false);
  ui->pushDetFromDB->setEnabled(false);
  ui->pushDetToDB->setEnabled(false);

  bool unlocked = !ui->pushLock->isChecked();

  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  if (i < md_.detectors.size()) {
    ui->pushDetEdit->setEnabled(unlocked);
    ui->pushDetRename->setEnabled(unlocked);
    ui->pushDetToDB->setEnabled(unlocked);
    Qpx::Detector det = md_.detectors[i];
    if (unlocked && detectors_.has_a(det))
      ui->pushDetFromDB->setEnabled(true);
  }
}

void dialog_spectrum::on_pushLock_clicked()
{
  open_close_locks();
}

void dialog_spectrum::on_buttonBox_rejected()
{
  if (changed_) {
    my_spectrum_.set_generic_attrs(md_.attributes);
    my_spectrum_.set_detectors(md_.detectors);
  }

  emit finished(changed_);
  accept();
}

void dialog_spectrum::on_pushDetEdit_clicked()
{
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  DialogDetector* newDet = new DialogDetector(spectrum_detectors_.get(i), QDir("~/"), false, this);
  connect(newDet, SIGNAL(newDetReady(Qpx::Detector)), this, SLOT(changeDet(Qpx::Detector)));
  newDet->exec();
}

void dialog_spectrum::changeDet(Qpx::Detector newDetector) {
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  if (i < md_.detectors.size()) {
    md_.detectors[i] = newDetector;
    changed_ = true;
    updateData();
  }
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
                                       QString::fromStdString(spectrum_detectors_.get(i).name_),
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (i < md_.detectors.size()) {
      md_.detectors[i].name_ = text.toStdString();
      changed_ = true;
      updateData();
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

void dialog_spectrum::on_pushDetFromDB_clicked()
{
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  if (i < md_.detectors.size()) {
    Qpx::Detector newdet = detectors_.get(md_.detectors[i]);
    md_.detectors[i] = newdet;
    changed_ = true;
    updateData();
  }
}

void dialog_spectrum::on_pushDetToDB_clicked()
{
  QModelIndexList ixl = ui->tableDetectors->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();

  if (i < md_.detectors.size()) {
    Qpx::Detector newdet = md_.detectors[i];

    if (!detectors_.has_a(newdet)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(newdet.name_),
                                           &ok);

      if (!ok)
        return;

      if (!text.isEmpty()) {
        if (text.toStdString() != newdet.name_)
          newdet.name_ = text.toStdString();

        if (detectors_.has_a(newdet)) {
          QMessageBox::StandardButton reply = QMessageBox::question(this, "Replace existing?",
                                        "Detector " + text + " already exists. Replace?",
                                         QMessageBox::Yes|QMessageBox::No);
          if (reply == QMessageBox::No)
            return;
          else {
            detectors_.replace(newdet);
            if (md_.detectors[i].name_ != newdet.name_) {
              md_.detectors[i] = newdet;
              changed_ = true;
              updateData();
            }
          }
        } else
          detectors_.replace(newdet);
      }
    } else {
      QMessageBox::StandardButton reply = QMessageBox::question(this, "Replace existing?",
                                    "Detector " + QString::fromStdString(newdet.name_) + " already exists. Replace?",
                                     QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::No)
        return;
      else
        detectors_.replace(newdet);
    }
  }
}

void dialog_spectrum::on_spinDets_valueChanged(int arg1)
{
  if (!ui->spinDets->isEnabled())
    return;

  for (auto &a : md_.attributes.branches.my_data_)
    if (a.metadata.setting_type == Qpx::SettingType::pattern) {
      a.value_pattern.resize(arg1);
      changed_ = true;
    }

  updateData();
}
