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
 *      FormIntegration2D -
 *
 ******************************************************************************/

#include "form_integration2d.h"
#include "widget_detectors.h"
#include "ui_form_integration2d.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include <QCloseEvent>
#include "manip2d.h"

FormIntegration2D::FormIntegration2D(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormIntegration2D),
  settings_(settings),
  table_model_(this),
  sortModel(this)
{
  ui->setupUi(this);

  ui->plotGateX->setFit(&fit_x_);
  ui->plotGateY->setFit(&fit_y_);
  ui->plotGateDiagonal->setFit(&fit_d_);

  ui->plotGateX->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines | ShowFitElements::movavg | ShowFitElements::filtered, false);
  ui->plotGateY->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines | ShowFitElements::movavg | ShowFitElements::filtered, false);
  ui->plotGateDiagonal->set_visible_elements(ShowFitElements::gaussians | ShowFitElements::baselines | ShowFitElements::movavg | ShowFitElements::filtered, false);

  loadSettings();

  table_model_.set_data(peaks_);
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

  connect(ui->tableGateList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));
}

void FormIntegration2D::update_range(Range range) {
//  range2d.visible = range.visible;
//  range2d.x1 = range.l;
//  range2d.x2 = range.r;
  update_peaks(false);
}

FormIntegration2D::~FormIntegration2D()
{
  delete ui;
}

void FormIntegration2D::setPeaks(std::list<MarkerBox2D> pks) {
  peaks_ = std::vector<MarkerBox2D>(pks.begin(), pks.end());
  rebuild_table(true);
}


double FormIntegration2D::width_factor() {
  return ui->doubleGateOn->value();
}

void FormIntegration2D::closeEvent(QCloseEvent *event) {
  saveSettings();
  event->accept();
}

int32_t FormIntegration2D::index_of(MarkerBox2D pk) {
  if (peaks_.empty())
    return -1;

  //PL_DBG << "indexof checking " << pk.x_c.energy()  << " " << pk.y_c.energy();

  for (int i=0; i < peaks_.size(); ++i) {
    //PL_DBG << "   against " << peaks_[i].x_c.energy() << " " << peaks_[i].y_c.energy();

    if ((peaks_[i].x_c == pk.x_c) && (peaks_[i].y_c == pk.y_c))
      return i;
  }
  return -1;
}

int32_t FormIntegration2D::index_of(double px, double py) {
  if (peaks_.empty())
    return -1;

  for (int i=0; i < peaks_.size(); ++i)
    if ((peaks_[i].x_c.energy() == px) && (peaks_[i].y_c.energy() == py))
      return i;
  return -1;
}

void FormIntegration2D::make_range(Marker x, Marker y) {
  //ui->tableGateList->clearSelection();

  range_.x_c = x.pos;
  range_.y_c = y.pos;
  Gamma::Calibration f1, f2, e1, e2;

  if (md_.detectors.size() > 1) {
    //PL_DBG << "dets2";
    f1 = md_.detectors[0].fwhm_calibration_;
    f2 = md_.detectors[1].fwhm_calibration_;
    e1 = md_.detectors[0].best_calib(md_.bits);
    e2 = md_.detectors[1].best_calib(md_.bits);
  }
  PL_DBG << "making 2d marker, calibs valid " << f1.valid() << " " << f2.valid();

  if (f1.valid()) {
    range_.x1.set_energy(x.pos.energy() - f1.transform(x.pos.energy()) * 2, e1);
    range_.x2.set_energy(x.pos.energy() + f1.transform(x.pos.energy()) * 2, e1);
  } else {
    //DO THIS!
  }

  if (f2.valid()) {
    range_.y1.set_energy(y.pos.energy() - f2.transform(y.pos.energy()) * 2, e2);
    range_.y2.set_energy(y.pos.energy() + f2.transform(y.pos.energy()) * 2, e2);
  } else {
    //DO THIS!
  }

  range_.visible = (x.visible || y.visible);
  PL_DBG << "range visible " << range_.visible << " x" << range_.x1.energy() << "-" << range_.x2.energy()
         << " y" << range_.y1.energy() << "-" << range_.y2.energy();

  make_gates(range_);
  ui->pushAdd->setEnabled(range_.visible);
  emit range_changed(range_);
}


void FormIntegration2D::update_current_peak(MarkerBox2D peak) {
  int32_t index = index_of(peak);
  //PL_DBG << "update current gate " << index;


//  if ((index != -1) && (peaks_[index].approved))
//    gate.approved = true;

  double livetime = md_.live_time.total_seconds();
  PL_DBG << "update current peak " << peak.x_c.energy()  << " x " << peak.y_c.energy();
  if (livetime == 0)
    livetime = 10000;
//  gate.cps = gate.fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  if (index < 0) {
    PL_DBG << "non-existing peak. ignoring";
  } else {
    peaks_[index] = peak;
    //current_gate_ = index;
    //PL_DBG << "new current gate = " << current_gate_;
  }

  rebuild_table(true);
}

int32_t FormIntegration2D::current_idx() {
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  int32_t  current_gate_ = -1;
  //  PL_DBG << "will look at " << indexes.size() << " selected indexes";
  foreach (index, indexes) {
    int row = index.row();
    i = sortModel.index(row, 0, QModelIndex());
    double cx = sortModel.data(i, Qt::EditRole).toDouble();
    i = sortModel.index(row, 1, QModelIndex());
    double cy = sortModel.data(i, Qt::EditRole).toDouble();
    int32_t gate_idx = index_of(cx, cy);
    //    PL_DBG << "selected " << gate_idx << " -> "  << centroid;
    current_gate_ = gate_idx;
  }
  //  PL_DBG << "now current gate is " << current_gate_;
  return current_gate_;
}

MarkerBox2D FormIntegration2D::current_peak() {
  uint32_t current_peak = current_idx();

  if (current_peak < 0)
    return MarkerBox2D();
  if (current_peak >= peaks_.size()) {
    current_peak = -1;
    return MarkerBox2D();
  } else
    return peaks_[current_peak];
}

void FormIntegration2D::selection_changed(QItemSelection selected, QItemSelection deselected) {
  //current_gate_ = -1;

  QModelIndexList indexes = selected.indexes();

  for (auto &q : peaks_)
    q.selected = false;

  int32_t current = current_idx();
  if ((current >= 0) && (current < peaks_.size())) {
    peaks_[current].selected = true;
    make_gates(peaks_[current]);
  }
  //PL_DBG << "gates " << peaks_.size() << " selections " << indexes.size();

  ui->pushRemove->setEnabled(!indexes.empty());

//  remake_gate(false);

  emit peak_selected();
}

void FormIntegration2D::rebuild_table(bool contents_changed) {
  //PL_DBG << "rebuild table";

  if (contents_changed) {
    table_model_.set_data(peaks_);
  }

  int approved = 0;
  int peaks = 0;
  for (auto &q : peaks_) {
//    if (q.approved)
//      approved++;
//    if (q.x_c.energy() != -1)
//      peaks += q.fit_data_.peaks_.size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(peaks_.size() - approved));
  ui->labelPeaks->setText(QString::number(peaks));
}


void FormIntegration2D::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();


  settings_.beginGroup("Multi_gates");
  ui->doubleGateOn->setValue(settings_.value("gate_on", 2).toDouble());
  ui->doubleOverlaps->setValue(settings_.value("overlaps", 2).toDouble());
  settings_.endGroup();

}


void FormIntegration2D::saveSettings() {
  settings_.beginGroup("Multi_gates");
  settings_.setValue("gate_on", ui->doubleGateOn->value());
  settings_.setValue("overlaps", ui->doubleOverlaps->value());
  settings_.endGroup();
}

void FormIntegration2D::clear() {
  peaks_.clear();
  md_ = Qpx::Spectrum::Metadata();
  rebuild_table(true);

  emit peak_selected();
}
void FormIntegration2D::on_pushRemove_clicked()
{
  uint32_t  current_gate_ = current_idx();

  //PL_DBG << "trying to remove " << current_gate_ << " in total of " << peaks_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= peaks_.size())
    return;

  peaks_.erase(peaks_.begin() + current_gate_);

  rebuild_table(true);
  emit peak_selected();
}

void FormIntegration2D::on_doubleGateOn_editingFinished()
{
//  remake_gate(true);
  emit peak_selected();
}

void FormIntegration2D::on_doubleOverlaps_editingFinished()
{
//  remake_gate(true);
  emit peak_selected();
}


void FormIntegration2D::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
  Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

  if (spectrum && spectrum->resolution()) {
    md_ = spectrum->metadata();
    if (!peaks_.empty()) {
      ui->tableGateList->selectRow(0);
    }
    emit peak_selected();
  }
}


void FormIntegration2D::update_peaks(bool content_changed) {
}

void FormIntegration2D::choose_peaks(std::list<MarkerBox2D> chpeaks) {
  for (auto &q : peaks_)
    q.selected = false;

  //PL_DBG << "checking selected peaks " << chpeaks.size();

  int32_t idx = -1;
  for (auto &q : chpeaks) {
    idx = index_of(q);
    if (idx >= 0) {
      peaks_[idx].selected = true;
//      PL_DBG << "*";
    }
  }

  bool found = false;
  for (int i=0; i < sortModel.rowCount(); ++i) {
    QModelIndex ix = sortModel.index(i, 0);
    QModelIndex iy = sortModel.index(i, 1);
    double x = sortModel.data(ix, Qt::EditRole).toDouble();
    double y = sortModel.data(iy, Qt::EditRole).toDouble();
    for (auto &q : chpeaks) {
      if ((x == q.x_c.energy()) && (y == q.y_c.energy())) {
        found = true;
        ui->tableGateList->selectRow(i);
        break;
      }
    }
    if (found)
      break;
  }

  range_.visible = false;
  emit range_changed(range_);

  //rebuild_table(false);
  //update_peaks(false);
}

void FormIntegration2D::on_pushAdd_clicked()
{
  MarkerBox2D newpeak = range_;
  range_.visible = false;

  newpeak.mark_center = true;
  newpeak.vertical = true;
  newpeak.horizontal = true;
  newpeak.labelfloat = false;
  peaks_.push_back(newpeak);

  rebuild_table(true);
  emit peak_selected();
  emit range_changed(range_);
}

void FormIntegration2D::make_gates(MarkerBox2D peak) {
  Qpx::Spectrum::Spectrum* source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
  if (source_spectrum == nullptr)
    return;

  md_ = source_spectrum->metadata();
  double margin = 1;

  uint32_t adjrange = static_cast<uint32_t>(md_.resolution) - 1;

  double xwidth = peak.x2.bin(md_.bits) - peak.x1.bin(md_.bits);
  double ywidth = peak.y2.bin(md_.bits) - peak.y1.bin(md_.bits);

  double xmin = peak.x1.bin(md_.bits) - xwidth * margin;
  double xmax = peak.x2.bin(md_.bits) + xwidth * margin;
  double ymin = peak.y1.bin(md_.bits) - ywidth * margin;
  double ymax = peak.y2.bin(md_.bits) + ywidth * margin;

  if (xmin < 0)
    xmin = 0;
  if (ymin < 0)
    ymin = 0;
  if (xmax > adjrange)
    xmax = adjrange;
  if (ymax > adjrange)
    ymax = adjrange;

  Qpx::Spectrum::Template *temp;
  Qpx::Spectrum::Spectrum *gate;

  temp = Qpx::Spectrum::Factory::getInstance().create_template("1D");
  temp->visible = true;
  temp->name_ = "temp";
  temp->bits = md_.bits;


  temp->match_pattern = std::vector<int16_t>({1,1});
  temp->add_pattern = std::vector<int16_t>({1,0});
  gate = Qpx::Spectrum::Factory::getInstance().create_from_template(*temp);
  Qpx::Spectrum::slice_rectangular(source_spectrum, gate, {{xmin, xmax}, {ymin, ymax}}, spectra_->runInfo());
  fit_x_.clear();
  fit_x_.setData(gate);
  delete gate;

  temp->match_pattern = std::vector<int16_t>({1,1});
  temp->add_pattern = std::vector<int16_t>({0,1});
  gate = Qpx::Spectrum::Factory::getInstance().create_from_template(*temp);
  Qpx::Spectrum::slice_rectangular(source_spectrum, gate, {{xmin, xmax}, {ymin, ymax}}, spectra_->runInfo());
  fit_y_.clear();
  fit_y_.setData(gate);
  delete gate;

  temp->match_pattern = std::vector<int16_t>({1,0});
  temp->add_pattern = std::vector<int16_t>({1,0});
  gate = Qpx::Spectrum::Factory::getInstance().create_from_template(*temp);
  Qpx::Spectrum::slice_diagonal(source_spectrum, gate, peak.x_c.bin(md_.bits), peak.y_c.bin(md_.bits), xwidth, xmin, xmax, spectra_->runInfo());
  fit_d_.clear();
  fit_d_.setData(gate);
  delete gate;

  ui->plotGateX->clear();
  ui->plotGateX->setFit(&fit_x_);
  ui->plotGateX->update_spectrum();
  ui->plotGateX->tighten();

  ui->plotGateY->clear();
  ui->plotGateY->setFit(&fit_y_);
  ui->plotGateY->update_spectrum();
  ui->plotGateY->tighten();

  ui->plotGateDiagonal->clear();
  ui->plotGateDiagonal->setFit(&fit_d_);
  ui->plotGateDiagonal->update_spectrum();
  ui->plotGateDiagonal->tighten();

  delete temp;
}







TablePeaks2D::TablePeaks2D(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TablePeaks2D::set_data(std::vector<MarkerBox2D> peaks)
{
  //std::reverse(peaks_.begin(),peaks_.end());
  emit layoutAboutToBeChanged();
  peaks_ = peaks;
  /*QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );*/
  emit layoutChanged();
}

int TablePeaks2D::rowCount(const QModelIndex & /*parent*/) const
{
  //PL_DBG << "table has " << peaks_.size();
  return peaks_.size();
}

int TablePeaks2D::columnCount(const QModelIndex & /*parent*/) const
{
  return 3;
}

QVariant TablePeaks2D::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();

  MarkerBox2D peak = peaks_[peaks_.size() - 1 - row];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(peak.x_c.energy());
    case 1:
      return QVariant::fromValue(peak.y_c.energy());
    case 2:
      return QVariant::fromValue(peak.selected);
//    case 3:
//      return QVariant::fromValue(gate.width_chan);
//    case 4:
//      return QVariant::fromValue(gate.width_nrg);
//    case 5:
//      return QVariant::fromValue(gate.fit_data_.peaks_.size());
//    case 6:
//      if (gate.approved)
//        return "Yes";
//      else
//        return "...";
    }
  }
  return QVariant();
}


QVariant TablePeaks2D::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("X (nrg)");
      case 1:
        return QString("Y (nrg)");
      case 2:
        return QString("Selected?");
//      case 3:
//        return QString("Width (chan)");
//      case 4:
//        return QString("Width (keV)");
//      case 5:
//        return QString("Coincidences");
//      case 6:
//        return QString("Approved?");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}


Qt::ItemFlags TablePeaks2D::flags(const QModelIndex &index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index);
}


void TablePeaks2D::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

/*bool TablePeaks2D::setData(const QModelIndex & index, const QVariant & value, int role)
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
