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
 *      FormPeaks -
 *
 ******************************************************************************/

#include "form_peaks.h"
#include "widget_detectors.h"
#include "ui_form_peaks.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormPeaks::FormPeaks(QWidget *parent) :
  QWidget(parent),
  spectrum_(nullptr),
  ui(new Ui::FormPeaks)
{
  ui->setupUi(this);

  qRegisterMetaType<Gaussian>("Gaussian");
  qRegisterMetaType<QVector<Gaussian>>("QVector<Gaussian>");

  ui->plot1D->use_calibrated(true);
  ui->plot1D->set_markers_selectable(true);

  range_.visible = false;
  range_.center.visible = true;
  range_.center.appearance.themes["light"] = QPen(Qt::darkMagenta, 3);
  range_.center.appearance.themes["dark"] = QPen(Qt::magenta, 3);
  range_.l.appearance.themes["light"] = QPen(Qt::darkMagenta, 10);
  range_.l.appearance.themes["dark"] = QPen(Qt::magenta, 10);
  range_.l.visible = true;
  range_.r = range_.l;
  range_.base.themes["light"] = QPen(Qt::darkMagenta, 2, Qt::DashLine);
  range_.base.themes["dark"] = QPen(Qt::magenta, 2, Qt::DashLine);
  range_.top.themes["light"] = QPen(Qt::darkMagenta, 0, Qt::DashLine);
  range_.top.themes["dark"] = QPen(Qt::magenta, 0, Qt::DashLine);


  list.appearance.themes["light"] = QPen(Qt::darkGray, 1);
  list.appearance.themes["dark"] = QPen(Qt::white, 1);
  list.selected_appearance.themes["light"] = QPen(Qt::black, 2);
  list.selected_appearance.themes["dark"] = QPen(Qt::yellow, 2);
  list.visible = true;

  main_graph_.default_pen = QPen(Qt::gray, 1);
  prelim_peak_.default_pen = QPen(Qt::black, 4);
  filtered_peak_.default_pen = QPen(Qt::blue, 6);
  gaussian_.default_pen = QPen(Qt::blue, 1);
  pseudo_voigt_.default_pen = QPen(Qt::cyan, 1);
  baseline_.default_pen = QPen(Qt::darkBlue, 1);
  rise_.default_pen = QPen(Qt::green, 1);
  fall_.default_pen = QPen(Qt::red, 1);
  even_.default_pen = QPen(Qt::black, 1);

  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(range_moved()), this, SLOT(range_moved()));

}

FormPeaks::~FormPeaks()
{
  delete ui;
}

void FormPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  ui->spinMinPeakWidth->setValue(settings_.value("min_peak_width", 5).toInt());
  ui->spinMovAvg->setValue(settings_.value("moving_avg_window", 15).toInt());
  ui->checkShowMovAvg->setChecked(settings_.value("show_moving_avg", false).toBool());
  ui->checkShowFilteredPeaks->setChecked(settings_.value("show_filtered_peaks", false).toBool());
  ui->checkShowPrelimPeaks->setChecked(settings_.value("show_prelim_peaks", false).toBool());
  ui->checkShowGaussians->setChecked(settings_.value("show_gaussians", false).toBool());
  ui->checkShowBaselines->setChecked(settings_.value("show_baselines", false).toBool());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plot1D->set_plot_style(settings_.value("plot_style", "Step").toString());
  ui->plot1D->set_marker_labels(settings_.value("marker_labels", true).toBool());
  settings_.endGroup();
}

void FormPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("min_peak_width", ui->spinMinPeakWidth->value());
  settings_.setValue("moving_avg_window", ui->spinMovAvg->value());
  settings_.setValue("show_moving_avg", ui->checkShowMovAvg->isChecked());
  settings_.setValue("show_prelim_peaks", ui->checkShowPrelimPeaks->isChecked());
  settings_.setValue("show_filtered_peaks", ui->checkShowFilteredPeaks->isChecked());
  settings_.setValue("show_gaussians", ui->checkShowGaussians->isChecked());
  settings_.setValue("show_baselines", ui->checkShowBaselines->isChecked());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.setValue("plot_style", ui->plot1D->plot_style());
  settings_.setValue("marker_labels", ui->plot1D->marker_labels());
  settings_.endGroup();
}

void FormPeaks::clear() {
  peaks_.clear();
  selected_peaks_.clear();
  calibration_ = Gamma::Calibration();
  detector_ = Gamma::Detector();
  spectrum_data_ = Gamma::Fitter();
  maxima.clear();
  minima.clear();
  toggle_push();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->clearExtras();
  ui->plot1D->reset_scales();
  replot_all();
}


void FormPeaks::setSpectrum(Pixie::Spectrum::Spectrum *newspectrum) {
  clear();
  spectrum_ = newspectrum;

  if (spectrum_ && spectrum_->resolution()) {
    int bits = spectrum_->bits();
    calibration_ = Gamma::Calibration("Energy", bits);
    for (std::size_t i=0; i < spectrum_->add_pattern().size(); i++) {
      if (spectrum_->add_pattern()[i] == 1) {
        detector_ = spectrum_->get_detectors()[i];
        if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits))) {
          calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
          PL_INFO << "<Peak finder> Energy calibration drawn from detector \"" << detector_.name_ << "\" with coefs = " << calibration_.coef_to_string();
        } else
          PL_INFO << "<Peak finder> No existing energy calibration for this resolution";
        QString title = "Spectrum=" + QString::fromStdString(spectrum_->name()) + "  resolution=" + QString::number(bits) + "bits  Detector=" + QString::fromStdString(detector_.name_);
        ui->plot1D->setFloatingText(title);
        ui->plot1D->setTitle(title);
      }
    }
  }

  ui->plot1D->reset_scales();

  update_spectrum();
}

void FormPeaks::update_spectrum() {

  if ((!spectrum_) || (!spectrum_->resolution())) {
    clear();
    replot_all();
    return;
  }

  minima.clear();
  maxima.clear();

  std::vector<double> x_chan(spectrum_->resolution());
  std::vector<double> y(spectrum_->resolution());

  std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_dump =
      std::move(spectrum_->get_spectrum({{0, y.size()}}));

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

  spectrum_data_ = Gamma::Fitter(x_chan, y, ui->spinMovAvg->value());
  spectrum_data_.find_prelim();
  spectrum_data_.filter_prelim(ui->spinMinPeakWidth->value());

  replot_all();
}


void FormPeaks::replot_all() {
  ui->plot1D->clearGraphs();
  ui->plot1D->addGraph(QVector<double>::fromStdVector(spectrum_data_.x_), QVector<double>::fromStdVector(spectrum_data_.y_), main_graph_);

  if (ui->checkShowMovAvg->isChecked())
    plot_derivs(spectrum_data_);

  QVector<double> xx, yy;

  if (ui->checkShowPrelimPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : spectrum_data_.prelim) {
      xx.push_back(q);
      yy.push_back(spectrum_data_.y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, prelim_peak_, QCPScatterStyle::ssDiamond);
  }

  if (ui->checkShowFilteredPeaks->isChecked()) {
    xx.clear(); yy.clear();
    for (auto &q : spectrum_data_.filtered) {
      xx.push_back(q);
      yy.push_back(spectrum_data_.y_[q]);
    }
    if (yy.size())
      ui->plot1D->addPoints(xx, yy, filtered_peak_, QCPScatterStyle::ssDiamond);
  }

  for (auto &q : peaks_) {
    if (ui->checkShowPseudoVoigt->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_fullfit_pseudovoigt_), pseudo_voigt_);
    if (ui->checkShowGaussians->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_fullfit_gaussian_), gaussian_);
    if (ui->checkShowBaselines->isChecked())
      ui->plot1D->addGraph(QVector<double>::fromStdVector(q.x_), QVector<double>::fromStdVector(q.y_baseline_), baseline_);
  }


  ui->plot1D->setLabels("channel", "counts");

  ui->plot1D->setYBounds(minima, maxima); //no baselines or avgs
  replot_markers();
}

void FormPeaks::addMovingMarker(double x) {
  range_.visible = true;

  range_.center.channel = x;
  range_.center.chan_valid = true;
  range_.center.energy = calibration_.transform(x);
  range_.center.energy_valid = (range_.center.channel != range_.center.energy);

  uint16_t ch = static_cast<uint16_t>(x);

  uint16_t ch_l = spectrum_data_.find_left(ch, ui->spinMinPeakWidth->value());
  range_.l.channel = ch_l;
  range_.l.chan_valid = true;
  range_.l.energy = calibration_.transform(ch_l);
  range_.l.energy_valid = (range_.l.channel != range_.l.energy);

  uint16_t ch_r = spectrum_data_.find_right(ch, ui->spinMinPeakWidth->value());
  range_.r.channel = ch_r;
  range_.r.chan_valid = true;
  range_.r.energy = calibration_.transform(ch_r);
  range_.r.energy_valid = (range_.r.channel != range_.r.energy);

  ui->pushAdd->setEnabled(true);
  PL_INFO << "<Peak finder> Marker at chan=" << range_.center.channel << " (" << range_.center.energy << " " << calibration_.units_ << ")"
          << ", edges=[" << range_.l.channel << ", " << range_.r.channel << "]";

  replot_markers();
}

void FormPeaks::removeMovingMarker(double x) {
  range_.visible = false;
  toggle_push();
  replot_markers();
}


void FormPeaks::replot_markers() {


  ui->plot1D->clearExtras();

  if (range_.visible)
    ui->plot1D->set_range(range_);

  std::list<Marker> markers;

  for (auto &q : peaks_) {
    Marker m = list;
    m.selected = false;
    m.channel = q.center;
    m.energy = q.energy;
    m.chan_valid = true;
    m.energy_valid = (m.channel != m.energy);

    if (selected_peaks_.count(m.channel) == 1)
      m.selected = true;
    markers.push_back(m);
  }


  ui->plot1D->set_markers(markers);
  ui->plot1D->replot_markers();
  ui->plot1D->redraw();
}

void FormPeaks::select_peaks(const std::set<double>& pks) {
  selected_peaks_ = pks;

  toggle_push();
  replot_markers();
}


void FormPeaks::on_pushAdd_clicked()
{
  if (range_.l.channel >= range_.r.channel)
    return;

  std::vector<double> baseline = spectrum_data_.make_baseline(range_.l.channel, range_.r.channel, 3);
  Gamma::Peak newpeak = Gamma::Peak(std::vector<double>(spectrum_data_.x_.begin() + range_.l.channel, spectrum_data_.x_.begin() + range_.r.channel + 1),
                      std::vector<double>(spectrum_data_.y_.begin() + range_.l.channel, spectrum_data_.y_.begin() + range_.r.channel + 1),
                      baseline, calibration_);

  if (newpeak.gaussian_.height_ > 0) {
    peaks_.push_back(newpeak);
    ui->pushAdd->setEnabled(false);
    removeMovingMarker(0);
    toggle_push();
    replot_all();
    emit peaks_changed();
  }
}

void FormPeaks::user_selected_peaks() {
  selected_peaks_ = ui->plot1D->get_selected_markers();
  emit peaks_selected();
}

std::vector<Gamma::Peak> FormPeaks::peaks() {
  return peaks_;
}

std::set<double> FormPeaks::selected_peaks() {
  return selected_peaks_;
}


void FormPeaks::toggle_push() {
  ui->pushAdd->setEnabled(range_.visible);
  ui->pushMarkerRemove->setEnabled(selected_peaks_.size() > 0);
}

void FormPeaks::on_pushMarkerRemove_clicked()
{
  for (auto &q : selected_peaks_) {
    for (int i=0; i < peaks_.size(); ++i) {
      if (peaks_[i].gaussian_.center_ == q) {
        peaks_.erase(peaks_.begin() + i);
        break;
      }
    }
  }

  selected_peaks_.clear();
  toggle_push();
  replot_all();
  emit peaks_changed();
}

void FormPeaks::on_pushFindPeaks_clicked()
{
  this->setCursor(Qt::WaitCursor);

  selected_peaks_.clear();
  spectrum_data_.find_peaks(ui->spinMinPeakWidth->value(), calibration_);
  peaks_ = spectrum_data_.peaks_;

  toggle_push();
  replot_all();

  emit peaks_changed();
  this->setCursor(Qt::ArrowCursor);
}

void FormPeaks::plot_derivs(Gamma::Fitter &data)
{
  QVector<double> temp_y, temp_x;
  int was = 0, is = 0;

  for (int i = 0; i < spectrum_data_.deriv1.size(); ++i) {
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
          ui->plot1D->addGraph(temp_x, temp_y, rise_);
        else if (was == -1)
          ui->plot1D->addGraph(temp_x, temp_y, fall_);
        else
          ui->plot1D->addGraph(temp_x, temp_y, even_);
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
      ui->plot1D->addGraph(temp_x, temp_y, rise_);
    else if (was == -1)
      ui->plot1D->addGraph(temp_x, temp_y, fall_);
    else
      ui->plot1D->addGraph(temp_x, temp_y, even_);
  }
}

void FormPeaks::on_checkShowMovAvg_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowPrelimPeaks_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowGaussians_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowBaselines_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowFilteredPeaks_clicked()
{
  replot_all();
}

void FormPeaks::on_checkShowPseudoVoigt_clicked()
{
  replot_all();
}

void FormPeaks::on_spinMovAvg_editingFinished()
{
  spectrum_data_.set_mov_avg(ui->spinMovAvg->value());
  spectrum_data_.find_prelim();
  spectrum_data_.filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormPeaks::on_spinMinPeakWidth_editingFinished()
{
  spectrum_data_.filter_prelim(ui->spinMinPeakWidth->value());
  replot_all();
}

void FormPeaks::range_moved() {
  range_ = ui->plot1D->get_range();
  toggle_push();
  replot_markers();
}
