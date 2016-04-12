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

#include "daq_sink_factory.h"
#include "form_multi_gates.h"
#include "widget_detectors.h"
#include "ui_form_multi_gates.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include <QCloseEvent>
#include "manip2d.h"
#include "qpx_util.h"

using namespace Qpx;

FormMultiGates::FormMultiGates(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormMultiGates),
  settings_(settings),
  table_model_(this),
  sortModel(this),
  gate_x(nullptr),
  spectra_(nullptr),
  current_spectrum_(0)
{
  ui->setupUi(this);

  loadSettings();

  table_model_.set_data(gates_);
  sortModel.setSourceModel( &table_model_ );
  sortModel.setSortRole(Qt::EditRole);
  ui->tableGateList->setModel( &sortModel );

  ui->tableGateList->setSortingEnabled(true);
  ui->tableGateList->sortByColumn(0, Qt::DescendingOrder);
  ui->tableGateList->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableGateList->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableGateList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableGateList->verticalHeader()->hide();
  ui->tableGateList->horizontalHeader()->setStretchLastSection(true);
  ui->tableGateList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableGateList->show();

  ui->gatedSpectrum->setFit(&fit_data_);
  connect(ui->gatedSpectrum , SIGNAL(peaks_changed()), this, SLOT(peaks_changed_in_plot()));
  connect(ui->gatedSpectrum , SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(peak_selection_changed(std::set<double>)));

  connect(ui->gatedSpectrum, SIGNAL(range_changed(Range)), this, SLOT(range_changed_in_plot(Range)));

  res_ = 0;

  connect(ui->tableGateList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

}

void FormMultiGates::range_changed_in_plot(Range range) {
  Gate gate = current_gate();
  MarkerBox2D range2d = gate.constraints;

  range2d.visible = range.visible;
  range2d.vertical = true;
  range2d.x_c = range.c;
  range2d.x1 = range.l;
  range2d.x2 = range.r;
//  range2d.labelfloat = (gate.width_chan == res);

  emit range_changed(range2d);
}

void FormMultiGates::peaks_changed_in_plot() {
  update_peaks(true);
  emit gate_selected();
  range_changed_in_plot(Range());
}

void FormMultiGates::peak_selection_changed(std::set<double> sel)
{
  update_peaks(false);
  emit gate_selected();
}


//void FormMultiGates::update_range(Range range) {
//  range2d.visible = range.visible;
//  range2d.x1 = range.l;
//  range2d.x2 = range.r;
//  update_peaks(false);
//}

FormMultiGates::~FormMultiGates()
{
  delete ui;
}

double FormMultiGates::width_factor() {
  return ui->doubleGateOn->value();
}

void FormMultiGates::closeEvent(QCloseEvent *event) {
  saveSettings();
  event->accept();
}

int32_t FormMultiGates::index_of(double centroid, bool fuzzy) {
  if (gates_.empty())
    return -1;

  if (fuzzy) {
    double threshold = ui->doubleOverlaps->value();
    for (int i=0; i < gates_.size(); ++i) {
      Coord center = gates_[i].constraints.y_c;
      if (abs(center.bin(center.bits()) - centroid) < threshold)
        return i;
    }
  } else {
    for (int i=0; i < gates_.size(); ++i) {
      Coord center = gates_[i].constraints.y_c;
      if (center.bin(center.bits()) == centroid)
        return i;
    }
  }
  return -1;
}


void FormMultiGates::update_current_gate(Gate gate) {
  Coord center = gate.constraints.y_c;
  int32_t index = index_of(center.bin(center.bits()), false);
  //PL_DBG << "update current gate " << index;


  if ((index != -1) && (gates_[index].approved))
    gate.approved = true;

  double livetime = gate.fit_data_.metadata_.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
  if (livetime <= 0)
    livetime = 100;
  gate.cps = gate.fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  if (gate.constraints.y_c.bin(0) <= -1) {

    if (index < 0) {
      gates_.clear();
      gates_.push_back(gate);
      index = 0;
    }
    gates_[index] = gate;
  } else if (index < 0) {
    //PL_DBG << "non-existing gate. ignoring";
  } else {
    gates_[index] = gate;
  }

  rebuild_table(true);
}

int32_t FormMultiGates::current_idx() {
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  int32_t  idx = -1;
  foreach (index, indexes) {
    int row = index.row();
    i = sortModel.index(row, 1, QModelIndex());
    double centroid = sortModel.data(i, Qt::EditRole).toDouble();
    int32_t gate_idx = index_of(centroid, false);
    idx = gate_idx;
  }
  return idx;
}

Gate FormMultiGates::current_gate() {
  int32_t  idx = current_idx();

  if ((idx > -1) && (idx < gates_.size()))
    return gates_[idx];

  Gate gate;
  gate.approved = false;
  gate.constraints.x_c.set_bin(-1, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.x1.set_bin(-0.5, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.x2.set_bin(res_ - 0.5, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.y_c.set_bin(-1, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.y1.set_bin(-0.5, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.y2.set_bin(res_ - 0.5, md_.bits, fit_data_.settings().cali_nrg_);
  gate.constraints.labelfloat = true;
  gate.constraints.visible = false;
  gate.constraints.horizontal = true;

  //cps?
  return gate;
}

void FormMultiGates::selection_changed(QItemSelection selected, QItemSelection deselected) {
  QModelIndexList indexes = selected.indexes();

  ui->pushRemove->setEnabled(!indexes.empty());
  ui->pushApprove->setEnabled(!indexes.empty());
  ui->pushAddGatedSpectrum->setEnabled(!indexes.empty());
  make_gate();

  emit gate_selected();
}

void FormMultiGates::rebuild_table(bool contents_changed) {
  //PL_DBG << "rebuild table";

  if (contents_changed) {
    table_model_.set_data(gates_);
  }

  int approved = 0;
  int peaks = 0;
  for (auto &q : gates_) {
    if (q.approved)
      approved++;
    if (q.constraints.y_c.bin(0) >= 0)
      peaks += q.fit_data_.peaks().size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(gates_.size() - approved));
  ui->labelPeaks->setText(QString::number(peaks));

  ui->pushDistill->setEnabled(peaks > 0);

}


void FormMultiGates::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();


  settings_.beginGroup("Multi_gates");
  ui->doubleGateOn->setValue(settings_.value("gate_on", 2).toDouble());
  ui->doubleOverlaps->setValue(settings_.value("overlaps", 2).toDouble());
  ui->gatedSpectrum->loadSettings(settings_);
  settings_.endGroup();

}


void FormMultiGates::saveSettings() {
  settings_.beginGroup("Multi_gates");
  settings_.setValue("gate_on", ui->doubleGateOn->value());
  settings_.setValue("overlaps", ui->doubleOverlaps->value());
  ui->gatedSpectrum->saveSettings(settings_);
  settings_.endGroup();
}

void FormMultiGates::clear() {
  gates_.clear();
  rebuild_table(true);

  make_gate();
//  emit gate_selected();
}
void FormMultiGates::on_pushRemove_clicked()
{
  int32_t idx = current_idx();

  if ((idx < 0) || (idx >= gates_.size()))
    return;

  gates_.erase(gates_.begin() + idx);

  rebuild_table(true);
}


void FormMultiGates::on_pushApprove_clicked()
{
  int32_t  idx = current_idx();

  if ((idx < 0) || (idx >= gates_.size()))
    return;

  gates_[idx].approved = true;

  if (!fit_data_.peaks().size())
    return;

  for (auto &q : fit_data_.peaks()) {
    Gate gate;
    double livetime = fit_data_.metadata_.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
    if (livetime <= 0)
      livetime = 100;
    gate.cps           = q.second.area_best.val / livetime;
    gate.approved = false;
    gate.constraints.labelfloat = true;

    double w = q.second.fwhm_hyp * ui->doubleGateOn->value();
    double L_nrg = q.second.energy - w / 2;
    double R_nrg = q.second.energy + w / 2;
    double L_chan = fit_data_.settings().cali_nrg_.inverse_transform(L_nrg, fit_data_.metadata_.bits);
    double R_chan = fit_data_.settings().cali_nrg_.inverse_transform(R_nrg, fit_data_.metadata_.bits);

    gate.constraints.x_c.set_bin(res_/2, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.x1.set_bin(-0.5, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.x2.set_bin(res_ - 0.5, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.y_c.set_bin(q.second.center, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.y1.set_bin(std::round(L_chan) - 0.5, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.y2.set_bin(std::round(R_chan) + 0.5, md_.bits, fit_data_.settings().cali_nrg_);
    gate.constraints.visible = true;

    if (index_of(gate.constraints.y_c.bin(0), true) < 0)
      gates_.push_back(gate);
    //else
    //PL_DBG << "duplicate. ignoring";
  }

  rebuild_table(true);
  //  emit gate_selected();
}

void FormMultiGates::on_pushDistill_clicked()
{
  all_boxes_.clear();

  for (auto &gate : gates_) {
    MarkerBox2D box = gate.constraints;
    if (box.y_c.bin(0) < 0)
      continue;

    box.visible = true;
    box.selectable = true;
    box.selected = false;
    box.vertical = true;
    box.horizontal = true;
    box.mark_center = true;

    for (auto &p : gate.fit_data_.peaks()) {
      Peak &peak = p.second;
      double w = peak.fwhm_hyp * ui->doubleGateOn->value();
      double L_nrg = peak.energy - w / 2;
      double R_nrg = peak.energy + w / 2;
      double L_chan = gate.fit_data_.settings().cali_nrg_.inverse_transform(L_nrg, gate.fit_data_.settings().bits_);
      double R_chan = gate.fit_data_.settings().cali_nrg_.inverse_transform(R_nrg, gate.fit_data_.settings().bits_);

      box.x_c.set_bin(peak.center, md_.bits, gate.fit_data_.settings().cali_nrg_);
      box.x1.set_bin(std::round(L_chan) - 0.5, md_.bits, gate.fit_data_.settings().cali_nrg_);
      box.x2.set_bin(std::round(R_chan) + 0.5, md_.bits, gate.fit_data_.settings().cali_nrg_);
      all_boxes_.push_back(box);
    }
  }

  emit boxes_made();
}

void FormMultiGates::on_doubleGateOn_editingFinished()
{
  update_peaks(false);
  emit gate_selected();
}

void FormMultiGates::make_range(Marker mrk) {
  ui->gatedSpectrum->make_range(mrk);
}


void FormMultiGates::make_gate() {
  Gate gate = current_gate();
  fit_data_ = gate.fit_data_;

  SinkPtr source_spectrum;
  if (spectra_)
    source_spectrum = spectra_->get_sink(current_spectrum_);

  if (!source_spectrum)
    return;

  this->setCursor(Qt::WaitCursor);

//  gate.centroid_nrg = fit_data_.settings().cali_nrg_.transform(gate.centroid_chan, fit_data_.metadata_.bits);
//  gate.width_nrg = fit_data_.settings().cali_nrg_.transform(gate.centroid_chan + gate.width_chan*0.5, fit_data_.metadata_.bits)
//      - fit_data_.settings().cali_nrg_.transform(gate.centroid_chan - gate.width_chan*0.5, fit_data_.metadata_.bits);

  uint32_t xmin = std::ceil(gate.constraints.x1.bin(md_.bits));
  uint32_t xmax = std::floor(gate.constraints.x2.bin(md_.bits));
  uint32_t ymin = std::ceil(gate.constraints.y1.bin(md_.bits));
  uint32_t ymax = std::floor(gate.constraints.y2.bin(md_.bits));

  PL_DBG << "Coincidence gate x[" << xmin << "-" << xmax << "]   y[" << ymin << "-" << ymax << "]";

  Metadata md = source_spectrum->metadata();

  if ((md.total_count > 0) && (md.dimensions() == 2))
  {
    std::string name =
        md.detectors[0].name_ +
        "[" + to_str_precision(md.detectors[1].best_calib(md.bits).transform(ymin, md.bits), 0)
        + "," +
        to_str_precision(md.detectors[1].best_calib(md.bits).transform(ymax, md.bits), 0) + "]"

        + " x " +

        md.detectors[1].name_ +
        "[" + to_str_precision(md.detectors[0].best_calib(md.bits).transform(xmin, md.bits), 0)
        + "," +
        to_str_precision(md.detectors[0].best_calib(md.bits).transform(xmax, md.bits), 0) + "]";

    if (gate_x && (gate_x->name() == name)) {
//      PL_DBG << "same gate";
    } else {
      gate_x = slice_rectangular(source_spectrum, {{xmin, xmax}, {ymin, ymax}}, true);
      gate_x->set_name(name);
//      PL_DBG << "made new gate";
    }

//    fit_data_.clear();
    fit_data_.setData(gate_x);
    ui->gatedSpectrum->setFit(&fit_data_);
    ui->gatedSpectrum->update_spectrum();
    gate.fit_data_ = fit_data_;
    update_current_gate(gate);
    //    ui->plotMatrix->refresh();
  }
  this->setCursor(Qt::ArrowCursor);

  update_peaks(true);
}

void FormMultiGates::setSpectrum(Project *newset, int64_t idx) {
  clear();
  spectra_ = newset;
  current_spectrum_ = idx;
  SinkPtr spectrum = spectra_->get_sink(idx);

  if (spectrum && spectrum->bits()) {
    md_ = spectrum->metadata();
    res_ = pow(2,md_.bits);
    make_gate();
    if (!gates_.empty()) {
      //PL_DBG << "not empty";
      ui->tableGateList->selectRow(0);
      ui->gatedSpectrum->update_spectrum();
    }
    emit gate_selected();
  }
}

void FormMultiGates::update_peaks(bool content_changed) {
  //update sums

  if (content_changed)
    ui->gatedSpectrum->update_spectrum();

  current_peaks_.clear();
  Gate cgate = current_gate();

  MarkerBox2D box = cgate.constraints;
  box.labelfloat = (box.y_c.bin(0) < 0);

  for (auto &q : fit_data_.peaks()) {
    Peak &peak = q.second;

    box.visible = true;
    box.selected = ui->gatedSpectrum->get_selected_peaks().count(peak.center);

    double w = peak.fwhm_hyp * ui->doubleGateOn->value();
    double L_nrg = peak.energy - w / 2;
    double R_nrg = peak.energy + w / 2;
    double L_chan = fit_data_.settings().cali_nrg_.inverse_transform(L_nrg, md_.bits);
    double R_chan = fit_data_.settings().cali_nrg_.inverse_transform(R_nrg, md_.bits);

    box.x_c.set_bin(peak.center, md_.bits, fit_data_.settings().cali_nrg_);
    box.x1.set_bin(std::round(L_chan) - 0.5, md_.bits, fit_data_.settings().cali_nrg_);
    box.x2.set_bin(std::round(R_chan) + 0.5, md_.bits, fit_data_.settings().cali_nrg_);

    box.horizontal = false;
    box.vertical = true;
    current_peaks_.push_back(box);
  }

  cgate.fit_data_    = fit_data_;
  double livetime = fit_data_.metadata_.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
  if (livetime <= 0)
    livetime = 100;
  cgate.cps = fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  update_current_gate(cgate);
}

void FormMultiGates::choose_peaks(std::list<MarkerBox2D> chpeaks) {
  std::set<double> xs;//, ys;
  for (auto &q : chpeaks) {
    xs.insert(q.x_c.bin(fit_data_.metadata_.bits));
//    ys.insert(q.y_c.bin(fit_data_.metadata_.bits));
  }
  ui->gatedSpectrum->update_selection(xs);
  update_peaks(false);
}

void FormMultiGates::on_pushAddGatedSpectrum_clicked()
{
  this->setCursor(Qt::WaitCursor);
  bool success = false;

  if (gate_x && (gate_x->metadata().total_count > 0)) {
    Setting app = gate_x->metadata().attributes.branches.get(Setting("appearance"));
    app.value_text = generateColor().name(QColor::HexArgb).toStdString();
    gate_x->set_option(app);

    spectra_->add_sink(gate_x);
    success = true;
  }

  if (success) {
//    emit spectraChanged();
  }

  this->setCursor(Qt::ArrowCursor);
}










TableGates::TableGates(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TableGates::set_data(std::vector<Gate> gates)
{
  //std::reverse(gates_.begin(),gates_.end());
  emit layoutAboutToBeChanged();
  gates_ = gates;
  /*QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );*/
  emit layoutChanged();
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

  Gate gate = gates_[row];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(gate.cps);
    case 1:
      return QVariant::fromValue(gate.constraints.y_c.bin(0));
    case 2:
      return QVariant::fromValue(gate.constraints.y_c.energy());
    case 3:
      return QVariant::fromValue(gate.constraints.y2.bin(0) - gate.constraints.y1.bin(0));
    case 4:
      return QVariant::fromValue(gate.constraints.y2.energy() - gate.constraints.y1.energy());
    case 5:
      return QVariant::fromValue(gate.fit_data_.peaks().size());
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


