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
 *      FormMultiGates -
 *
 ******************************************************************************/

#include "form_multi_gates.h"
#include "widget_detectors.h"
#include "ui_form_multi_gates.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormMultiGates::FormMultiGates(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormMultiGates),
  //current_gate_(-1),
  settings_(settings),
  table_model_(this),
  sortModel(this)
{
  ui->setupUi(this);

  loadSettings();

  table_model_.set_data(gates_);

  ui->tableGateList->setModel(&table_model_);
  ui->tableGateList->setSortingEnabled(true);
  ui->tableGateList->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGateList->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableGateList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableGateList->horizontalHeader()->setStretchLastSection(true);
  ui->tableGateList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGateList->show();

  sortModel.setSourceModel( &table_model_ );
  sortModel.setSortRole(Qt::EditRole);
  ui->tableGateList->setModel( &sortModel );

  connect(ui->tableGateList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

}

FormMultiGates::~FormMultiGates()
{
  delete ui;
}

int32_t FormMultiGates::index_of(double centroid, bool fuzzy) {
  if (gates_.empty())
    return -1;

  if (fuzzy) {
    double threshold = ui->doubleOverlaps->value();
    for (int i=0; i < gates_.size(); ++i)
      if (abs(gates_[i].centroid_chan - centroid) < threshold)
        return i;
  } else {
    for (int i=0; i < gates_.size(); ++i)
      if (gates_[i].centroid_chan == centroid)
        return i;
  }
    return -1;
}


void FormMultiGates::update_current_gate(Gamma::Gate gate) {
  PL_DBG << "update current gate";
  int32_t index = index_of(gate.centroid_chan, false);


  if ((index != -1) && (gates_[index].approved))
    gate.approved = true;

  bool livetime = gate.fit_data_.metadata_.live_time.total_seconds();
  if (livetime == 0)
    livetime = 10000;
  gate.cps = gate.fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  if (gate.centroid_chan == -1) {
    if (index < 0) {
      gates_.clear();
      gates_.push_back(gate);
      index = 0;
    }
    gates_[index] = gate;
    //current_gate_ = index;
  } else if (index < 0) {
    //PL_DBG << "non-existing gate. ignoring";
  } else {
    gates_[index] = gate;
    //current_gate_ = index;
    //PL_DBG << "new current gate = " << current_gate_;
  }

  rebuild_table(true);
}

uint32_t FormMultiGates::current_idx() {
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  uint32_t  current_gate_ = -1;
    foreach (index, indexes) {
        int row = sortModel.mapToSource(index).row();
        PL_DBG << "row " << row;
        i = table_model_.index(row, 1, QModelIndex());
        double centroid = table_model_.data(i, Qt::EditRole).toDouble();
        int32_t gate_idx = index_of(centroid, false);
        PL_DBG << "selected " << gate_idx << " -> "  << centroid;
        current_gate_ = gate_idx;
    }
    return current_gate_;
}

Gamma::Gate FormMultiGates::current_gate() {
  uint32_t  current_gate_ = current_idx();

  if (current_gate_ < 0)
    return Gamma::Gate();
  if (current_gate_ >= gates_.size()) {
    current_gate_ = -1;
    return Gamma::Gate();
  } else
    return gates_[current_gate_];
}

void FormMultiGates::selection_changed(QItemSelection selected, QItemSelection deselected) {
  //current_gate_ = -1;

  QModelIndexList indexes = selected.indexes();
  QModelIndex index, i;

  PL_DBG << "gates " << gates_.size() << " selections " << indexes.size();

  // Go through all the selected items
//  foreach (QModelIndex idx, ui->tableGateList->selectionModel()->selectedRows()) {
//    double centroid = sortModel.data(idx, Qt::EditRole).toDouble();
//    int32_t gate_idx = index_of(centroid, false);
//    PL_DBG << "selected " << gate_idx << " -> "  << centroid;
//    current_gate_ = gate_idx;
//  }

//  foreach (index, indexes) {
//      int row = sortModel.mapToSource(index).row();
//      PL_DBG << "row " << row;
//      i = table_model_.index(row, 1, QModelIndex());
//      double centroid = table_model_.data(i, Qt::EditRole).toDouble();
//      int32_t gate_idx = index_of(centroid, false);
//      PL_DBG << "selected " << gate_idx << " -> "  << centroid;
//      current_gate_ = gate_idx;
//  }

  emit gate_selected();
}

void FormMultiGates::rebuild_table(bool contents_changed) {
  PL_DBG << "rebuild table";

  if (contents_changed)
    table_model_.set_data(gates_);

  int approved = 0;
  int peaks = 0;
  for (auto &q : gates_) {
    if (q.approved)
      approved++;
    peaks += q.fit_data_.peaks_.size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(gates_.size() - approved));
  ui->labelPeaks->setText(QString::number(peaks));

  ui->pushDistill->setEnabled(gates_.size() == approved);


  //ui->tableGateList->clearSelection();
//  for (int i=0; i < sortModel.rowCount(); ++i) {
//    QModelIndex ix = sortModel.index(i, 1);
//    double centroid = sortModel.data(ix, Qt::EditRole).toDouble();
//    PL_DBG << "considering for selection c " << centroid;
//    int32_t index = index_of(centroid, false);
//    if (index == current_gate_) {
//      ui->tableGateList->selectionModel()->select(ix, QItemSelectionModel::SelectCurrent);
//      break;
//    }
//  }

  table_model_.update();
}


void FormMultiGates::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();


/*  settings_.beginGroup("Gain_calibration");
  ui->spinPolyOrder->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->doubleCullDelta->setValue(settings_.value("r_squared_goal", 3.00).toDouble());
  settings_.endGroup();*/

}


void FormMultiGates::saveSettings() {

  /*settings_.beginGroup("Gain_calibration");
  settings_.setValue("fit_function_terms", ui->spinPolyOrder->value());
  settings_.setValue("r_squared_goal", ui->doubleCullDelta->value());
  settings_.endGroup();*/
}

void FormMultiGates::clear() {
  gates_.clear();
  //current_gate_ = -1;
  rebuild_table(true);
  emit gate_selected();
}
void FormMultiGates::on_pushRemove_clicked()
{
  uint32_t  current_gate_ = current_idx();

  PL_DBG << "trying to remove " << current_gate_ << " in total of " << gates_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= gates_.size())
    return;
}


void FormMultiGates::on_pushApprove_clicked()
{
  uint32_t  current_gate_ = current_idx();

  PL_DBG << "trying to approve " << current_gate_ << " in total of " << gates_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= gates_.size())
    return;


  gates_[current_gate_].approved = true;
  Gamma::Fitter data = gates_[current_gate_].fit_data_;

  if (!data.peaks_.size())
    return;

  for (auto &q : data.peaks_) {
    Gamma::Gate newgate;
    newgate.cps           = q.second.cts_per_sec_gauss_;
    newgate.centroid_chan = q.second.center;
    newgate.centroid_nrg  = q.second.energy;
    newgate.width_chan    = q.second.gaussian_.hwhm_ * 2.0 * ui->doubleGateOn->value();
    newgate.width_nrg     = q.second.fwhm_gaussian;
    if (index_of(newgate.centroid_chan, true) < 0)
      gates_.push_back(newgate);
    //else
      //PL_DBG << "duplicate. ignoring";
  }

  rebuild_table(true);
//  emit gate_selected();
}











TableGates::TableGates(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TableGates::set_data(std::vector<Gamma::Gate> gates)
{
  //std::reverse(gates_.begin(),gates_.end());
  emit layoutAboutToBeChanged();
  gates_ = gates;
}

int TableGates::rowCount(const QModelIndex & /*parent*/) const
{
  //PL_DBG << "table has " << gates_.size();
  return gates_.size();
}

int TableGates::columnCount(const QModelIndex & /*parent*/) const
{
    return 7;
}

QVariant TableGates::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();

  Gamma::Gate gate = gates_[gates_.size() - 1 - row];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(gate.cps);
    case 1:
      return QVariant::fromValue(gate.centroid_chan);
    case 2:
      return QVariant::fromValue(gate.centroid_nrg);
    case 3:
      return QVariant::fromValue(gate.width_chan);
    case 4:
      return QVariant::fromValue(gate.width_nrg);
    case 5:
      return QVariant::fromValue(gate.fit_data_.peaks_.size());
    case 6:
      if (gate.approved)
        return "Yes";
      else
        return "...";
    }
  }
  return QVariant();
}


QVariant TableGates::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("Intensity (cps)");
      case 1:
        return QString("Centroid (chan)");
      case 2:
        return QString("Centroid (keV)");
      case 3:
        return QString("Width (chan)");
      case 4:
        return QString("Width (keV)");
      case 5:
        return QString("Coincidences");
      case 6:
        return QString("Approved?");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}


Qt::ItemFlags TableGates::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index);
}


void TableGates::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

/*bool TableGates::setData(const QModelIndex & index, const QVariant & value, int role)
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
}*/
