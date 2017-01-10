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
#include "fitter.h"
#include "qt_util.h"
#include <QCloseEvent>
#include "manip2d.h"
#include "qpx_util.h"
#include <QSettings>

using namespace Qpx;

FormMultiGates::FormMultiGates(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormMultiGates),
  table_model_(this),
  sortModel(this)
{
  ui->setupUi(this);

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

  connect(ui->gatedSpectrum, SIGNAL(range_selection_changed(double,double)),
          this, SLOT(update_range_selection(double,double)));
  connect(ui->gatedSpectrum, SIGNAL(fitting_done()), this, SLOT(fitting_finished()));

  connect(ui->tableGateList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  loadSettings();
}

FormMultiGates::~FormMultiGates()
{
  delete ui;
}

void FormMultiGates::closeEvent(QCloseEvent *event)
{
  saveSettings();
  event->accept();
}

void FormMultiGates::loadSettings()
{
  QSettings settings;

  settings.beginGroup("Multi_gates");
  ui->doubleGateOn->setValue(settings.value("gate_on", 3).toDouble());
  ui->doubleOverlaps->setValue(settings.value("overlaps", 2).toDouble());
  ui->gatedSpectrum->loadSettings(settings);
  settings.endGroup();
}

void FormMultiGates::saveSettings()
{
  QSettings settings;

  settings.beginGroup("Multi_gates");
  settings.setValue("gate_on", ui->doubleGateOn->value());
  settings.setValue("overlaps", ui->doubleOverlaps->value());
  ui->gatedSpectrum->saveSettings(settings);
  settings.endGroup();
}

void FormMultiGates::clear()
{
  md_ = Qpx::Metadata();
  res_ = 0;
  nrg_x_ = Qpx::Calibration();
  nrg_y_ = Qpx::Calibration();

  gates_.clear();
  rebuild_table(true);

  make_gate();
  //  emit gate_selected();
}

void FormMultiGates::clearSelection()
{
  range2d.selected = false;
  ui->gatedSpectrum->clearSelection();
  emit gate_selected();
}

void FormMultiGates::update_range_selection(double l, double r)
{
  range2d = current_gate().constraints;
  range2d.x.set_bounds(fit_data_.settings().energy_to_bin(l),
                       fit_data_.settings().energy_to_bin(r));
  range2d.selected = (l != r);
  range2d.color = Qt::magenta;
  range2d.horizontal = false;
  range2d.vertical = false;
  emit gate_selected();
}

void FormMultiGates::peaks_changed_in_plot()
{
  update_current_gate();
  emit gate_selected();
}

void FormMultiGates::peak_selection_changed(std::set<double> sel)
{
  emit gate_selected();
}

void FormMultiGates::setSpectrum(ProjectPtr newset, int64_t idx)
{
  clear();
  project_ = newset;
  current_spectrum_ = idx;
  SinkPtr spectrum = project_->get_sink(idx);

  if (spectrum)
  {
    md_ = spectrum->metadata();
    uint16_t bits = md_.get_attribute("resolution").value_int;
    res_ = pow(2, bits);
    if (md_.detectors.size() > 1)
    {
      nrg_x_ = md_.detectors[0].best_calib(bits);
      nrg_y_ = md_.detectors[1].best_calib(bits);
    }

    table_model_.set_calib(nrg_y_, bits);

    make_gate();
    if (!gates_.empty())
    {
      ui->tableGateList->selectRow(0);
      ui->gatedSpectrum->update_spectrum();
    }
    emit gate_selected();
  }
}

void FormMultiGates::make_gate()
{
  this->setCursor(Qt::WaitCursor);

  Gate gate = current_gate();

  if (gate.fit_data_.peaks().empty())
  {
    auto slice = make_gated_spectrum(gate.constraints);
    if (slice)
    {
      gate.fit_data_.apply_settings(fit_data_.settings());
      gate.fit_data_.setData(slice);
      fit_data_ = gate.fit_data_;
      update_current_gate();
    }
  }

  fit_data_ = gate.fit_data_;
  ui->gatedSpectrum->update_spectrum();

  this->setCursor(Qt::ArrowCursor);

  if (auto_)
    if (!gate.approved)
      ui->gatedSpectrum->perform_fit();
    else
      fitting_finished();
}

void FormMultiGates::update_current_gate()
{
  Gate gate = current_gate();
  gate.fit_data_ = fit_data_;
  gate.count  = gate.fit_data_.metadata_.get_attribute("total_events").number();

  int32_t index = index_of(gate.constraints.y.centroid(), false);

  if ((gate.constraints.y.centroid() == -1) && (index == -1))
  {
    gates_.clear();
    gates_.push_back(gate);
    index = 0;
  }

  if (index >= 0)
    gates_[index] = gate;

  rebuild_table(true);
}

int32_t FormMultiGates::current_idx() const
{
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index;

  int32_t  idx = -1;
  foreach (index, indexes)
  {
    double centroid = sortModel.data(sortModel.index(index.row(), 1, QModelIndex()),
                                     Qt::EditRole).toDouble();
    idx = index_of(centroid, false);
  }
  return idx;
}

int32_t FormMultiGates::index_of(double centroid, bool fuzzy) const
{
  if (gates_.empty())
    return -1;

  if (fuzzy)
  {
    for (size_t i=0; i < gates_.size(); ++i)
      if (abs(gates_[i].constraints.y.centroid() - centroid) < ui->doubleOverlaps->value())
        return i;
  }
  else
  {
    for (size_t i=0; i < gates_.size(); ++i)
      if (gates_[i].constraints.y.centroid() == centroid)
        return i;
  }

  return -1;
}


Gate FormMultiGates::current_gate() const
{
  int32_t  idx = current_idx();

  if ((idx > -1) && (idx < static_cast<int>(gates_.size())))
    return gates_.at(idx);

  Gate gate;
  gate.approved = false;
  gate.constraints.x.set_centroid(-1);
  gate.constraints.x.set_bounds(0, res_);
  gate.constraints.y.set_centroid(-1);
  gate.constraints.y.set_bounds(0, res_);
  gate.constraints.labelfloat = true;
  gate.constraints.horizontal = true;
  gate.constraints.color = Qt::yellow;

  return gate;
}

void FormMultiGates::selection_changed(QItemSelection selected, QItemSelection deselected)
{
  QModelIndexList indexes = selected.indexes();

  ui->pushRemove->setEnabled(!indexes.empty());
  ui->pushApprove->setEnabled(!indexes.empty());
  ui->pushAddGatedSpectrum->setEnabled(!indexes.empty());
  make_gate();

  emit gate_selected();
}

void FormMultiGates::rebuild_table(bool contents_changed)
{
  if (contents_changed)
    table_model_.set_data(gates_);

  int approved = 0;
  int peaks = 0;
  for (auto &q : gates_)
  {
    if (q.approved)
      approved++;
    if (q.constraints.y.centroid() >= 0)
      peaks += q.fit_data_.peaks().size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(gates_.size() - approved));
  ui->labelPeaks->setText(QString::number(peaks));

  ui->pushDistill->setEnabled(peaks > 0);
}

void FormMultiGates::on_pushRemove_clicked()
{
  int32_t idx = current_idx();

  if ((idx < 0) || (idx >= static_cast<int>(gates_.size())))
    return;

  gates_.erase(gates_.begin() + idx);

  rebuild_table(true);
}

void FormMultiGates::on_pushDistill_clicked()
{
  emit boxes_made();
}

int32_t FormMultiGates::energy_to_bin(double energy) const
{
  return std::round(fit_data_.settings().energy_to_bin(energy));
}

void FormMultiGates::on_doubleGateOn_editingFinished()
{
  emit gate_selected();
}

void FormMultiGates::make_range(double energy)
{
  ui->gatedSpectrum->make_range(energy);
}


Qpx::SinkPtr FormMultiGates::make_gated_spectrum(const Bounds2D &bounds)
{
  SinkPtr source_spectrum;
  if (project_)
    source_spectrum = project_->get_sink(current_spectrum_);

  if (!source_spectrum)
    return nullptr;

  Metadata md = source_spectrum->metadata();

  if (md.dimensions() != 2)
    return nullptr;

  auto x_bounds =  bounds.x.calibrate(nrg_x_, fit_data_.settings().bits_);
  auto y_bounds =  bounds.y.calibrate(nrg_y_, fit_data_.settings().bits_);

  std::string name = md.detectors[0].name_ + x_bounds.bounds_to_string()
      + "  "
      + md.detectors[1].name_ + y_bounds.bounds_to_string();

  //  if (fit_data_.metadata_.get_attribute("name").value_text == name)
  //  {
  //    //      DBG << "same gate";
  //  }
  //  else
  //  {
  SinkPtr gate_x = slice_rectangular(source_spectrum,
          {{bounds.x.lower(), bounds.x.upper()}, {bounds.y.lower(), bounds.y.upper()}}, true);
  Qpx::Setting nm = gate_x->metadata().get_attribute("name");
  nm.value_text =  name;
  gate_x->set_attribute(nm);
  return gate_x;
  //  }
}

void FormMultiGates::choose_peaks(std::set<int64_t> chpeaks)
{
  DBG << "Chpeaks = " << chpeaks.size();
  auto cpeaks = current_peaks();
  std::set<double> xs;
  for (Bounds2D &p : cpeaks)
  {
    if (chpeaks.count(p.id))
    {
      DBG << "Selected peak " << p.x.centroid() << "x" << p.y.centroid();
      xs.insert(p.x.centroid());
    }
  }
  ui->gatedSpectrum->update_selection(xs);
}

void FormMultiGates::on_pushAddGatedSpectrum_clicked()
{
  this->setCursor(Qt::WaitCursor);

  auto gate_x = make_gated_spectrum(current_gate().constraints);

  if (gate_x)
  {
    Setting app = gate_x->metadata().get_attribute("appearance");
    app.value_text = generateColor().name(QColor::HexArgb).toStdString();
    gate_x->set_attribute(app);

    project_->add_sink(gate_x);
    emit projectChanged();
  }

  this->setCursor(Qt::ArrowCursor);
}

void FormMultiGates::on_pushAuto_clicked()
{
  auto_ = true;
  ui->gatedSpectrum->perform_fit();
}

void FormMultiGates::fitting_finished()
{
  if (!auto_)
    return;

  on_pushApprove_clicked();

  std::map<double, Gate> gates;
  for (auto g : gates_)
  {
    Gate gate = g;
    if (!gate.approved)
      gates[gate.count] = gate;
  }

  double c = -1;
  if (!gates.empty())
    c = gates.rbegin()->second.constraints.y.centroid();
  else
    auto_ = false;

  if (c == -1)
    return;

  for (int row=0; row < table_model_.rowCount(); ++row)
  {
    QModelIndex i = sortModel.index(row, 1, QModelIndex());
    double centroid = sortModel.data(i, Qt::EditRole).toDouble();
    if (centroid == c)
    {
      ui->tableGateList->selectRow(row);
      return;
    }
  }
}

void FormMultiGates::on_pushApprove_clicked()
{
  int32_t  idx = current_idx();

  if ((idx < 0) || (idx >= static_cast<int>(gates_.size())))
    return;

  gates_[idx].approved = true;

  if (!fit_data_.peaks().size())
    return;

  Gate gate;
  gate.approved = false;
  gate.constraints.vertical = false;
  gate.constraints.horizontal = true;
  gate.constraints.labelfloat = true;
  gate.constraints.color = Qt::yellow;
  gate.constraints.x.set_centroid(res_/2.0);
  gate.constraints.x.set_bounds(0, res_);

  for (auto &q : fit_data_.peaks())
  {
    const Peak& peak = q.second;

    gate.count = peak.sum4().gross_area().value();

    double w = peak.fwhm().value() * ui->doubleGateOn->value() * 0.5;

    gate.constraints.y.set_centroid(q.second.center().value());
    gate.constraints.y.set_bounds(energy_to_bin(peak.energy().value() - w),
                                  energy_to_bin(peak.energy().value() + w));

    if (index_of(gate.constraints.y.centroid(), true) == -1)
      gates_.push_back(gate);
  }

  rebuild_table(true);
}

std::list<Bounds2D> FormMultiGates::current_peaks() const
{
  std::list<Bounds2D> boxes;

  Bounds2D box = current_gate().constraints;

  if (box.y.centroid() != -1)
    boxes.push_back(box);

  box.labelfloat = (box.y.centroid() == -1);
  box.horizontal = false;
  box.vertical = true;
  box.color = Qt::cyan;
  box.selectable = true;

  int64_t id = 0;
  for (auto &q : fit_data_.peaks())
  {
    const Peak &peak = q.second;

    box.id = id;
    box.selected = ui->gatedSpectrum->get_selected_peaks().count(peak.center().value());

    double w = peak.fwhm().value() * ui->doubleGateOn->value() * 0.5;

    box.x.set_centroid(peak.center().value());
    box.x.set_bounds(energy_to_bin(peak.energy().value() - w),
                     energy_to_bin(peak.energy().value() + w));

    boxes.push_back(box);
    id++;
  }

  if (range2d.selected)
    boxes.push_back(range2d);

  return boxes;
}

std::list<Bounds2D> FormMultiGates::get_all_peaks() const
{
  std::list<Bounds2D> ret;
  int64_t id = 0;
  for (auto &gate : gates_)
  {
    Bounds2D box = gate.constraints;
    if (box.y.centroid() < 0)
      continue;

    box.id = id;
    box.selectable = true;
    box.selected = false;
    box.vertical = false;
    box.horizontal = false;
    box.color = Qt::cyan;

    for (const auto &p : gate.fit_data_.peaks())
    {
      const Peak& peak = p.second;
      double w = peak.fwhm().value() * ui->doubleGateOn->value() * 0.5;

      box.x.set_centroid(peak.center().value());
      box.x.set_bounds(energy_to_bin(peak.energy().value() - w),
                       energy_to_bin(peak.energy().value() + w));
      ret.push_back(box);
    }
    id++;
  }
  return ret;
}







TableGates::TableGates(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TableGates::set_data(std::vector<Gate> gates)
{
  emit layoutAboutToBeChanged();
  gates_ = gates;
  emit layoutChanged();
}

int TableGates::rowCount(const QModelIndex & /*parent*/) const
{
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
      return QVariant::fromValue(gate.count);
    case 1:
      return QVariant::fromValue(gate.constraints.y.centroid());
    case 2:
      return QVariant::fromValue(calib_.transform(gate.constraints.y.centroid(), bits_));
    case 3:
      return QVariant::fromValue(gate.constraints.y.upper() - gate.constraints.y.lower());
    case 4:
      return QVariant::fromValue(
            calib_.transform(gate.constraints.y.upper(), bits_) -
            calib_.transform(gate.constraints.y.lower(), bits_));
    case 5:
      return QVariant::fromValue(gate.fit_data_.peaks().size());
    case 6:
      if (gate.approved)
        return "Processed";
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
        return QString("Intensity (count)");
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
        return QString("Status");
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


void TableGates::update()
{
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}



