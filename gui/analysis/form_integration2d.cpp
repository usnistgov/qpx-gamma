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

  SinkPtr source_spectrum = spectra_->get_sink(current_spectrum_);
  if (source_spectrum) {
    md_ = source_spectrum->metadata();
    uint16_t bits = md_.get_attribute("resolution").value_int;
    if (md_.get_attribute("symmetrized").value_int) {
      uint32_t res = pow(2, bits);
      std::list<MarkerBox2D> northwest;
      std::list<MarkerBox2D> southeast;
      std::list<MarkerBox2D> diagonal;
      for (auto &q : pks) {
        if (q.northwest())
          northwest.push_back(q);
        else if (q.southeast())
          southeast.push_back(q);
        else
          diagonal.push_back(q);
      }
      DBG << "Sorted into NW=" << northwest.size() << " SE=" << southeast.size()
             << " DIAG=" << diagonal.size() << " should take up to "
             << (northwest.size() * southeast.size()) << " tests";

      for (auto &q : northwest) {
        //reflect
        MarkerBox2D nw = q;
        nw.x1 = q.y1;
        nw.y1 = q.x1;
        nw.x2 = q.y2;
        nw.y2 = q.x2;
        nw.x_c = q.y_c;
        nw.y_c = q.x_c;

        bool ok = true;
        for (auto &se : southeast) {
          if (nw.intersects(se)) {
            ok = false;
            break;
          }
        }
        if (ok) {
          diagonal.push_back(nw);
        }
      }

      for (auto &q : southeast)
        diagonal.push_back(q);

      DBG << "downsized to " << diagonal.size();
      if (diagonal.size())
        pks = diagonal;
    }
  }


  peaks_.clear();
  for (auto &q : pks) {
    Peak2D newpeak;
    newpeak.area[1][1] = q;
    peaks_.push_back(newpeak);
  }
  rebuild_table(true);
}

void FormIntegration2D::on_pushRefelct_clicked()
{




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

  //DBG << "indexof checking " << pk.x_c.energy()  << " " << pk.y_c.energy();

  for (size_t i=0; i < peaks_.size(); ++i) {
    //DBG << "   against " << peaks_[i].x_c.energy() << " " << peaks_[i].y_c.energy();

    if ((peaks_[i].area[1][1].x_c == pk.x_c) && (peaks_[i].area[1][1].y_c == pk.y_c))
      return i;
  }
  return -1;
}

int32_t FormIntegration2D::index_of(double px, double py) {
  if (peaks_.empty())
    return -1;

  for (size_t i=0; i < peaks_.size(); ++i)
    if ((peaks_[i].area[1][1].x_c.energy() == px) && (peaks_[i].area[1][1].y_c.energy() == py))
      return i;
  return -1;
}

void FormIntegration2D::make_range(Coord x, Coord y) {
  ui->tableGateList->clearSelection();

  range_.x_c = x;
  range_.y_c = y;
  Calibration f1, f2, e1, e2;

  uint16_t bits = md_.get_attribute("resolution").value_int;

  if (md_.detectors.size() > 1) {
    //DBG << "dets2";
    f1 = md_.detectors[0].fwhm_calibration_;
    f2 = md_.detectors[1].fwhm_calibration_;
    e1 = md_.detectors[0].best_calib(bits);
    e2 = md_.detectors[1].best_calib(bits);
  }
  //  DBG << "making 2d marker, calibs valid " << f1.valid() << " " << f2.valid();

  if (f1.valid()) {
    range_.x1.set_energy(x.energy() - f1.transform(x.energy()) * gate_width_ * 0.5, e1);
    range_.x2.set_energy(x.energy() + f1.transform(x.energy()) * gate_width_ * 0.5, e1);
  } else {
    range_.x1.set_energy(x.energy() - gate_width_ * 2, e1);
    range_.x2.set_energy(x.energy() + gate_width_ * 2, e1);
  }

  if (f2.valid()) {
    range_.y1.set_energy(y.energy() - f2.transform(y.energy()) * gate_width_ * 0.5, e2);
    range_.y2.set_energy(y.energy() + f2.transform(y.energy()) * gate_width_ * 0.5, e2);
  } else {
    range_.y1.set_energy(y.energy() - gate_width_ * 2, e2);
    range_.y2.set_energy(y.energy() + gate_width_ * 2, e2);
  }

  range_.visible = !(x.null() || y.null());
  //  DBG << "range visible " << range_.visible << " x" << range_.x1.energy() << "-" << range_.x2.energy()
  //         << " y" << range_.y1.energy() << "-" << range_.y2.energy();

  make_gates();
  gated_fits_updated(std::set<double>());
  emit range_changed(range_);
}


void FormIntegration2D::update_current_peak(MarkerBox2D peak) {
  int32_t index = index_of(peak);
  //DBG << "update current gate " << index;


  //  if ((index != -1) && (peaks_[index].approved))
  //    gate.approved = true;

  double livetime = md_.get_attribute("live_time").value_duration.total_milliseconds() * 0.001;
  DBG << "update current peak " << peak.x_c.energy()  << " x " << peak.y_c.energy();
  if (livetime == 0)
    livetime = 10000;
  //  gate.cps = gate.fit_data_.metadata_.total_count.convert_to<double>() / livetime;

  if (index < 0) {
    DBG << "non-existing peak. ignoring";
  } else {
    peaks_[index].area[1][1] = peak;
    //current_gate_ = index;
    //DBG << "new current gate = " << current_gate_;
  }

  rebuild_table(true);
}

int32_t FormIntegration2D::current_idx() {
  QModelIndexList indexes = ui->tableGateList->selectionModel()->selectedRows();
  QModelIndex index, i;

  int32_t  current_gate_ = -1;
  //  DBG << "will look at " << indexes.size() << " selected indexes";
  foreach (index, indexes) {
    int row = index.row();
    i = sortModel.index(row, 0, QModelIndex());
    double cx = sortModel.data(i, Qt::EditRole).toDouble();
    i = sortModel.index(row, 1, QModelIndex());
    double cy = sortModel.data(i, Qt::EditRole).toDouble();
    int32_t gate_idx = index_of(cx, cy);
    //    DBG << "selected " << gate_idx << " -> "  << centroid;
    current_gate_ = gate_idx;
  }
  //  DBG << "now current gate is " << current_gate_;
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
  if ((current >= 0) && (current < static_cast<int>(peaks_.size()))) {
    peaks_[current].area[1][1].selected = true;
    make_gates();
  }
  //DBG << "gates " << peaks_.size() << " selections " << indexes.size();

  ui->pushRemove->setEnabled(!indexes.empty());

  //  remake_gate(false);

  emit peak_selected();
}

void FormIntegration2D::rebuild_table(bool contents_changed) {
  //DBG << "rebuild table";

  if (contents_changed) {
    table_model_.set_data(peaks_);
    ui->tableGateList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  }

  int approved = 0;

  for (auto &q : peaks_) {
    //    if (q.approved)
    //      approved++;
    //    if (q.x_c.energy() != -1)
    //      peaks += q.fit_data_.peaks().size();
  }

  ui->labelGates->setText(QString::number(approved) + "/"  + QString::number(peaks_.size() - approved));
}


void FormIntegration2D::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
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
  QSettings settings_;
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
  int32_t  current_gate_ = current_idx();

  //DBG << "trying to remove " << current_gate_ << " in total of " << peaks_.size();

  if (current_gate_ < 0)
    return;
  if (current_gate_ >= static_cast<int>(peaks_.size()))
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
    make_range(range_.x_c, range_.y_c);
  }
}

void FormIntegration2D::setSpectrum(Project *newset, int64_t idx) {
  clear();
  spectra_ = newset;
  current_spectrum_ = idx;
  SinkPtr spectrum = spectra_->get_sink(idx);

  if (spectrum) {
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

  //DBG << "checking selected peaks " << chpeaks.size();

  int32_t idx = -1;
  for (auto &q : chpeaks) {
    idx = index_of(q);
    if (idx >= 0) {
      peaks_[idx].area[1][1].selected = true;
      //      DBG << "*";
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
  if ((current >= 0) && (current < static_cast<int>(peaks_.size())))
    peak = peaks_[current].area[1][1];

  if (peak.visible) {

    SinkPtr source_spectrum = spectra_->get_sink(current_spectrum_);
    if (!source_spectrum)
      return;

    md_ = source_spectrum->metadata();
    uint16_t bits = md_.get_attribute("resolution").value_int;
    double margin = 3;

    uint32_t adjrange = pow(2,bits) - 1;

    double xwidth = peak.x2.bin(bits) - peak.x1.bin(bits);
    double ywidth = peak.y2.bin(bits) - peak.y1.bin(bits);

    double xmin = peak.x1.bin(bits) - xwidth * margin;
    double xmax = peak.x2.bin(bits) + xwidth * margin;
    double ymin = peak.y1.bin(bits) - ywidth * margin;
    double ymax = peak.y2.bin(bits) + ywidth * margin;

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
//      temp.name = "temp";
      uint16_t bits = md_.get_attribute("resolution").value_int;
      temp.set_attribute(md_.get_attribute("resolution"));

      Setting pattern;

      pattern = temp.get_attribute("pattern_coinc");
      pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
      pattern.value_pattern.set_theshold(1);
      temp.set_attribute(pattern);
      pattern = temp.get_attribute("pattern_add");
      pattern.value_pattern.set_gates(std::vector<bool>({1,0}));
      pattern.value_pattern.set_theshold(1);
      temp.set_attribute(pattern);
      gate = SinkFactory::getInstance().create_from_prototype(temp);
      slice_diagonal_x(source_spectrum, gate, peak.x_c.bin(bits), peak.y_c.bin(bits), (xwidth + ywidth) / 2, xmin, xmax);
      fit_x_.setData(gate);

      gate = SinkFactory::getInstance().create_from_prototype(temp);
      slice_diagonal_y(source_spectrum, gate, peak.x_c.bin(bits), peak.y_c.bin(bits), (xwidth + ywidth) / 2, ymin, ymax);
      fit_y_.setData(gate);

    }

  }

//  if ((current >= 0) && (current < peaks_.size())) {
//    Peak2D pk = peaks_[current];
//    if (!ui->pushShowDiagonal->isChecked()) {
//       if (pk.x.peak_count())
//         fit_x_.regions_[pk.x.L()] = pk.x;
//       if (pk.y.peak_count())
//         fit_y_.regions_[pk.y.L()] = pk.y;
//    } else {
//      if (pk.dx.peak_count())
//        fit_x_.regions_[pk.dx.L()] = pk.dx;
//      if (pk.dy.peak_count())
//        fit_y_.regions_[pk.dy.L()] = pk.dy;
//    }
//  }


  if (!ui->pushShowDiagonal->isChecked())
  {
    ui->plotGateX->update_spectrum(/*"Gate X rectangular slice"*/);
    ui->plotGateY->update_spectrum(/*"Gate Y rectangular slice"*/);
  }
  else
  {
    ui->plotGateX->update_spectrum(/*"Gate X diagonal slice"*/);
    ui->plotGateY->update_spectrum(/*"Gate Y diagonal slice"*/);
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

  int32_t current = current_idx();

  if ((current < 0) || (current >= static_cast<int>(peaks_.size())))
      return;

  if (!ui->pushShowDiagonal->isChecked())
    peaks_[current].adjust_x(fit_x_.parent_region(center), center);
  else
    peaks_[current].adjust_diag_x(fit_x_.parent_region(center), center);

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

  int32_t current = current_idx();

  if ((current < 0) || (current >= static_cast<int>(peaks_.size())))
      return;


  if (!ui->pushShowDiagonal->isChecked())
    peaks_[current].adjust_y(fit_y_.parent_region(center), center);
  else
    peaks_[current].adjust_diag_y(fit_y_.parent_region(center), center);

  rebuild_table(true);
  emit peak_selected();
}

void FormIntegration2D::on_pushAdjust_clicked()
{
  adjust_x();
  adjust_y();
  adjust_x();

  int32_t current = current_idx();

  SinkPtr spectrum = spectra_->get_sink(current_spectrum_);

  if ((current < 0) || (current >= static_cast<int>(peaks_.size())) || !spectrum)
      return;

  peaks_[current].integrate(spectrum);
  peaks_[current].approved = true;

  rebuild_table(true);
  emit peak_selected();

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

  Peak2D pk(fit_x_.parent_region(c_x), fit_y_.parent_region(c_y), c_x, c_y);

  peaks_.push_back(pk);

  rebuild_table(true);

  std::list<MarkerBox2D> ch;
  ch.push_back(pk.area[1][1]);
  choose_peaks(ch);

  int32_t current = current_idx();
  SinkPtr spectrum = spectra_->get_sink(current_spectrum_);
  if ((current >= 0) && (current < static_cast<int>(peaks_.size())) && spectrum.operator bool() ) {
    peaks_[current].integrate(spectrum);
    peaks_[current].approved = true;
  }

  rebuild_table(true);

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

  MarkerBox2D peak = peaks_[row].area[1][1];

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(peak.x_c.energy());
    case 1:
      return QVariant::fromValue(peak.y_c.energy());
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

