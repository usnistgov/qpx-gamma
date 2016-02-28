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
#include <QCloseEvent>
#include "manip2d.h"
#include "qpx_util.h"

FormMultiGates::FormMultiGates(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormMultiGates),
  //current_gate_(-1),
  settings_(settings),
  table_model_(this),
  sortModel(this),
  gate_x(nullptr)
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
  connect(ui->gatedSpectrum , SIGNAL(peaks_changed(bool)), this, SLOT(peaks_changed_in_plot(bool)));
  connect(ui->gatedSpectrum, SIGNAL(range_changed(Range)), this, SLOT(range_changed_in_plot(Range)));

  tempx = Qpx::Spectrum::Factory::getInstance().create_template("1D");
  tempx->visible = true;
  tempx->match_pattern = std::vector<int16_t>({1,1});
  tempx->add_pattern = std::vector<int16_t>({1,0});

  res = 0;
  xmin_ = 0;
  xmax_ = 0;
  ymin_ = 0;
  ymax_ = 0;
  xc_ = -1;
  yc_ = -1;

  connect(ui->tableGateList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

}

void FormMultiGates::range_changed_in_plot(Range range) {
  range2d.visible = range.visible;
  range2d.vertical = true;
  range2d.x1 = range.l;
  range2d.x2 = range.r;
  emit range_changed(range2d);
}

void FormMultiGates::peaks_changed_in_plot(bool content_changed) {
  update_peaks(content_changed);
  emit gate_selected();
}


void FormMultiGates::update_range(Range range) {
  range2d.visible = range.visible;
  range2d.x1 = range.l;
  range2d.x2 = range.r;
  update_peaks(false);
}

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


void FormMultiGates::update_current_gate(Qpx::Gate gate) {
  int32_t index = index_of(gate.centroid_chan, false);
  //PL_DBG << "update current gate " << index;


  if ((index != -1) && (gates_[index].approved))
    gate.approved = true;

  double livetime = gate.fit_data_.metadata_.live_time.total_seconds();
  //PL_DBG << "update current gate " << gate.centroid_chan << " with LT = " << livetime;
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

int32_t FormMultiGates::current_idx() {
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  int32_t  current_gate_ = -1;
  //  PL_DBG << "will look at " << indexes.size() << " selected indexes";
  foreach (index, indexes) {
    int row = index.row();
    i = sortModel.index(row, 1, QModelIndex());
    double centroid = sortModel.data(i, Qt::EditRole).toDouble();
    int32_t gate_idx = index_of(centroid, false);
    //    PL_DBG << "selected " << gate_idx << " -> "  << centroid;
    current_gate_ = gate_idx;
  }
  //  PL_DBG << "now current gate is " << current_gate_;
  return current_gate_;
}

Qpx::Gate FormMultiGates::current_gate() {
  uint32_t  current_gate_ = current_idx();

  if (current_gate_ < 0)
    return Qpx::Gate();
  if (current_gate_ >= gates_.size()) {
    current_gate_ = -1;
    return Qpx::Gate();
  } else
    return gates_[current_gate_];
}

void FormMultiGates::selection_changed(QItemSelection selected, QItemSelection deselected) {
  //current_gate_ = -1;

  QModelIndexList indexes = selected.indexes();
  QModelIndex i;

  //PL_DBG << "gates " << gates_.size() << " selections " << indexes.size();

  ui->pushRemove->setEnabled(!indexes.empty());
  ui->pushApprove->setEnabled(!indexes.empty());
  ui->pushAddGatedSpectrum->setEnabled(!indexes.empty());

  remake_gate(false);

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
    if (q.centroid_chan != -1)
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
  //current_gate_ = -1;
  rebuild_table(true);

  remake_gate(true);
//  emit gate_selected();
}
void FormMultiGates::on_pushRemove_clicked()
{
  uint32_t  current_gate_ = current_idx();

  //PL_DBG << "trying to remove " << current_gate_ << " in total of " << gates_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= gates_.size())
    return;

  gates_.erase(gates_.begin() + current_gate_);

  rebuild_table(true);
}


void FormMultiGates::on_pushApprove_clicked()
{
  uint32_t  current_gate_ = current_idx();

  //PL_DBG << "trying to approve " << current_gate_ << " in total of " << gates_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= gates_.size())
    return;


  gates_[current_gate_].approved = true;
  Qpx::Fitter data = gates_[current_gate_].fit_data_;

  if (!data.peaks().size())
    return;

  for (auto &q : data.peaks()) {
    Qpx::Gate newgate;
    newgate.cps           = q.second.area_best.val / (data.metadata_.live_time.total_milliseconds() / 1000);
    newgate.centroid_chan = q.second.center;
    newgate.centroid_nrg  = q.second.energy;
    newgate.width_chan    = std::round(q.second.fwhm_hyp * ui->doubleGateOn->value());
    newgate.width_nrg     = data.settings().cali_nrg_.transform(newgate.centroid_chan + newgate.width_chan / 2)
        - data.settings().cali_nrg_.transform(newgate.centroid_chan - newgate.width_chan / 2);
    if (index_of(newgate.centroid_chan, true) < 0)
      gates_.push_back(newgate);
    //else
    //PL_DBG << "duplicate. ignoring";
  }

  rebuild_table(true);
  //  emit gate_selected();
}

void FormMultiGates::on_pushDistill_clicked()
{
  all_boxes_.clear();
  MarkerBox2D box;
  box.visible = true;
  box.selectable = true;
  box.selected = false;
  box.vertical = true;
  box.horizontal = true;
  box.mark_center = true;

  for (auto &q : gates_) {
    Qpx::Gate gate = q;
    if (gate.centroid_chan != -1) {
      box.y_c.set_bin(gate.centroid_chan, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
      box.y1.set_bin(gate.centroid_chan - (gate.width_chan / 2.0), gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
      box.y2.set_bin(gate.centroid_chan + (gate.width_chan / 2.0), gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);

      for (auto &p : gate.fit_data_.peaks()) {
        Qpx::Peak peak = p.second;
        box.x_c.set_bin(peak.center, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
        box.x1.set_bin(peak.center - (peak.fwhm_hyp * 0.5 * ui->doubleGateOn->value()), gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
        box.x2.set_bin(peak.center + (peak.fwhm_hyp * 0.5 * ui->doubleGateOn->value()), gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
        all_boxes_.push_back(box);
      }
    }
  }

  emit boxes_made();
}

void FormMultiGates::on_doubleGateOn_editingFinished()
{
  remake_gate(true);
  emit gate_selected();
}

void FormMultiGates::on_doubleOverlaps_editingFinished()
{
  remake_gate(true);
  emit gate_selected();
}

void FormMultiGates::change_width(int width) {
  if (current_gate().width_chan != width) {
    uint32_t  current_gate_ = current_idx();

    if (current_gate_ < 0)
      return;
    if (current_gate_ >= gates_.size())
      return;


    gates_[current_gate_].width_chan = width;
    remake_gate(true);
    emit gate_selected();
  }
}

void FormMultiGates::make_range(Marker mrk) {
  ui->gatedSpectrum->make_range(mrk);
}


void FormMultiGates::remake_gate(bool force) {

  double xmin = 0;
  double xmax = res - 1;
  double ymin = 0;
  double ymax = res - 1;

  Qpx::Gate gate = current_gate();
  //PL_DBG << "gates remake gate c=" << gate.centroid_chan << " w=" << gate.width_chan;

  if (gates_.empty()) {
    gate.centroid_chan = -1;
    gate.width_chan = res;
  }

  if (gate.width_chan == 0)
    return;

  fit_data_ = gate.fit_data_;
  yc_ = xc_ = gate.centroid_chan;


  Marker xx, yy;
  double width = gate.width_chan;

  if (gate.centroid_chan == -1) {
    width = res;
    gate.width_chan = res;
    xx.pos.set_bin(res / 2, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
    yy.pos.set_bin(res / 2, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);

    xmin = 0;
    xmax = res - 1;
    ymin = 0;
    ymax = res - 1;

  } else {
    xmin = 0;
    xmax = xmax = res - 1;
    ymin = yc_ - (width / 2); if (ymin < 0) ymin = 0;
    ymax = yc_ + (width / 2); if (ymax >= res) ymax = res - 1;

    xx.pos.set_bin(res / 2, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
    yy.pos.set_bin(yc_, gate.fit_data_.metadata_.bits, gate.fit_data_.settings().cali_nrg_);
  }

  xx.visible = false;
  yy.visible = true;

  //ui->plotMatrix->set_boxes(boxes);

  if ((xmin != xmin_) || (xmax != xmax_) || (ymin != ymin_) || (ymax != ymax_)
      /*|| (static_cast<double>(ui->plotMatrix->gate_width()) != width)*/ || force) {
    xmin_ = xmin; xmax_ = xmax;
    ymin_ = ymin; ymax_ = ymax;
    //    ui->plotMatrix->set_gate_width(static_cast<uint16_t>(gate.width_chan));
    //    ui->plotMatrix->set_markers(xx, yy);
    if (fit_data_.peaks().empty() || force)
      make_gated_spectra();
  }
  update_peaks(true);
}

void FormMultiGates::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  PL_DBG << "gates set spectrum";
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
  Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

  if (spectrum && spectrum->resolution()) {
    Qpx::Spectrum::Metadata md = spectrum->metadata();
    res = md.resolution;
    remake_gate(true);
    if (!gates_.empty()) {
      //PL_DBG << "not empty";
      ui->tableGateList->selectRow(0);
      ui->gatedSpectrum->update_spectrum();
    }
    emit gate_selected();
  }
}

void FormMultiGates::make_gated_spectra() {
  Qpx::Spectrum::Spectrum* source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
  if (source_spectrum == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  Qpx::Spectrum::Metadata md = source_spectrum->metadata();

  //  PL_DBG << "Coincidence gate x[" << xmin_ << "-" << xmax_ << "]   y[" << ymin_ << "-" << ymax_ << "]";

  if ((md.total_count > 0) && (md.dimensions == 2))
  {
    uint32_t adjrange = static_cast<uint32_t>(md.resolution) - 1;

    tempx->bits = md.bits;
    //    tempx->name_ = detector1_.name_ + "[" + to_str_precision(nrg_calibration2_.transform(ymin_), 0) + "," + to_str_precision(nrg_calibration2_.transform(ymax_), 0) + "]";
    tempx->name_ = md.detectors[0].name_ + "[" + to_str_precision(md.detectors[1].best_calib(md.bits).transform(ymin_, md.bits), 0) + ","
        + to_str_precision(md.detectors[1].best_calib(md.bits).transform(ymax_, md.bits), 0) + "]";

    if (gate_x != nullptr)
      delete gate_x;
    gate_x = Qpx::Spectrum::Factory::getInstance().create_from_template(*tempx);

    //if?
    Qpx::Spectrum::slice_rectangular(source_spectrum, gate_x, {{0, adjrange}, {ymin_, ymax_}}, spectra_->runInfo());

    fit_data_.clear();
    fit_data_.setData(gate_x);
    ui->gatedSpectrum->setFit(&fit_data_);
    ui->gatedSpectrum->update_spectrum();
    update_peaks(true);

    //    ui->plotMatrix->refresh();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormMultiGates::update_peaks(bool content_changed) {
  //update sums

  if (content_changed)
    ui->gatedSpectrum->update_data();

  current_peaks_.clear();
  double width = current_gate().width_chan / 2;
  if (yc_ < 0) {
    range2d.y_c.set_bin(res / 2, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.y1.set_bin(res / 2 - width, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.y2.set_bin(res / 2 + width, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.labelfloat = true;
  } else {
    range2d.y_c.set_bin(yc_, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.y1.set_bin(yc_ - width, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.y2.set_bin(yc_ + width, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    range2d.labelfloat = false;
  }

  MarkerBox2D box;
  box.y_c = range2d.y_c;
  box.y1 = range2d.y1;
  box.y2 = range2d.y2;


  for (auto &q : fit_data_.peaks()) {
    box.visible = true;
    box.selected = ui->gatedSpectrum->get_selected_peaks().count(q.first);
    box.x_c.set_bin(q.second.center, fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    box.x1.set_bin(q.second.center - (q.second.fwhm_hyp * 0.5 * width_factor()), fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);
    box.x2.set_bin(q.second.center + (q.second.fwhm_hyp * 0.5 * width_factor()), fit_data_.metadata_.bits, fit_data_.settings().cali_nrg_);

    box.horizontal = false;
    box.vertical = true;
    box.labelfloat = range2d.labelfloat;
    current_peaks_.push_back(box);
    range2d.y1 = box.y1;
    range2d.y2 = box.y2;
  }


  //    ui->plotMatrix->set_boxes(boxes);
  //    ui->plotMatrix->set_range_x(range2d);
  //    ui->plotMatrix->replot_markers();

  Qpx::Gate gate;
  gate.fit_data_ = fit_data_;
  gate.centroid_chan = yc_;
  gate.centroid_nrg = fit_data_.settings().cali_nrg_.transform(yc_);
  if (yc_ >= 0) {
    gate.width_chan = current_gate().width_chan;
    gate.width_nrg = fit_data_.settings().cali_nrg_.transform(ymax_) - fit_data_.settings().cali_nrg_.transform(ymin_);
  } else {
    gate.width_chan = res;
    gate.width_nrg = fit_data_.settings().cali_nrg_.transform(res) - fit_data_.settings().cali_nrg_.transform(0);
  }
  update_current_gate(gate);
}

void FormMultiGates::choose_peaks(std::list<MarkerBox2D> chpeaks) {
  std::set<double> xs, ys;
  for (auto &q : chpeaks) {
    xs.insert(q.x_c.bin(fit_data_.metadata_.bits));
    ys.insert(q.y_c.bin(fit_data_.metadata_.bits));
  }
  ui->gatedSpectrum->update_selection(xs);
  update_peaks(false);
}

void FormMultiGates::on_pushAddGatedSpectrum_clicked()
{
  this->setCursor(Qt::WaitCursor);
  bool success = false;

  if ((gate_x != nullptr) && (gate_x->metadata().total_count > 0)) {
    if (spectra_->by_name(gate_x->name()) != nullptr)
      PL_WARN << "Spectrum " << gate_x->name() << " already exists.";
    else
    {
      gate_x->set_appearance(generateColor().rgba());
      spectra_->add_spectrum(gate_x);
      gate_x = nullptr;
      success = true;
    }
  }

  if (success) {
//    emit spectraChanged();
  }
  make_gated_spectra();
  this->setCursor(Qt::ArrowCursor);
}










TableGates::TableGates(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TableGates::set_data(std::vector<Qpx::Gate> gates)
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

  Qpx::Gate gate = gates_[row];

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


