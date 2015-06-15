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
 *      FormCalibration -
 *
 ******************************************************************************/

#include "form_calibration.h"
#include "widget_detectors.h"
#include "ui_form_calibration.h"
#include "poly_fit.h"
#include "qt_util.h"

FormCalibration::FormCalibration(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormCalibration),
  marker_table_(my_markers_),
  settings_(settings),
  selection_model_(&marker_table_)
{
  ui->setupUi(this);

  loadSettings();

  qRegisterMetaType<Gaussian>("Gaussian");
  qRegisterMetaType<QVector<Gaussian>>("QVector<Gaussian>");

  //file formats, should be in detector db widget
  std::vector<std::string> spectypes = Pixie::Spectrum::Factory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    Pixie::Spectrum::Template* type_template = Pixie::Spectrum::Factory::getInstance().create_template(q);
    if (!type_template->input_types.empty())
      filetypes.push_back("Spectrum " + QString::fromStdString(q) + "(" + catExtensions(type_template->input_types) + ")");
    delete type_template;
  }
  load_formats_ = catFileTypes(filetypes);
  ui->plot1D->use_calibrated(true);

  ui->PlotCalib->set_scale_type("Linear");
  ui->PlotCalib->showButtonColorThemes(false);
  ui->PlotCalib->showButtonMarkerLabels(false);
  ui->PlotCalib->showButtonPlotStyle(false);
  ui->PlotCalib->showButtonScaleType(false);
  ui->PlotCalib->setZoomable(false);
  ui->PlotCalib->showTitle(false);
  ui->PlotCalib->setLabels("channel", "energy");

  moving.themes["light"] = QPen(Qt::darkRed, 2);
  moving.themes["dark"] = QPen(Qt::red, 2);

  list.themes["light"] = QPen(Qt::darkGray, 1);
  list.themes["dark"] = QPen(Qt::white, 1);
  list.visible = true;

  selected.themes["light"] = QPen(Qt::black, 2);
  selected.themes["dark"] = QPen(Qt::yellow, 2);
  selected.visible = true;

  ui->tableMarkers->setModel(&marker_table_);
  ui->tableMarkers->setSelectionModel(&selection_model_);
  ui->tableMarkers->setItemDelegate(&special_delegate_);
  ui->tableMarkers->verticalHeader()->hide();
  ui->tableMarkers->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableMarkers->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableMarkers->horizontalHeader()->setStretchLastSection(true);
  ui->tableMarkers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableMarkers->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));
  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
}

FormCalibration::~FormCalibration()
{
  delete ui;
}

void FormCalibration::closeEvent(QCloseEvent *event) {
  if (!ui->isotopes->save_close()) {
    event->ignore();
    return;
  }

  clear();

  saveSettings();
  event->accept();
}

void FormCalibration::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->isotopes->setDir(data_directory_);

  settings_.beginGroup("Calibration");
  ui->spinMinPeakWidth->setValue(settings_.value("min_peak_width", 5).toInt());
  ui->spinMovAvg->setValue(settings_.value("moving_avg_window", 15).toInt());
  ui->checkShowMovAvg->setChecked(settings_.value("show_moving_avg", false).toBool());
  ui->checkShowFilteredPeaks->setChecked(settings_.value("show_filtered_peaks", false).toBool());
  ui->checkShowPrelimPeaks->setChecked(settings_.value("show_prelim_peaks", false).toBool());
  ui->checkShowGaussians->setChecked(settings_.value("show_gaussians", false).toBool());
  ui->checkShowBaselines->setChecked(settings_.value("show_baselines", false).toBool());

  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->isotopes->set_current_isotope(settings_.value("current_isotope", "Co-60").toString());

  settings_.beginGroup("McaPlot");
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plot1D->set_plot_style(settings_.value("plot_style", "Step").toString());
  ui->plot1D->set_marker_labels(settings_.value("marker_labels", true).toBool());
  settings_.endGroup();

  settings_.endGroup();
}

void FormCalibration::saveSettings() {
  settings_.beginGroup("Calibration");
  settings_.setValue("min_peak_width", ui->spinMinPeakWidth->value());
  settings_.setValue("moving_avg_window", ui->spinMovAvg->value());
  settings_.setValue("show_moving_avg", ui->checkShowMovAvg->isChecked());
  settings_.setValue("show_prelim_peaks", ui->checkShowPrelimPeaks->isChecked());
  settings_.setValue("show_filtered_peaks", ui->checkShowFilteredPeaks->isChecked());
  settings_.setValue("show_gaussians", ui->checkShowGaussians->isChecked());
  settings_.setValue("show_baselines", ui->checkShowBaselines->isChecked());
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("current_isotope", ui->isotopes->current_isotope());

  settings_.beginGroup("McaPlot");
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.setValue("plot_style", ui->plot1D->plot_style());
  settings_.setValue("marker_labels", ui->plot1D->marker_labels());
  settings_.endGroup();

  settings_.endGroup();
}

void FormCalibration::setData(XMLableDB<Pixie::Detector>& newDetDB) {
  detectors_ = &newDetDB;
  toggle_radio();
}

void FormCalibration::clear() {
  peaks_.clear();
  my_markers_.clear();
  current_spectrum_.clear();
  new_calibration_ = Pixie::Calibration();
  old_calibration_ = Pixie::Calibration();
  detector_ = Pixie::Detector();
  x_chan.clear(); y.clear();
  spectrum_data_ = UtilXY();
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  toggle_radio();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();
  ui->plot1D->reset_scales();
  ui->pushFromDB->setEnabled(false);
  update_plot();
}


void FormCalibration::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  spectra_ = newset;
  clear();

  if (!spectra_)
    return;

  peaks_.clear();
  Pixie::Spectrum::Spectrum *spectrum;
  if ((spectrum = spectra_->by_name(name.toStdString())) &&
      (spectrum->resolution())) {
    current_spectrum_ = name;
    int bits = spectrum->bits();
    old_calibration_ = new_calibration_ = Pixie::Calibration(bits);
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];
        if (detector_.energy_calibrations_.has_a(Pixie::Calibration(bits))) {
          new_calibration_ = old_calibration_ = detector_.energy_calibrations_.get(Pixie::Calibration(bits));
          PL_INFO << "Old calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "No existing calibration for this resolution";
        ui->plot1D->setFloatingText("Spectrum=" + current_spectrum_ + "  resolution=" + QString::number(bits) + "bits  Detector=" + QString::fromStdString(detector_.name_));
      }
    }
  }

  ui->plot1D->reset_scales();
  ui->plot1D->redraw();

  update_plot();
  toggle_radio();
}

void FormCalibration::update_plot() {
  if (!spectra_)
    return;

  std::map<double, double> minima, maxima;

  Pixie::Spectrum::Spectrum *spectrum;
  if (!(spectrum = spectra_->by_name(current_spectrum_.toStdString()))) {
    return;
  }
  if (spectrum->resolution()) {
    x_chan.resize(spectrum->resolution());
    y.resize(spectrum->resolution());

    std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_dump =
        std::move(spectrum->get_spectrum({{0, y.size()}}));

    int i = 0;
    for (auto it : *spectrum_dump) {
      double yy = it.second;
      double xx = static_cast<double>(i);
      x_chan[i] = xx;
      y[i] = yy;
      if (!minima.count(xx) || (minima[xx] > yy))
        minima[xx] = yy;
      if (!maxima.count(xx) || (maxima[xx] < yy))
        maxima[xx] = yy;
      ++i;
    }

    ui->plot1D->clearGraphs();
    ui->plot1D->addGraph(x_chan, y, Qt::gray, 1);

    spectrum_data_ = UtilXY(x_chan.toStdVector(), y.toStdVector(), ui->spinMovAvg->value());
    spectrum_data_.find_prelim();
    spectrum_data_.filter_prelim(ui->spinMinPeakWidth->value());

    if (ui->checkShowMovAvg->isChecked())
      plot_derivs(spectrum_data_);

    QVector<double> xx, yy;

    if (ui->checkShowFilteredPeaks->isChecked()) {
      xx.clear(); yy.clear();
      for (auto &q : spectrum_data_.filtered) {
        xx.push_back(q);
        yy.push_back(y[q]);
      }
      if (yy.size())
        ui->plot1D->addPoints(xx, yy, Qt::blue, 6, QCPScatterStyle::ssDiamond);
    }

    if (ui->checkShowPrelimPeaks->isChecked()) {
      xx.clear(); yy.clear();
      for (auto &q : spectrum_data_.prelim) {
        xx.push_back(q);
        yy.push_back(y[q]);
      }
      if (yy.size())
        ui->plot1D->addPoints(xx, yy, Qt::black, 4, QCPScatterStyle::ssDiamond);
    }

    for (auto &q : peaks_) {
      if (ui->checkShowBaselines->isChecked())
        ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_baseline_), Qt::blue, 1);
      if (ui->checkShowGaussians->isChecked())
        ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_fullfit_), Qt::darkYellow, 1);
    }

    ui->plot1D->setLabels("channel", "counts");
  }
  ui->plot1D->setYBounds(minima, maxima);
  replot_markers();
}

void FormCalibration::addMovingMarker(double x) {
  moving.channel = x;
  moving.chan_valid = true;
  moving.energy = old_calibration_.transform(x);
  moving.energy_valid = (moving.channel != moving.energy);
  moving.visible = true;
  ui->pushAdd->setEnabled(true);
  PL_INFO << "Marker at " << moving.channel << " originally caliblated to " << old_calibration_.transform(moving.channel)
          << ", new calibration = " << new_calibration_.transform(moving.channel);
  replot_markers();
}

void FormCalibration::removeMovingMarker(double x) {
  moving.visible = false;
  ui->pushAdd->setEnabled(false);
  replot_markers();
}


void FormCalibration::replot_markers() {
  std::list<Marker> markers;

  ui->PlotCalib->clearGraphs();
  ui->plot1D->clearExtras();

  QVector<double> xx, yy, ycab;

  markers.push_back(moving);
  for (auto &q : my_markers_) {
    Marker m = list;
    m.channel = q.first;
    m.energy = q.second;
    m.chan_valid = true;
    m.energy_valid = (q.first != q.second);
    markers.push_back(m);

    xx.push_back(q.first);
    yy.push_back(q.second);
  }

  if (xx.size()) {
    ui->PlotCalib->reset_scales();
    ui->PlotCalib->addPoints(xx, yy, Qt::darkBlue, 7, QCPScatterStyle::ssDiamond);
    if (new_calibration_.units_ != "channels") {
      double min = xx[0], max = xx[0];
      for (auto &q : xx) {
        if (q < min)
          min = q;
        if (q > max)
          max = q;
      }

      max = max + abs(min*2);
      min = min - abs(min*2);
      double step = (max-min) / 50.0;

      Polynomial thispoly(new_calibration_.coefficients_);
      xx.clear();
      for (double i=min; i <= max; i+=step) {
        xx.push_back(i);
        ycab.push_back(thispoly.evaluate(i));
      }
      ui->PlotCalib->addGraph(xx, ycab, Qt::blue, 2);
    }
    ui->PlotCalib->rescale();
  }
  ui->PlotCalib->redraw();

  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 0);
      QModelIndex nrg_ix = marker_table_.index(idx.row(), 1);
      double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      double nrg = marker_table_.data(nrg_ix, Qt::DisplayRole).toDouble();
      Marker sm = selected;
      sm.channel = chan;
      sm.energy = nrg;
      sm.chan_valid = true;
      sm.energy_valid = (sm.channel != sm.energy);
      markers.push_back(sm);
    }
  }

  ui->plot1D->set_markers(markers);
  ui->plot1D->replot_markers();
  ui->plot1D->redraw();
}

void FormCalibration::on_pushAdd_clicked()
{
  my_markers_[moving.channel] = old_calibration_.transform(moving.channel);

  ui->pushAdd->setEnabled(false);
  marker_table_.update();
  toggle_push();

  removeMovingMarker(0);
}

void FormCalibration::selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
  replot_markers();
}

void FormCalibration::toggle_push() {
  ui->pushMarkerRemove->setEnabled(false);
  ui->pushAllEnergies->setEnabled(false);
  ui->pushAllmarkers->setEnabled(false);

  if (!selection_model_.selectedIndexes().empty()) {
    ui->pushMarkerRemove->setEnabled(true);

    if (selection_model_.selectedRows().size() == ui->isotopes->current_gammas().size())
      ui->pushAllEnergies->setEnabled(true);

    if (!ui->isotopes->current_isotope().isEmpty() && (ui->isotopes->current_gammas().empty()))
      ui->pushAllmarkers->setEnabled(true);
  }

  if (static_cast<int>(my_markers_.size()) >= 2) {
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
  }

  if (detectors_->has_a(detector_) && detectors_->get(detector_).energy_calibrations_.has_a(old_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
}

void FormCalibration::toggle_radio() {

  Polynomial thispoly(new_calibration_.coefficients_);
  ui->plot1D->setTitle("E = " + QString::fromStdString(thispoly.to_markup()));
  ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(thispoly.to_UTF8()));

  ui->pushApplyCalib->setEnabled(false);
  ui->comboApplyTo->clear();

  if (!new_calibration_.shallow_equals(Pixie::Calibration()) && !detector_.shallow_equals(Pixie::Detector())) {
    ui->pushApplyCalib->setEnabled(true);
    Pixie::Spectrum::Spectrum* spectrum;
    bool others_have_det = false;
    for (auto &q : spectra_->spectra()) {
      if (q && (q->name() != current_spectrum_.toStdString())) {
        for (auto &p : q->get_detectors())
          if (p.shallow_equals(detector_))
            others_have_det = true;
      }
    }
    if (others_have_det)
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on all open spectra", QVariant::fromValue((int)CalibApplyTo::DetOnAllOpen));
    if (spectrum  = spectra_->by_name(current_spectrum_.toStdString())) {
      for (auto &q : spectrum->get_detectors()) {
        if (q.shallow_equals(detector_))
          ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on " + current_spectrum_, QVariant::fromValue((int)CalibApplyTo::DetOnCurrentSpec));
      }
      ui->comboApplyTo->addItem("all detectors on " + current_spectrum_, QVariant::fromValue((int)CalibApplyTo::AllDetsOnCurrentSpec));
    }
    if (detectors_ && detectors_->has_a(detector_))
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " in detector database", QVariant::fromValue((int)CalibApplyTo::DetInDB));
  }

}

void FormCalibration::on_pushMarkerRemove_clicked()
{
  std::list<double> to_remove;
  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 0);
      double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      to_remove.push_back(chan);
      my_markers_.erase(chan);
    }
  }

  selection_model_.reset();
  marker_table_.update();
  toggle_push();

  for (auto &q : peaks_) {
    for (auto &p : to_remove)
      if (q.gaussian_.center_ == p)
        q = Peak();
  }
  update_plot();
}

void FormCalibration::on_pushFit_clicked()
{  
  std::vector<double> x, y, coefs(ui->spinTerms->value()+1);
  x.resize(my_markers_.size());
  y.resize(my_markers_.size());
  int i = 0;
  for (auto &q : my_markers_) {
    x[i] = q.first;
    y[i] = q.second;
    i++;
  }

  Polynomial p = Polynomial(x, y, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    new_calibration_.coefficients_ = p.coeffs_;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = Pixie::CalibrationModel::polynomial;
    PL_INFO << "Calibration coefficients = " << new_calibration_.coef_to_string();
    toggle_radio();
  }
  else
    PL_INFO << "Calibration failed";

  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormCalibration::isotope_energies_chosen() {
  toggle_push();
}

void FormCalibration::on_pushApplyCalib_clicked()
{
  PL_INFO << "<Calibration> Applying calibration";

  if ((ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetInDB) && detectors_->has_a(detector_)) {
    Pixie::Detector modified = detectors_->get(detector_);
    if (modified.energy_calibrations_.has_a(new_calibration_))
      modified.energy_calibrations_.replace(new_calibration_);
    else
      modified.energy_calibrations_.add(new_calibration_);
    detectors_->replace(modified);
    emit detectorsChanged();
  } else {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      bool is_selected_spectrum = (q->name() == current_spectrum_.toStdString());
      if ((ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetOnAllOpen) || is_selected_spectrum) {
        std::vector<Pixie::Detector> detectors = q->get_detectors();
        for (auto &p : detectors) {
          if ((is_selected_spectrum && (ui->comboApplyTo->currentData() == (int)CalibApplyTo::AllDetsOnCurrentSpec)) ||
              (is_selected_spectrum && (ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetOnCurrentSpec) && p.shallow_equals(detector_)) ||
              ((ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetOnAllOpen) && p.shallow_equals(detector_))) {
            PL_INFO << "   applying new calibration for " << detector_.name_ << " on " << q->name();
            if (p.energy_calibrations_.has_a(new_calibration_))
              p.energy_calibrations_.replace(new_calibration_);
            else
              p.energy_calibrations_.add(new_calibration_);
          }
        }
        q->set_detectors(detectors);
      }
      emit calibrationComplete();
    }
  }

}

void FormCalibration::on_pushFromDB_clicked()
{
  Pixie::Detector newdet = detectors_->get(detector_);
  new_calibration_ = newdet.energy_calibrations_.get(old_calibration_);
  toggle_radio();
}

void FormCalibration::on_pushFindPeaks_clicked()
{
  this->setCursor(Qt::WaitCursor);

  my_markers_.clear();

  UtilXY xy(x_chan.toStdVector(), y.toStdVector(), ui->spinMovAvg->value());
  xy.find_peaks(ui->spinMinPeakWidth->value());
  peaks_ = xy.peaks_;
  for (auto &q : peaks_) {
    double chan = q.gaussian_.center_;
    my_markers_[chan] = old_calibration_.transform(chan);
  }
  marker_table_.update();
  toggle_push();
  update_plot();

  this->setCursor(Qt::ArrowCursor);
}

void FormCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(*detectors_, data_directory_, load_formats_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormCalibration::on_pushRefresh_clicked()
{
  update_plot();
}

void FormCalibration::plot_derivs(UtilXY &data)
{
  QVector<double> temp_y, temp_x;
  int was = 0, is = 0;

  for (int i = 0; i < y.size(); ++i) {
    if (data.deriv1[i] > 0)
      is = 1;
    else if (data.deriv1[i] < 0)
      is = -1;
    else
      is = 0;

    if ((was != is) && (temp_x.size()))
    {
      if (temp_x.size() > ui->spinMinPeakWidth->value()) {
        if (was == 1)
          ui->plot1D->addGraph(temp_x, temp_y, Qt::green, 1);
        else if (was == -1)
          ui->plot1D->addGraph(temp_x, temp_y, Qt::red, 1);
        else
          ui->plot1D->addGraph(temp_x, temp_y, Qt::black, 1);
      }
      temp_x.clear(); temp_x.push_back(i-1);
      temp_y.clear(); temp_y.push_back(data.y_avg_[i-1]);
    }

    was = is;
    temp_y.push_back(data.y_avg_[i]);
    temp_x.push_back(i);
  }

  if (temp_x.size())
  {
    if (was == 1)
      ui->plot1D->addGraph(temp_x, temp_y, Qt::green, 1);
    else if (was == -1)
      ui->plot1D->addGraph(temp_x, temp_y, Qt::red, 1);
    else
      ui->plot1D->addGraph(temp_x, temp_y, Qt::black, 1);
  }
}

void FormCalibration::on_checkShowMovAvg_clicked()
{
  update_plot();
}

void FormCalibration::on_checkShowPrelimPeaks_clicked()
{
  update_plot();
}

void FormCalibration::on_checkShowGaussians_clicked()
{
  update_plot();
}

void FormCalibration::on_checkShowBaselines_clicked()
{
  update_plot();
}

void FormCalibration::on_checkShowFilteredPeaks_clicked()
{
  update_plot();
}

void FormCalibration::on_pushAllmarkers_clicked()
{
  std::vector<double> gammas;
  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 1);
      double nrg = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      gammas.push_back(nrg);
    }
  }
  ui->isotopes->push_energies(gammas);
}

void FormCalibration::on_pushAllEnergies_clicked()
{
  std::vector<double> gammas = ui->isotopes->current_gammas();

  std::vector<double> to_change;
  if (!selection_model_.selectedIndexes().empty()) {
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 0);
      double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      to_change.push_back(chan);
    }
  }


  if (gammas.size() != to_change.size())
    return;

  std::sort(gammas.begin(), gammas.end());

  for (int i=0; i<gammas.size(); i++) {
    my_markers_[to_change[i]] = gammas[i];
  }

  marker_table_.update();
  replot_markers();
}
