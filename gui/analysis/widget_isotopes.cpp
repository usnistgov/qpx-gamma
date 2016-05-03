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
#include <QMessageBox>
#include <QCloseEvent>
#include <QInputDialog>
#include "widget_isotopes.h"
#include "ui_widget_isotopes.h"
#include "qt_util.h"

#include "custom_logger.h"

WidgetIsotopes::WidgetIsotopes(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WidgetIsotopes),
  isotopes_("isotopes"),
  table_gammas_(this),
  special_delegate_(this),
  sortModel(this)
{
  ui->setupUi(this);

  modified_ = false;

  ui->tableGammas->setModel(&table_gammas_);
  ui->tableGammas->setItemDelegate(&special_delegate_);
  ui->tableGammas->verticalHeader()->hide();
  ui->tableGammas->setSortingEnabled(true);
  ui->tableGammas->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGammas->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableGammas->horizontalHeader()->setStretchLastSection(true);
  ui->tableGammas->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGammas->show();

  sortModel.setSourceModel( &table_gammas_ );
  sortModel.setSortRole(Qt::EditRole);
  ui->tableGammas->setModel( &sortModel );

  connect(ui->tableGammas->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  connect(ui->listIsotopes, SIGNAL(currentTextChanged(QString)), this, SLOT(isotopeChosen(QString)));
  connect(&table_gammas_, SIGNAL(energiesChanged()), this, SLOT(energies_changed()));
}

WidgetIsotopes::~WidgetIsotopes()
{
  delete ui;
}

void WidgetIsotopes::set_editable(bool enable) {
  ui->groupEnergies->setVisible(enable);
  ui->groupIsotopes->setVisible(enable);
  if (enable) {
    ui->tableGammas->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableGammas->setEditTriggers(QAbstractItemView::AllEditTriggers);
  } else {
    ui->tableGammas->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableGammas->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }
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

  modified_ = false;
}

std::list<RadTypes::Gamma> WidgetIsotopes::current_isotope_gammas() const {
  if (ui->listIsotopes->currentItem() != nullptr)
    return isotopes_.get(RadTypes::Isotope(ui->listIsotopes->currentItem()->text().toStdString())).gammas.my_data_;
  else
    return std::list<RadTypes::Gamma>();
}

void WidgetIsotopes::isotopeChosen(QString choice) {

  ui->labelPeaks->setText("Energies");
  ui->pushRemoveIsotope->setEnabled(false);
  ui->pushAddGamma->setEnabled(false);
  table_gammas_.clear();

  RadTypes::Isotope iso(choice.toStdString());
  if (isotopes_.has_a(iso)) {
    ui->labelPeaks->setText(choice);
    ui->pushRemoveIsotope->setEnabled(true);
    iso = isotopes_.get(iso);
    table_gammas_.set_gammas(iso.gammas);
    ui->pushAddGamma->setEnabled(true);
  }
  selection_changed(QItemSelection(), QItemSelection());

  emit isotopeSelected();
}

void WidgetIsotopes::selection_changed(QItemSelection, QItemSelection) {
  current_gammas_.clear();

  foreach (QModelIndex idx, ui->tableGammas->selectionModel()->selectedRows()) {
    QModelIndex nrg_ix = table_gammas_.index(idx.row(), 0);
    current_gammas_.push_back(table_gammas_.data(nrg_ix, Qt::DisplayRole).toDouble());
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
  INFO << "sum=" << sum_energy;

  RadTypes::Gamma newgamma;
  newgamma.energy = sum_energy;
  newgamma.abundance = 0.0;
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  modified.gammas.add(newgamma);
  INFO << "modifying " << modified.name << " to have " << modified.gammas.size() << " gammas";
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));

  modified_ = true;
}

void WidgetIsotopes::on_pushRemove_clicked()
{
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(ui->listIsotopes->currentItem()->text().toStdString()));
  for (auto q : current_gammas_)
    modified.gammas.remove_a(RadTypes::Gamma(q, 0.0));
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));

  modified_ = true;
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

bool WidgetIsotopes::save_close() {

/*  if (modified_)
  {
    int reply = QMessageBox::warning(this, "Isotopes modified",
                                     "Save?",
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      QString fileName = root_dir_ + "/isotopes.xml";
      isotopes_.write_xml(fileName.toStdString());
      modified_ = false;
    } else if (reply == QMessageBox::Cancel) {
      return false;
    }
  }*/

  if (modified_) {
    QString fileName = root_dir_ + "/isotopes.xml";
    isotopes_.write_xml(fileName.toStdString());
    modified_ = false;
  }

  return true;
}

void WidgetIsotopes::on_pushAddGamma_clicked()
{
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(current_isotope().toStdString()));
  modified.gammas.add_a(RadTypes::Gamma(1, 0.0));
  isotopes_.replace(modified);
  isotopeChosen(QString::fromStdString(modified.name));
  modified_ = true;
}

void WidgetIsotopes::on_pushRemoveIsotope_clicked()
{
  table_gammas_.clear();
  isotopes_.remove_a(RadTypes::Isotope(current_isotope().toStdString()));
  ui->listIsotopes->clear();
  for (int i=0; i < isotopes_.size(); i++) {
    ui->listIsotopes->addItem(QString::fromStdString(isotopes_.get(i).name));
  }
  modified_ = true;
  isotopeChosen("");
}

void WidgetIsotopes::on_pushAddIsotope_clicked()
{
  bool ok;
  QString text = QInputDialog::getText(this, "New Isotope",
                                       "Isotope name:", QLineEdit::Normal,
                                       "", &ok);
  if (ok && !text.isEmpty()) {
    if (isotopes_.has_a(RadTypes::Isotope(text.toStdString())))
      QMessageBox::warning(this, "Already exists", "Isotope " + text + " already exists", QMessageBox::Ok);
    else {
      isotopes_.add(RadTypes::Isotope(text.toStdString()));
      ui->listIsotopes->clear();
      for (int i=0; i < isotopes_.size(); i++) {
        ui->listIsotopes->addItem(QString::fromStdString(isotopes_.get(i).name));
      }
      modified_ = true;
      ui->listIsotopes->setCurrentRow(ui->listIsotopes->count() - 1);
    }
  }
}

void WidgetIsotopes::energies_changed() {
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(current_isotope().toStdString()));
  modified.gammas = table_gammas_.get_gammas();
  isotopes_.replace(modified);
  modified_ = true;
}

void WidgetIsotopes::push_energies(std::vector<double> new_energies) {
  RadTypes::Isotope modified = isotopes_.get(RadTypes::Isotope(current_isotope().toStdString()));
  for (auto &q : new_energies)
    modified.gammas.add(RadTypes::Gamma(q, 0));
  isotopes_.replace(modified);
  modified_ = true;
  isotopeChosen(current_isotope());
}

void WidgetIsotopes::select_energies(std::set<double> sel_energies) {
  XMLableDB<RadTypes::Gamma> gammas = table_gammas_.get_gammas();
  for (auto &q : gammas.my_data_)
    q.marked = (sel_energies.count(q.energy) > 0);
  table_gammas_.set_gammas(gammas);
}


void WidgetIsotopes::select_next_energy()
{
  current_gammas_.clear();

  int top_row = -1;
  foreach (QModelIndex idx, ui->tableGammas->selectionModel()->selectedRows()) {
    if (idx.row() > top_row)
      top_row = idx.row();
  }

  top_row++;
  if (top_row < table_gammas_.rowCount()) {
    ui->tableGammas->clearSelection();
    ui->tableGammas->selectRow(top_row);
  }

  emit energiesSelected();
}





TableGammas::TableGammas(QObject *parent) :
  QAbstractTableModel(parent)
{
}

int TableGammas::rowCount(const QModelIndex & /*parent*/) const
{
  return gammas_.size();
}

int TableGammas::columnCount(const QModelIndex & /*parent*/) const
{
  return 2;
}

QVariant TableGammas::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();
  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(gammas_[row].energy);
    case 1:
      return QVariant::fromValue(gammas_[row].abundance);
    }
  } else if (role == Qt::ForegroundRole) {
    if (gammas_[row].marked)
      return QBrush(Qt::darkGreen);
    else
      return QBrush(Qt::black);
  }
  return QVariant();
}

QVariant TableGammas::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("Energy");
      case 1:
        return QString("Intensity");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}


void TableGammas::set_gammas(const XMLableDB<RadTypes::Gamma> &newgammas) {
  gammas_.clear();
  for (int i=0; i <newgammas.size(); ++i)
    gammas_.push_back(newgammas.get(i));

  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( (rowCount() - 1), (columnCount() - 1) );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

XMLableDB<RadTypes::Gamma> TableGammas::get_gammas() {
  XMLableDB<RadTypes::Gamma> gammas("gammas");
  for (auto &q : gammas_)
    gammas.add(q);
  return gammas;
}

void TableGammas::clear() {
  set_gammas(XMLableDB<RadTypes::Gamma>("gammas"));
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( (rowCount() - 1), (columnCount() - 1) );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableGammas::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

bool TableGammas::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();
  QModelIndex ix = createIndex(row, col);

  if (role == Qt::EditRole) {
    if (col == 0)
      gammas_[row].energy = value.toDouble();
    else if (col == 1)
      gammas_[row].abundance = value.toDouble();
  }

  QModelIndex start_ix = createIndex( row, col );
  QModelIndex end_ix = createIndex( row, col );
  emit dataChanged( start_ix, end_ix );
  //emit layoutChanged();
  emit energiesChanged();
  return true;
}

