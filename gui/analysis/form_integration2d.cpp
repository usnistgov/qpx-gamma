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
#include "fitter.h"
#include "qt_util.h"
#include <QCloseEvent>
#include "manip2d.h"
#include <QSettings>

using namespace Qpx;

FormIntegration2D::FormIntegration2D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormIntegration2D),
  table_model_(this),
  sortModel(this)
{
  ui->setupUi(this);

  ui->plotGateX->setFit(&fit_x_);
  ui->plotGateY->setFit(&fit_y_);

  connect(ui->plotGateX, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(gated_fits_updated(std::set<double>)));
  connect(ui->plotGateY, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(gated_fits_updated(std::set<double>)));

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

  range_.selectable = false;
  range_.color = Qt::magenta;

  loadSettings();
}

FormIntegration2D::~FormIntegration2D()
{
  delete ui;
}

void FormIntegration2D::closeEvent(QCloseEvent *event)
{
  saveSettings();
  event->accept();
}

void FormIntegration2D::loadSettings()
{
  QSettings settings;

  settings.beginGroup("Integration2d");
  gate_width_ = settings.value("gate_width", 2).toDouble();
  ui->doubleGateOn->setValue(gate_width_);
//  ui->plotGateX->loadSettings(settings); //name these!!!!
//  ui->plotGateY->loadSettings(settings);
  settings.endGroup();

  on_pushShowDiagonal_clicked();
}

void FormIntegration2D::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Integration2d");
  settings.setValue("gate_width", gate_width_);
//  ui->plotGateX->saveSettings(settings); //name these!!!
//  ui->plotGateY->saveSettings(settings);
  settings.endGroup();
}

void FormIntegration2D::setSpectrum(ProjectPtr newset, int64_t idx)
{
  clear();
  project_ = newset;
  current_spectrum_ = idx;
  SinkPtr spectrum = project_->get_sink(idx);

  if (spectrum) {
    md_ = spectrum->metadata();
    if (!peaks_.empty()) {
      ui->tableGateList->selectRow(0);
    }
    emit peak_selected();
  }
}

void FormIntegration2D::setPeaks(std::list<Bounds2D> pks)
{
  SinkPtr source_spectrum = project_->get_sink(current_spectrum_);
  if (source_spectrum)
  {
    md_ = source_spectrum->metadata();
    if (/*md_.get_attribute("symmetrized").value_int*/ true)
    {
      std::list<Bounds2D> northwest;
      std::list<Bounds2D> southeast;
      for (auto &q : pks)
      {
        if (q.northwest())
          northwest.push_back(q);
        else
          southeast.push_back(q);
      }
      DBG << "Sorted into NW=" << northwest.size() << " SE=" << southeast.size()
             << " should take up to "
             << (northwest.size() * southeast.size()) << " tests";

      std::list<Bounds2D> merged;
      for (auto &q : northwest)
      {
        Bounds2D nw = q.reflect();

        bool ok = true;
        for (auto &se : southeast)
          if (nw.intersects(se))
          {
            ok = false;
            break;
          }

        if (ok)
          merged.push_back(nw);
      }

      for (auto &q : southeast)
        merged.push_back(q);

      DBG << "downsized to " << merged.size();
      if (merged.size())
        pks = merged;
    }
  }


  peaks_.clear();
  for (auto &q : pks)
  {
    Sum2D newpeak;
    newpeak.area[1][1] = q;
    peaks_.push_back(newpeak);
  }
  rebuild_table(true);
}

int32_t FormIntegration2D::index_of(double px, double py)
{
  if (peaks_.empty())
    return -1;

  for (size_t i=0; i < peaks_.size(); ++i)
    if ((peaks_[i].area[1][1].x.centroid() == px)
        && (peaks_[i].area[1][1].y.centroid() == py))
      return i;
  return -1;
}

void FormIntegration2D::make_range(double x, double y)
{
  ui->tableGateList->clearSelection();

  Calibration f1, f2, e1, e2;

  uint16_t bits = md_.get_attribute("resolution").value_int;

  if (md_.detectors.size() > 1)
  {
    f1 = md_.detectors[0].fwhm_calibration_;
    f2 = md_.detectors[1].fwhm_calibration_;
    e1 = md_.detectors[0].best_calib(bits);
    e2 = md_.detectors[1].best_calib(bits);
  }

  range_.x.set_centroid(x);
  range_.y.set_centroid(y);

  range_.x.set_bounds(e1.inverse_transform(make_lower(f1, e1.transform(x,bits)), bits),
                      e1.inverse_transform(make_upper(f1, e1.transform(x,bits)), bits));

  range_.y.set_bounds(e2.inverse_transform(make_lower(f2, e2.transform(y,bits)), bits),
                      e2.inverse_transform(make_upper(f2, e2.transform(y,bits)), bits));

  range_.selected = true;

  make_gates();
  gated_fits_updated(std::set<double>());
  emit peak_selected();
}

double FormIntegration2D::make_lower(Qpx::Calibration fw, double center)
{
  if (fw.valid())
    return center - fw.transform(center) * gate_width_ * 0.5;
  else
    return center - gate_width_ * 2;
}

double FormIntegration2D::make_upper(Qpx::Calibration fw, double center)
{
  if (fw.valid())
    return center + fw.transform(center) * gate_width_ * 0.5;
  else
    return center + gate_width_ * 2;
}

int32_t FormIntegration2D::current_idx()
{
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  int32_t  selected_peak = -1;
  //  DBG << "will look at " << indexes.size() << " selected indexes";
  foreach (index, indexes) {
    int row = index.row();
    i = sortModel.index(row, 0, QModelIndex());
    double cx = sortModel.data(i, Qt::EditRole).toDouble();
    i = sortModel.index(row, 1, QModelIndex());
    double cy = sortModel.data(i, Qt::EditRole).toDouble();
    int32_t gate_idx = index_of(cx, cy);
    //    DBG << "selected " << gate_idx << " -> "  << centroid;
    selected_peak = gate_idx;
  }
  //  DBG << "now current gate is " << selected_peak;
  return selected_peak;
}

void FormIntegration2D::clearSelection()
{
  for (auto &q : peaks_)
    q.area[1][1].selected = false;
  range_.selected = false;

  emit peak_selected();
}

void FormIntegration2D::selection_changed(QItemSelection /*selected*/,
                                          QItemSelection /*deselected*/)
{
  range_.selected = false;
  for (auto &q : peaks_)
    q.area[1][1].selected = false;

  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedIndexes();

  int32_t current = current_idx();
  if ((current >= 0) && (current < static_cast<int>(peaks_.size())))
    peaks_[current].area[1][1].selected = true;

  make_gates();
  ui->pushRemove->setEnabled(!indexes.empty());

  //  remake_gate(false);

  emit peak_selected();
}

void FormIntegration2D::rebuild_table(bool contents_changed)
{
  if (contents_changed)
  {
    table_model_.set_data(peaks_);
    ui->tableGateList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  }

  int approved = 0;

  for (auto &q : peaks_)
    if (q.approved)
      approved++;

  ui->labelGates->setText(QString::number(approved) + "/"
                          + QString::number(peaks_.size() - approved));
}

void FormIntegration2D::clear()
{
  peaks_.clear();
  md_ = Metadata();
  rebuild_table(true);
  clearSelection();
}

void FormIntegration2D::on_pushRemove_clicked()
{
  int32_t  selected_peak = current_idx();

  //DBG << "trying to remove " << selected_peak << " in total of " << peaks_.size();

  if (selected_peak < 0)
    return;
  if (selected_peak >= static_cast<int>(peaks_.size()))
    return;

  peaks_.erase(peaks_.begin() + selected_peak);
  reindex_peaks();

  rebuild_table(true);

  std::set<int64_t> selpeaks;
  for (Sum2D& g : peaks_)
    if (!g.approved)
    {
      selpeaks.insert(g.area[1][1].id);
      break;
    }

  choose_peaks(selpeaks);
  emit peak_selected();
}

void FormIntegration2D::on_doubleGateOn_editingFinished()
{
  if (range_.selected && (gate_width_ != ui->doubleGateOn->value()))
  {
    gate_width_ =  ui->doubleGateOn->value();
    make_range(range_.x.centroid(), range_.y.centroid());
  }
}

void FormIntegration2D::choose_peaks(std::set<int64_t> chpeaks)
{
  for (auto &q : peaks_)
    q.area[1][1].selected = false;

  //DBG << "checking selected peaks " << chpeaks.size();

  std::vector<Bounds2D> selpeaks;

  for (auto p : peaks_)
  {
    p.area[1][1].selected = chpeaks.count(p.area[1][1].id);
    if (p.area[1][1].selected)
      selpeaks.push_back(p.area[1][1]);
  }

  bool found = false;
  for (int i=0; i < sortModel.rowCount(); ++i)
  {
    QModelIndex ix = sortModel.index(i, 0);
    QModelIndex iy = sortModel.index(i, 1);
    double x = sortModel.data(ix, Qt::EditRole).toDouble();
    double y = sortModel.data(iy, Qt::EditRole).toDouble();
    for (auto &q : selpeaks)
    {
      if ((x == q.x.centroid()) && (y == q.y.centroid()))
      {
        found = true;
        ui->tableGateList->selectRow(i);
        break;
      }
    }
    if (found)
      break;
  }

  range_.selected = false;
  selection_changed(QItemSelection(), QItemSelection());
  gated_fits_updated(std::set<double>());
  emit peak_selected();
}

void FormIntegration2D::make_gates()
{
  fit_x_.clear();
  fit_y_.clear();

  Bounds2D peak = range_;

  int32_t current = current_idx();
  if ((current >= 0) && (current < static_cast<int>(peaks_.size())))
  {
    peak = peaks_[current].area[1][1];
    if (!ui->pushShowDiagonal->isChecked())
    {
      fit_x_ = peaks_[current].x;
      fit_y_ = peaks_[current].y;
    }
    else
    {
      fit_x_ = peaks_[current].dx;
      fit_y_ = peaks_[current].dy;
    }
  }

  if (peak.selected && (fit_x_.empty() || fit_y_.empty()))
  {
    SinkPtr source_spectrum = project_->get_sink(current_spectrum_);
    if (!source_spectrum)
      return;

    md_ = source_spectrum->metadata();
    uint16_t bits = md_.get_attribute("resolution").value_int;
    double margin = 5; //hack

    uint32_t adjrange = pow(2,bits) - 1;

    double xwidth = peak.x.width();
    double ywidth = peak.y.width();

    double xmin = peak.x.lower() - xwidth * margin;
    double xmax = peak.x.upper() + xwidth * margin;
    double ymin = peak.y.lower() - ywidth * margin;
    double ymax = peak.y.upper() + ywidth * margin;

    if (xmin < 0)
      xmin = 0;
    if (ymin < 0)
      ymin = 0;
    if (xmax > adjrange)
      xmax = adjrange;
    if (ymax > adjrange)
      ymax = adjrange;

    SinkPtr gate;

    if (!ui->pushShowDiagonal->isChecked())
    {
      gate = slice_rectangular(source_spectrum, {{xmin, xmax}, {ymin, ymax}}, true);
      fit_x_.setData(gate);

      gate = slice_rectangular(source_spectrum, {{xmin, xmax}, {ymin, ymax}}, false);
      fit_y_.setData(gate);
    }
    else
    {
      gate = slice_diagonal_x(source_spectrum, peak.x.centroid(), peak.y.centroid(),
                              (xwidth + ywidth) / 2, xmin, xmax);
      fit_x_.setData(gate);

      gate = slice_diagonal_y(source_spectrum, peak.x.centroid(), peak.y.centroid(),
                              (xwidth + ywidth) / 2, ymin, ymax);
      fit_y_.setData(gate);
    }

  }

  ui->plotGateX->update_spectrum();
  ui->plotGateY->update_spectrum();

  gated_fits_updated(std::set<double>());
}


void FormIntegration2D::on_pushShowDiagonal_clicked()
{
  make_gates();
}

void FormIntegration2D::gated_fits_updated(std::set<double> /*dummy*/)
{
  ui->pushAdjust->setEnabled(false);
  ui->pushAddPeak2d->setEnabled(false);

  bool xgood = ((ui->plotGateX->get_selected_peaks().size() == 1) ||
                (fit_x_.peaks().size() == 1));

  bool ygood = ((ui->plotGateY->get_selected_peaks().size() == 1) ||
                (fit_y_.peaks().size() == 1));

  bool good = current_idx() && xgood && ygood;

  if (current_idx() >= 0)
    ui->pushAdjust->setEnabled(good);
  else
    ui->pushAddPeak2d->setEnabled(good && !ui->pushShowDiagonal->isChecked());
}

void FormIntegration2D::adjust(Sum2D& peak)
{
  fit_x_.set_selected_peaks(ui->plotGateX->get_selected_peaks());
  fit_y_.set_selected_peaks(ui->plotGateY->get_selected_peaks());

  if (!ui->pushShowDiagonal->isChecked())
    peak.adjust(fit_x_, fit_y_);
  else
    peak.adjust_diag(fit_x_, fit_y_);

  SinkPtr spectrum = project_->get_sink(current_spectrum_);
  if (spectrum)
  {
    peak.integrate(spectrum);
    peak.approved = true;
  }
}


void FormIntegration2D::on_pushAdjust_clicked()
{
  int32_t current = current_idx();
  if ((current < 0) || (current >= static_cast<int>(peaks_.size())))
      return;

  adjust(peaks_[current]);

  rebuild_table(true);
  emit peak_selected();
}

void FormIntegration2D::on_pushAddPeak2d_clicked()
{
  Sum2D pk;
  pk.area[1][1].id = peaks_.size();
  adjust(pk);

//  pk.area[1][1].selected = true;
  peaks_.push_back(pk);

  rebuild_table(true);

  std::set<int64_t> ch;
  ch.insert(peaks_.size() - 1);
  choose_peaks(ch);

  //  make_gates(range_);
//  emit peak_selected();
}

void FormIntegration2D::reindex_peaks()
{
  int64_t id = 0;
  for (auto p : peaks_)
  {
    p.area[1][1].id = id;
    id++;
  }
}


std::list<Bounds2D> FormIntegration2D::peaks()
{
  std::list<Bounds2D> ret;
  for (auto &q : peaks_)
  {
    if (q.area[1][1] != Bounds2D())
    {
      Bounds2D cbox = q.area[1][1];
      cbox.color = Qt::cyan;
      if (cbox.selected)
      {
        cbox.vertical = true;
        cbox.horizontal = true;
      }
      ret.push_back(cbox);
      if (!cbox.selected)
        continue;
      for (int i=0; i < 3; ++i)
        for (int j=0; j < 3; ++j)
        {
          if ((i == 1) && (j == 1))
            continue;
          Bounds2D box = q.area[i][j];
          if (box != Bounds2D())
          {
            box.id = cbox.id;
            box.color = Qt::cyan;
            box.selected = cbox.selected;
            ret.push_back(box);
          }
        }
    }
  }
  if (range_.selected)
    ret.push_back(range_);
  return ret;
}

void FormIntegration2D::on_pushTransitions_clicked()
{

  std::list<UncertainDouble> all_energies;
  int total_approved = 0;
  for (auto &p : peaks_)
  {
    if (!p.approved || (p.currie_quality_indicator < 1)
        || (p.currie_quality_indicator > 2))
      continue;
    total_approved++;
    all_energies.push_back(p.energy_x);
    all_energies.push_back(p.energy_y);
  }

  std::list<std::pair<UncertainDouble,std::list<UncertainDouble>>> energies;
  int duplicates = 0;
  for (auto &k : all_energies) {
    int found = 0;
    for (auto &e: energies) {
      if (e.first.almost(k)) {
        e.second.push_back(k);
        e.first = (UncertainDouble::average(e.second));
        found++;
      }
    }
    if (!found)
      energies.push_back(std::pair<UncertainDouble,std::list<UncertainDouble>>(k, std::list<UncertainDouble>({k})));
    if (found > 1)
      duplicates++;
  }
  all_energies.clear();
  for (auto &e : energies)
    all_energies.push_back(e.first);

  DBG << "Reduced energies=" << energies.size() << "  duplicates=" << duplicates;

  std::map<UncertainDouble, Transition> trans_final;

  for (auto &p : peaks_)
  {
    if (!p.approved || (p.currie_quality_indicator < 1)
        || (p.currie_quality_indicator > 2))
      continue;

    UncertainDouble e1 = p.energy_x;
    UncertainDouble e2 = p.energy_y;

    for (auto &e : all_energies) {
      if (e.almost(e1))
        e1 = e;
      if (e.almost(e2))
        e2 = e;
    }

    DBG << "energy1 generalized " << p.energy_x.to_string(6) << " --> " << e1.to_string(6);
    DBG << "energy2 generalized " << p.energy_y.to_string(6) << " --> " << e2.to_string(6);

    if (!trans_final.count(e1))
    {
      DBG << "new transition e1->e2";
      Transition t1;
      t1.energy = e1;
      t1.above.push_back(TransitionNode(e2, p.net));
      trans_final[e1] = t1;
    } else {
      bool has = false;
      for (auto &to : trans_final[e1].above)
        if (to.energy.almost(e2))
          has = true;
      if (!has) {
        trans_final[e1].above.push_back(TransitionNode(e2, p.net));
        DBG << "appending e1->e2";
      } else
        DBG << "ignoring duplicate e1->e2";
    }

    if (!trans_final.count(e2))
    {
      DBG << "new transition e2->e1";
      Transition t2;
      t2.energy = e2;
      t2.above.push_back(TransitionNode(e1, p.net));
      trans_final[e2] = t2;
    } else {
      bool has = false;
      for (auto &to : trans_final[e2].above)
        if (to.energy.almost(e1))
          has = true;
      if (!has) {
        trans_final[e2].above.push_back(TransitionNode(e1, p.net));
        DBG << "appending e2->e1";
      } else
        DBG << "ignoring duplicate e2->e1";
    }
  }

  DBG << "Total=" << peaks_.size() << " approved=" << total_approved
         << " final_trans.size=" << trans_final.size();

  for (auto &t : trans_final) {
    DBG << "Transitions from " << t.first.to_string();
    for (auto &to : t.second.above)
      DBG << "    -->" << to.energy.to_string()
             << "  (Intensity: " << to.intensity.to_string() << ")";
  }
}







TablePeaks2D::TablePeaks2D(QObject *parent) :
  QAbstractTableModel(parent)
{
}

void TablePeaks2D::set_data(std::vector<Sum2D> peaks)
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
  //DBG << "table has " << peaks_.size();
  return peaks_.size();
}

int TablePeaks2D::columnCount(const QModelIndex & /*parent*/) const
{
  return 11;
}

QVariant TablePeaks2D::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();

  Bounds2D peak = peaks_[row].area[1][1];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(peak.x.centroid());
    case 1:
      return QVariant::fromValue(peak.y.centroid());
    case 2:
      return QString::fromStdString(peaks_[row].energy_x.to_string()) ;
    case 3:
      return QString::fromStdString(peaks_[row].energy_y.to_string()) ;
    case 4:
      return QString::fromStdString(peaks_[row].xback.to_string()) ;
    case 5:
      return QString::fromStdString(peaks_[row].yback.to_string()) ;
    case 6:
      return QString::fromStdString(peaks_[row].dback.to_string()) ;
    case 7:
      return QString::fromStdString(peaks_[row].gross.to_string()) ;
    case 8:
      return QString::fromStdString(peaks_[row].net.to_string())
          + " (" + QString::fromStdString(peaks_[row].net.error_percent()) + ")";
    case 9:
      return QVariant::fromValue(peaks_[row].currie_quality_indicator);
    case 10:
      return QVariant::fromValue(peaks_[row].approved);

    }
  } else if (role == Qt::BackgroundColorRole) {
    int cqi = peaks_[row].currie_quality_indicator;
    if ((cqi < 0) || (cqi > 2))
      return QBrush(Qt::red);
    else if (cqi == 2)
      return QBrush(Qt::yellow);
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
        return QString("Energy X");
      case 3:
        return QString("Energy Y");
      case 4:
        return QString("xback");
      case 5:
        return QString("yback");
      case 6:
        return QString("dback");
      case 7:
        return QString("gross");
      case 8:
        return QString("net");
      case 9:
        return QString("CQI");
      case 10:
        return QString("approved?");

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

