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

#include "daq_sink_factory.h"
#include "form_integration2d.h"
#include "widget_detectors.h"
#include "ui_form_integration2d.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include <QCloseEvent>
#include "manip2d.h"

using namespace Qpx;

FormIntegration2D::FormIntegration2D(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormIntegration2D),
  settings_(settings),
  table_model_(this),
  sortModel(this),
  current_spectrum_(0)
{
  ui->setupUi(this);

  gate_width_ = 0;

  ui->plotGateX->setFit(&fit_x_);
  ui->plotGateY->setFit(&fit_y_);

  connect(ui->plotGateX, SIGNAL(peak_selection_changed(std::set<double>)), this, SLOT(gated_fits_updated(std::set<double>)));
  connect(ui->plotGateY, SIGNAL(peak_selection_changed(std::set<double>)), this, SLOT(gated_fits_updated(std::set<double>)));

  //loadSettings();

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

FormIntegration2D::~FormIntegration2D()
{
  delete ui;
}

void FormIntegration2D::setPeaks(std::list<MarkerBox2D> pks) {
  peaks_.clear();
  for (auto &q : pks) {
    Peak2D newpeak;
    newpeak.area[1][1] = q;
    peaks_.push_back(newpeak);
  }
  rebuild_table(true);
}


double FormIntegration2D::width_factor() {
  return gate_width_;
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

    if ((peaks_[i].area[1][1].x_c == pk.x_c) && (peaks_[i].area[1][1].y_c == pk.y_c))
      return i;
  }
  return -1;
}

int32_t FormIntegration2D::index_of(double px, double py) {
  if (peaks_.empty())
    return -1;

  for (int i=0; i < peaks_.size(); ++i)
    if ((peaks_[i].area[1][1].x_c.energy() == px) && (peaks_[i].area[1][1].y_c.energy() == py))
      return i;
  return -1;
}

void FormIntegration2D::make_range(Marker x, Marker y) {
  ui->tableGateList->clearSelection();

  range_.x_c = x.pos;
  range_.y_c = y.pos;
  Calibration f1, f2, e1, e2;

  if (md_.detectors.size() > 1) {
    //PL_DBG << "dets2";
    f1 = md_.detectors[0].fwhm_calibration_;
    f2 = md_.detectors[1].fwhm_calibration_;
    e1 = md_.detectors[0].best_calib(md_.bits);
    e2 = md_.detectors[1].best_calib(md_.bits);
  }
  //  PL_DBG << "making 2d marker, calibs valid " << f1.valid() << " " << f2.valid();

  if (f1.valid()) {
    range_.x1.set_energy(x.pos.energy() - f1.transform(x.pos.energy()) * gate_width_, e1);
    range_.x2.set_energy(x.pos.energy() + f1.transform(x.pos.energy()) * gate_width_, e1);
  } else {
    range_.x1.set_energy(x.pos.energy() - gate_width_, e1);
    range_.x2.set_energy(x.pos.energy() + gate_width_, e1);
  }

  if (f2.valid()) {
    range_.y1.set_energy(y.pos.energy() - f2.transform(y.pos.energy()) * gate_width_, e2);
    range_.y2.set_energy(y.pos.energy() + f2.transform(y.pos.energy()) * gate_width_, e2);
  } else {
    range_.y1.set_energy(y.pos.energy() - gate_width_, e2);
    range_.y2.set_energy(y.pos.energy() + gate_width_, e2);
  }

  range_.visible = (x.visible || y.visible);
  //  PL_DBG << "range visible " << range_.visible << " x" << range_.x1.energy() << "-" << range_.x2.energy()
  //         << " y" << range_.y1.energy() << "-" << range_.y2.energy();

  make_gates();
  gated_fits_updated(std::set<double>());
  emit range_changed(range_);
}


void FormIntegration2D::update_current_peak(MarkerBox2D peak) {
  int32_t index = index_of(peak);
  //PL_DBG << "update current gate " << index;


  //  if ((index != -1) && (peaks_[index].approved))
  //    gate.approved = true;

  double livetime = md_.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
  PL_DBG << "update current peak " << peak.x_c.energy()  << " x " << peak.y_c.energy();
  if (livetime == 0)
    livetime = 10000;
  //  gate.cps = gate.fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  if (index < 0) {
    PL_DBG << "non-existing peak. ignoring";
  } else {
    peaks_[index].area[1][1] = peak;
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

//MarkerBox2D FormIntegration2D::current_peak() {
//  uint32_t current_peak = current_idx();

//  if (current_peak < 0)
//    return MarkerBox2D();
//  if (current_peak >= peaks_.size()) {
//    current_peak = -1;
//    return MarkerBox2D();
//  } else
//    return peaks_[current_peak].area[1][1];
//}

void FormIntegration2D::selection_changed(QItemSelection selected, QItemSelection deselected) {
  //current_gate_ = -1;

  QModelIndexList indexes = selected.indexes();

  for (auto &q : peaks_)
    q.area[1][1].selected = false;

  int32_t current = current_idx();
  if ((current >= 0) && (current < peaks_.size())) {
    peaks_[current].area[1][1].selected = true;
    PL_DBG << "selchg";
    make_gates();
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
    //      peaks += q.fit_data_.peaks().size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(peaks_.size() - approved));
  ui->labelPeaks->setText(QString::number(peaks));
}


void FormIntegration2D::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();


  settings_.beginGroup("Integration2d");
  gate_width_ = settings_.value("gate_width", 2).toDouble();
  ui->doubleGateOn->setValue(gate_width_);
  ui->plotGateX->loadSettings(settings_); //name these!!!!
  ui->plotGateY->loadSettings(settings_);
  settings_.endGroup();

  on_pushShowDiagonal_clicked();
}


void FormIntegration2D::saveSettings() {
  settings_.beginGroup("Integration2d");
  settings_.setValue("gate_width", gate_width_);
  ui->plotGateX->saveSettings(settings_); //name these!!!
  ui->plotGateY->saveSettings(settings_);
  settings_.endGroup();
}

void FormIntegration2D::clear() {
  peaks_.clear();
  md_ = Metadata();
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

  choose_peaks(std::list<MarkerBox2D>());
  emit peak_selected();
}

void FormIntegration2D::on_doubleGateOn_editingFinished()
{
  if (range_.visible && (gate_width_ != ui->doubleGateOn->value())) {
    gate_width_ =  ui->doubleGateOn->value();
    PL_DBG << "gateon edited";
    Marker x;
    Marker y;
    x.pos = range_.x_c;
    y.pos = range_.y_c;
    x.visible = true;
    y.visible = true;
    make_range(x, y);
  }
}

void FormIntegration2D::setSpectrum(Project *newset, int64_t idx) {
  clear();
  spectra_ = newset;
  current_spectrum_ = idx;
  SinkPtr spectrum = spectra_->get_sink(idx);

  if (spectrum && spectrum->bits()) {
    md_ = spectrum->metadata();
    if (!peaks_.empty()) {
      ui->tableGateList->selectRow(0);
    }
    emit peak_selected();
  }
}

void FormIntegration2D::choose_peaks(std::list<MarkerBox2D> chpeaks) {
  for (auto &q : peaks_)
    q.area[1][1].selected = false;

  //PL_DBG << "checking selected peaks " << chpeaks.size();

  int32_t idx = -1;
  for (auto &q : chpeaks) {
    idx = index_of(q);
    if (idx >= 0) {
      peaks_[idx].area[1][1].selected = true;
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
  gated_fits_updated(std::set<double>());
  emit range_changed(range_);
}

void FormIntegration2D::make_gates() {
  fit_x_.clear();
  fit_y_.clear();

  MarkerBox2D peak = range_;

  int32_t current = current_idx();
  if ((current >= 0) && (current < peaks_.size()))
    peak = peaks_[current].area[1][1];

  if (peak.visible) {

    SinkPtr source_spectrum = spectra_->get_sink(current_spectrum_);
    if (!source_spectrum)
      return;

    md_ = source_spectrum->metadata();
    double margin = 3;

    uint32_t adjrange = pow(2,md_.bits) - 1;

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

    SinkPtr gate;

    if (!ui->pushShowDiagonal->isChecked()) {

      gate = slice_rectangular(source_spectrum, {{xmin, xmax}, {ymin, ymax}}, true);
      fit_x_.setData(gate);

      gate = slice_rectangular(source_spectrum, {{xmin, xmax}, {ymin, ymax}}, false);
      fit_y_.setData(gate);

    } else  {

      Metadata temp;
      temp = SinkFactory::getInstance().create_prototype("1D");
      //  temp.visible = true;
      temp.name = "temp";
      temp.bits = md_.bits;

      Setting pattern;

      pattern = temp.attributes.branches.get(Setting("pattern_coinc"));
      pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
      pattern.value_pattern.set_theshold(1);
      temp.attributes.branches.replace(pattern);
      pattern = temp.attributes.branches.get(Setting("pattern_add"));
      pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
      pattern.value_pattern.set_theshold(1);
      temp.attributes.branches.replace(pattern);
      gate = SinkFactory::getInstance().create_from_prototype(temp);
      slice_diagonal_x(source_spectrum, gate, peak.x_c.bin(md_.bits), peak.y_c.bin(md_.bits), (xwidth + ywidth) / 2, xmin, xmax);
      fit_x_.setData(gate);

      gate = SinkFactory::getInstance().create_from_prototype(temp);
      slice_diagonal_y(source_spectrum, gate, peak.x_c.bin(md_.bits), peak.y_c.bin(md_.bits), (xwidth + ywidth) / 2, ymin, ymax);
      fit_y_.setData(gate);

    }

  }

  if ((current >= 0) && (current < peaks_.size())) {
    Peak2D pk = peaks_[current];
    if (!ui->pushShowDiagonal->isChecked()) {
       if (!pk.x.peaks_.empty())
         fit_x_.regions_[pk.x.finder_.x_.front()] = pk.x;
       if (!pk.y.peaks_.empty())
         fit_y_.regions_[pk.y.finder_.x_.front()] = pk.y;
    } else {
      if (!pk.dx.peaks_.empty())
        fit_x_.regions_[pk.dx.finder_.x_.front()] = pk.dx;
      if (!pk.dy.peaks_.empty())
        fit_y_.regions_[pk.dy.finder_.x_.front()] = pk.dy;
    }
  }


  if (!ui->pushShowDiagonal->isChecked()) {
    ui->plotGateX->update_spectrum("Gate X rectangular slice");
    ui->plotGateY->update_spectrum("Gate Y rectangular slice");
  } else {
    ui->plotGateX->update_spectrum("Gate X diagonal slice");
    ui->plotGateY->update_spectrum("Gate Y diagonal slice");
  }

  gated_fits_updated(std::set<double>());
}


void FormIntegration2D::on_pushShowDiagonal_clicked()
{
  make_gates();
}

void FormIntegration2D::gated_fits_updated(std::set<double> dummy) {

  ui->pushAdjust->setEnabled(false);
  ui->pushAddPeak2d->setEnabled(false);

  bool xgood = ((ui->plotGateX->get_selected_peaks().size() == 1) ||
                (fit_x_.peaks().size() == 1));

  bool ygood = ((ui->plotGateY->get_selected_peaks().size() == 1) ||
                (fit_y_.peaks().size() == 1));

  if (current_idx() >= 0) {
    ui->pushAdjust->setEnabled(xgood || ygood);
  } else
    ui->pushAddPeak2d->setEnabled(xgood && ygood && !ui->pushShowDiagonal->isChecked());
}

void FormIntegration2D::adjust_x()
{
  double center;

  std::set<double> selected_x = ui->plotGateX->get_selected_peaks();
  if (selected_x.size() == 1)
    center = *selected_x.begin();
  else if (fit_x_.peaks().size() == 1)
    center = fit_x_.peaks().begin()->first;
  else
    return;

  ROI *roi = fit_x_.parent_of(center);

  if (!roi)
    return;

  int32_t current = current_idx();

  if ((current < 0) || (current >= peaks_.size()))
      return;

  if (!ui->pushShowDiagonal->isChecked())
    peaks_[current].adjust_x(*roi, center);
  else
    peaks_[current].adjust_diag_x(*roi, center);

  rebuild_table(true);
  emit peak_selected();
}

void FormIntegration2D::adjust_y()
{
  double center;

  std::set<double> selected_y = ui->plotGateY->get_selected_peaks();
  if (selected_y.size() == 1)
    center = *selected_y.begin();
  else if (fit_y_.peaks().size() == 1)
    center = fit_y_.peaks().begin()->first;
  else
    return;

  ROI *roi = fit_y_.parent_of(center);

  if (!roi)
    return;

  int32_t current = current_idx();

  if ((current < 0) || (current >= peaks_.size()))
      return;


  if (!ui->pushShowDiagonal->isChecked())
    peaks_[current].adjust_y(*roi, center);
  else
    peaks_[current].adjust_diag_y(*roi, center);

  rebuild_table(true);
  emit peak_selected();
}

void FormIntegration2D::on_pushAdjust_clicked()
{
  adjust_x();
  adjust_y();
  adjust_x();
  //redundant refreshes and signals from above calls
}

void FormIntegration2D::on_pushAddPeak2d_clicked()
{
  double c_x;
  double c_y;

  std::set<double> selected_x = ui->plotGateX->get_selected_peaks();
  if (selected_x.size() == 1)
    c_x = *selected_x.begin();
  else if (fit_x_.peaks().size() == 1)
    c_x = fit_x_.peaks().begin()->first;
  else
    return;

  std::set<double> selected_y = ui->plotGateY->get_selected_peaks();
  if (selected_y.size() == 1)
    c_y = *selected_y.begin();
  else if (fit_y_.peaks().size() == 1)
    c_y = fit_y_.peaks().begin()->first;
  else
    return;

  ROI *xx_roi = fit_x_.parent_of(c_x);
  ROI *yy_roi = fit_y_.parent_of(c_y);

  if (!xx_roi || !yy_roi)
    return;

  Peak2D pk(*xx_roi, *yy_roi, c_x, c_y);

  peaks_.push_back(pk);

  rebuild_table(true);

  std::list<MarkerBox2D> ch;
  ch.push_back(pk.area[1][1]);
  choose_peaks(ch);

  //  make_gates(range_);
  emit peak_selected();
}

std::list<MarkerBox2D> FormIntegration2D::peaks() {
  std::list<MarkerBox2D> ret;
  for (auto &q : peaks_) {
    if (q.area[1][1].visible) {
      for (int i=0; i < 3; ++i)
        for (int j=0; j < 3; ++j) {
          MarkerBox2D box = q.area[i][j];
          if (q.area[1][1].selected)
            box.visible = true;
          box.selected = q.area[1][1].selected;
          ret.push_back(box);
        }
    }
  }
  return ret;
}






TablePeaks2D::TablePeaks2D(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TablePeaks2D::set_data(std::vector<Peak2D> peaks)
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
  return 6;
}

QVariant TablePeaks2D::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();

  MarkerBox2D peak = peaks_[row].area[1][1];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(peak.x_c.energy());
    case 1:
      return QVariant::fromValue(peak.y_c.energy());
    case 2:
      return QVariant::fromValue(peak.x_c.bin(peak.x_c.bits()));
    case 3:
      return QVariant::fromValue(peak.y_c.bin(peak.y_c.bits()));
    case 4:
      return QString::number(peak.integral) + " +/- " + QString::number(sqrt(peak.integral));
    case 5:
      return QVariant::fromValue(peaks_[row].approved);
      //    case 3:
      //      return QVariant::fromValue(gate.width_chan);
      //    case 4:
      //      return QVariant::fromValue(gate.width_nrg);
      //    case 5:
      //      return QVariant::fromValue(gate.fit_data_.peaks().size());
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
        return QString("X (bin)");
      case 3:
        return QString("Y (bin)");
      case 4:
        return QString("peak area");
      case 5:
        return QString("approved?");
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

