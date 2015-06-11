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
 *      Isotope DB browsing
 *
 ******************************************************************************/

#include <QFileDialog>
#include "gui/widget_isotopes.h"
#include "ui_widget_isotopes.h"
#include "qt_util.h"

#include "custom_logger.h"

WidgetIsotopes::WidgetIsotopes(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WidgetIsotopes),
  isotopes_("isotopes")
{
  ui->setupUi(this);

  ui->tableEnergies->setColumnCount(2);
  ui->tableEnergies->setHorizontalHeaderItem(0, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableEnergies->setHorizontalHeaderItem(1, new QTableWidgetItem("Abundance", QTableWidgetItem::Type));
  ui->tableEnergies->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableEnergies->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableEnergies->horizontalHeader()->setStretchLastSection(true);
  ui->tableEnergies->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  connect(ui->listIsotopes, SIGNAL(currentTextChanged(QString)), this, SLOT(isotopeChosen(QString)));
  connect(ui->tableEnergies, SIGNAL(itemSelectionChanged()), this, SLOT(energiesChosen()));
}

WidgetIsotopes::~WidgetIsotopes()
{
  delete ui;
}

void WidgetIsotopes::setDir(QString filedir) {
  root_dir_ = filedir;
  filedir += "/isotopes.xml";

  if (validateFile(this, filedir, false)) {
    isotopes_.read_xml(filedir.toStdString());
    ui->listIsotopes->clear();
    for (int i=0; i < isotopes_.size(); i++) {
      ui->listIsotopes->addItem(QString::fromStdString(isotopes_.get(i).name));
    }
  }
}

void WidgetIsotopes::on_pushOpen_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load isotopes", root_dir_, "Isotope database (*.xml)");
  if (!validateFile(this, fileName, false))
    return;

  isotopes_.clear();
  isotopes_.read_xml(fileName.toStdString());
  ui->listIsotopes->clear();
  for (int i=0; i < isotopes_.size(); i++) {
    ui->listIsotopes->addItem(QString::fromStdString(isotopes_.get(i).name));
  }
}

void WidgetIsotopes::isotopeChosen(QString choice) {
  ui->tableEnergies->clear();
  ui->tableEnergies->setHorizontalHeaderItem(0, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableEnergies->setHorizontalHeaderItem(1, new QTableWidgetItem("Abundance", QTableWidgetItem::Type));
  RadTypes::Isotope iso(choice.toStdString());
  if (isotopes_.has_a(iso)) {
    iso = isotopes_.get(iso);
    ui->tableEnergies->setRowCount(iso.gammas.size());
    int i = 0;
    for (auto &q : iso.gammas.my_data_) {
      QTableWidgetItem *en = new QTableWidgetItem(QString::number(q.energy));
      en->setFlags(en->flags() ^ Qt::ItemIsEditable);
      ui->tableEnergies->setItem(i,0, en);
      QTableWidgetItem *abu = new QTableWidgetItem(QString::number(q.abundance));
      abu->setFlags(abu->flags() ^ Qt::ItemIsEditable);
      ui->tableEnergies->setItem(i,1, abu);
      ++i;
    }
  }
}

void WidgetIsotopes::energiesChosen() {
  current_gammas_.clear();
  for (auto &q : ui->tableEnergies->selectedRanges()) {
    if (q.rowCount() > 0)
      for (int j = q.topRow(); j <= q.bottomRow(); j++)
        current_gammas_.push_back(ui->tableEnergies->item(j,0)->data(Qt::EditRole).toDouble());
  }

  ui->pushSum->setEnabled(false);

  if (current_gammas_.size() > 0)
    ui->pushRemove->setEnabled(true);
  if (current_gammas_.size() > 1)
    ui->pushSum->setEnabled(true);

  emit energiesSelected();

}

std::vector<double> WidgetIsotopes::current_gammas() const {
  return current_gammas_;
}

void WidgetIsotopes::on_pushSum_clicked()
{
  double sum_energy = 0.0;
  for (auto &q : current_gammas_) {
    sum_energy += q;
  }
  PL_INFO << "sum=" << sum_energy;

  RadTypes::Gamma newgamma;
  newgamma.energy = sum_energy;
  newgamma.abundance = 0.0;
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  modified.gammas.add(newgamma);
  PL_INFO << "modifying " << modified.name << " to have " << modified.gammas.size() << " gammas";
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));
}

void WidgetIsotopes::on_pushRemove_clicked()
{
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  for (auto q : current_gammas_)
    modified.gammas.remove_a(RadTypes::Gamma(q, 0.0));
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));
}

QString WidgetIsotopes::current_isotope() const {
  if (ui->listIsotopes->currentItem() != nullptr)
    return ui->listIsotopes->currentItem()->text();
  else
    return "";
}

void WidgetIsotopes::set_current_isotope(QString choice) {
  for (int i = 0; i < ui->listIsotopes->count(); ++i)
    if (ui->listIsotopes->item(i)->text() == choice)
      ui->listIsotopes->setCurrentRow(i);
}
