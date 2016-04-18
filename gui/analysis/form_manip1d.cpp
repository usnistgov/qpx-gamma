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
 *
 ******************************************************************************/

#include "form_manip1d.h"
#include "ui_form_manip1d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"
#include "boost/algorithm/string.hpp"

using namespace Qpx;

TableSpectra1D::TableSpectra1D(Project *spectra, QObject *parent)
  : QAbstractTableModel(parent),
    spectra_(spectra)
{
  std::string str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::vector<char> letters(str.begin(), str.end());

  for (auto &q : letters) {
    char c = q;
    letters_.push_back(std::string(1, q));
  }

  if (spectra_ != nullptr) {
    int i = 0;
    for (auto &q : spectra_->get_sinks(1, -1)) {
      Metadata md;
      if (q.second)
        md = q.second->metadata();

      var_names_.push_back(make_var_name(i));
      data_.push_back(md);
      ++i;
    }
  }
}

std::string TableSpectra1D::make_var_name(int i) {
  std::string varname = letters_[i % letters_.size()];
  int j = i / letters_.size() - 1;
  if (j >= 0)
    varname = make_var_name(j) + varname;
  return varname;
}


int TableSpectra1D::rowCount(const QModelIndex & /*parent*/) const
{
  return data_.size();
}

int TableSpectra1D::columnCount(const QModelIndex & /*parent*/) const
{
  return 9;
}

QVariant TableSpectra1D::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(var_names_[row]);
    case 1:
      return QString::fromStdString(data_[row].name);
    case 2:
      return QString::fromStdString(data_[row].type());
    case 3:
      return QString::number(data_[row].bits);
    case 4:
      return QString::number(pow(2,data_[row].bits));
    case 5:
      return QVariant::fromValue(data_[row].attributes.branches.get(Setting("pattern_coinc")));
    case 6:
      return QVariant::fromValue(data_[row].attributes.branches.get(Setting("pattern_add")));
    case 7:
      return QVariant::fromValue(data_[row].attributes.branches.get(Setting("appearance")));
    case 8:
      return QVariant::fromValue(data_[row].attributes.branches.get(Setting("visible")));
    }

  }
  return QVariant();
}

QVariant TableSpectra1D::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("ID");
      case 1:
        return QString("name");
      case 2:
        return QString("type");
      case 3:
        return QString("bits");
      case 4:
        return QString("channels");
      case 5:
        return QString("match");
      case 6:
        return QString("add");
      case 7:
        return QString("appearance");
      case 8:
        return QString("plot");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableSpectra1D::update() {
  var_names_.clear();
  data_.clear();
  if (spectra_ != nullptr) {
    int i=0;
    for (auto &q : spectra_->get_sinks(1, -1)) {
      Metadata md;
      if (q.second)
        md = q.second->metadata();
      var_names_.push_back(make_var_name(i));
      data_.push_back(md);
      ++i;
    }
  }

  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex(data_.size(), columnCount());
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableSpectra1D::flags(const QModelIndex &index) const
{
  Qt::ItemFlags myflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index);
  return myflags;
}





FormManip1D::FormManip1D(Project& new_set, QWidget *parent) :
  QDialog(parent),
  mySpectra(&new_set),
  table_model_(&new_set, this),
  selection_model_(&table_model_),
  ui(new Ui::FormManip1D)
{
  ui->setupUi(this);

  connect(ui->mcaPlot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->mcaPlot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));


  ui->tableSpectra->setModel(&table_model_);
  ui->tableSpectra->setItemDelegate(&special_delegate_);
  ui->tableSpectra->setSelectionModel(&selection_model_);
  ui->tableSpectra->verticalHeader()->hide();
  ui->tableSpectra->horizontalHeader()->setStretchLastSection(true);
  ui->tableSpectra->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableSpectra->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableSpectra->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableSpectra->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableSpectra->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  table_model_.update();
  ui->tableSpectra->resizeColumnsToContents();
  ui->tableSpectra->resizeRowsToContents();

  ui->mcaPlot->reset_scales();
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearExtras();
  ui->mcaPlot->replot_markers();
  ui->mcaPlot->rescale();
  ui->mcaPlot->redraw();


  updateUI();
  update_plot();
}

void FormManip1D::selection_changed(QItemSelection, QItemSelection) {
  if (selection_model_.selectedIndexes().empty()) {
    spectrumDetails(std::string());

//    ui->pushDelete->setEnabled(false);
  } else {
    //ui->pushDelete->setEnabled(true);
    QModelIndexList ixl = selection_model_.selectedRows();
    if (ixl.empty())
      return;
    int i = ixl.front().row();
    QModelIndex ix = table_model_.index(i, 1);
    QString name = table_model_.data(ix).toString();
    spectrumDetails(name.toStdString());
  }

}

FormManip1D::~FormManip1D()
{
  delete ui;
}

void FormManip1D::spectrumDetails(std::string id)
{
  SinkPtr someSpectrum = mySpectra->get_sink(0);

  Metadata md;

  if (someSpectrum)
    md = someSpectrum->metadata();

  if (id.empty() || (someSpectrum == nullptr)) {
    ui->labelSpectrumInfo->setText("<html><head/><body><p>Select above to see info</p></body></html>");
    return;
  }

  std::string type = someSpectrum->type();
  double real = md.attributes.branches.get(Setting("real_time")).value_duration.total_milliseconds() * 0.001;
  double live = md.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
  double dead = 100;
  double rate = 0;
  Detector det = Detector();
  if (!md.detectors.empty())
    det = md.detectors[0];

  QString detstr("Detector: ");
  detstr += QString::fromStdString(det.name_);
  if (det.energy_calibrations_.has_a(Calibration("Energy", md.bits)))
    detstr += " [ENRG]";
  else if (det.highest_res_calib().valid())
    detstr += " (enrg)";
  if (det.fwhm_calibration_.valid())
    detstr += " [FWHM]";

  if (real > 0) {
    dead = (real - live) * 100.0 / real;
    rate = md.total_count.convert_to<double>() / real;
  }

  QString infoText =
      "<nobr>" + QString::fromStdString(id) + "(" + QString::fromStdString(type) + ", " + QString::number(md.bits) + "bits)</nobr><br/>"
      "<nobr>" + detstr + "</nobr><br/>"
      "<nobr>Count: " + QString::number(md.total_count.convert_to<double>()) + "</nobr><br/>"
      "<nobr>Rate: " + QString::number(rate) + "cps</nobr><br/>"
      "<nobr>Live:  " + QString::number(live) + "s</nobr><br/>"
      "<nobr>Real:  " + QString::number(real) + "s</nobr><br/>"
      "<nobr>Dead:  " + QString::number(dead) + "%</nobr><br/>";

  ui->labelSpectrumInfo->setText(infoText);
}

void FormManip1D::update_plot() {

  this->setCursor(Qt::WaitCursor);
  CustomTimer guiside(true);

  std::map<double, double> minima, maxima;

  calib_ = Calibration();

  ui->mcaPlot->clearGraphs();
  for (auto &q: mySpectra->get_sinks(1, -1)) {
    Metadata md;
    if (q.second)
      md = q.second->metadata();

//    double livetime = md.attributes.branches.get(Setting("live_time")).value_duration.total_milliseconds() * 0.001;
    double rescale  = md.attributes.branches.get(Setting("rescale")).value_precise.convert_to<double>();

    if (md.attributes.branches.get(Setting("visible")).value_int
        && (md.bits > 0) && (md.total_count > 0)) {

      QVector<double> x(pow(2,md.bits));
      QVector<double> y(pow(2,md.bits));

      std::shared_ptr<EntryList> spectrum_data =
          std::move(q.second->data_range({{0, y.size()}}));

      Detector detector = Detector();
      if (!md.detectors.empty())
        detector = md.detectors[0];
      Calibration temp_calib = detector.best_calib(md.bits);

      if (temp_calib.bits_ > calib_.bits_)
        calib_ = temp_calib;

      int i = 0;
      for (auto it : *spectrum_data) {
        double xx = temp_calib.transform(i, md.bits);
        double yy = it.second.convert_to<double>() * rescale;
//        if (ui->pushPerLive->isChecked() && (livetime > 0))
//          yy = yy / livetime;
        x[it.first[0]] = xx;
        y[it.first[0]] = yy;
        if (!minima.count(xx) || (minima[xx] > yy))
          minima[xx] = yy;
        if (!maxima.count(xx) || (maxima[xx] < yy))
          maxima[xx] = yy;
        ++i;
      }

      AppearanceProfile profile;
      profile.default_pen = QPen(QColor(QString::fromStdString(md.attributes.branches.get(Setting("appearance")).value_text)), 1);
      ui->mcaPlot->addGraph(x, y, profile, md.bits);

    }
  }

  ui->mcaPlot->use_calibrated(calib_.valid());
  ui->mcaPlot->setLabels(QString::fromStdString(calib_.units_), "count");
  ui->mcaPlot->setYBounds(minima, maxima);

  replot_markers();

  std::string new_label = boost::algorithm::trim_copy(mySpectra->identity());
  ui->mcaPlot->setTitle(QString::fromStdString(new_label));

  //spectrumDetails(SelectorItem());

  PL_DBG << "<Plot1D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormManip1D::updateUI()
{

//  mySpectra->activate();
}



void FormManip1D::addMovingMarker(double x) {
  PL_INFO << "<Plot1D> marker at " << x;

  if (calib_.valid())
    moving.pos.set_energy(x, calib_);
  else
    moving.pos.set_bin(x, calib_.bits_, calib_);

  moving.visible = true;
  replot_markers();
}

void FormManip1D::removeMovingMarker(double x) {
  moving.visible = false;
  replot_markers();
}

void FormManip1D::replot_markers() {
  std::list<Marker1D> markers;

  markers.push_back(moving);

  ui->mcaPlot->set_markers(markers);
  ui->mcaPlot->replot_markers();
  ui->mcaPlot->redraw();
}

void FormManip1D::on_pushEval_clicked()
{

}
