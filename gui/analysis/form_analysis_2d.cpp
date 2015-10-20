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
 *      FormAnalysis2D -
 *
 ******************************************************************************/

#include "form_analysis_2d.h"
#include "ui_form_analysis_2d.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>


FormAnalysis2D::FormAnalysis2D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis2D),
  detectors_(newDetDB),
  settings_(settings),
  my_gain_calibration_(nullptr),
  my_gates_(nullptr),
  gate_x(nullptr),
  gate_y(nullptr),
  second_spectrum_type_(SecondSpectrumType::second_det)
{
  ui->setupUi(this);

  initialized = false;

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);
  ui->plotSpectrum2->setFit(&fit_data_2_);

  ui->plotMatrix->set_show_selector(false);

  my_gain_calibration_ = new FormGainCalibration(settings_, detectors_, fit_data_, fit_data_2_, this);
  my_gain_calibration_->hide();
  //my_gain_calibration_->blockSignals(true);
  connect(my_gain_calibration_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(my_gain_calibration_, SIGNAL(update_detector()), this, SLOT(apply_gain_calibration()));
  connect(my_gain_calibration_, SIGNAL(symmetrize_requested()), this, SLOT(symmetrize()));
  ui->tabs->addTab(my_gain_calibration_, "Gain calibration");

  my_gates_ = new FormMultiGates(settings_, this);
  ui->tabs->addTab(my_gates_, "Gates");
  //my_gates_->hide();
  //my_gates_->blockSignals(true);
  connect(my_gates_, SIGNAL(gate_selected(bool)), this, SLOT(remake_gate(bool)));
  connect(my_gates_, SIGNAL(boxes_made()), this, SLOT(take_boxes()));

  connect(ui->plotMatrix, SIGNAL(markers_set(Marker,Marker)), this, SLOT(update_gates(Marker,Marker)));
  connect(ui->plotMatrix, SIGNAL(stuff_selected()), this, SLOT(matrix_selection()));


  connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(ui->plotSpectrum2, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  connect(ui->plotSpectrum, SIGNAL(range_changed(Range)), this, SLOT(update_range(Range)));

  tempx = Qpx::Spectrum::Factory::getInstance().create_template("1D");
  tempx->visible = true;
  tempx->name_ = "tempx";
  tempx->match_pattern = std::vector<int16_t>({1,1});
  tempx->add_pattern = std::vector<int16_t>({1,0});

  tempy = Qpx::Spectrum::Factory::getInstance().create_template("1D");
  tempy->visible = true;
  tempy->name_ = "tempy";

  res = 0;
  xmin_ = 0;
  xmax_ = 0;
  ymin_ = 0;
  ymax_ = 0;
  xc_ = -1;
  yc_ = -1;

  live_seconds = 0;
  sum_inclusive = 0;
  sum_exclusive = 0;

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.default_pen = QPen(Qt::darkBlue, 7);

  ui->comboPlot2->blockSignals(true);
  ui->comboPlot2->addItem("2nd detector");
  ui->comboPlot2->addItem("diagonal");
  ui->comboPlot2->addItem("none");
  ui->comboPlot2->addItem("boxes");
  ui->comboPlot2->blockSignals(false);

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

}

void FormAnalysis2D::take_boxes() {
  coincidences_ = my_gates_->boxes();
  show_all_coincidences();
}

void FormAnalysis2D::show_all_coincidences() {
  ui->plotMatrix->set_boxes(coincidences_);
  ui->plotMatrix->replot_markers();
}


void FormAnalysis2D::update_range(Range range) {
  if (second_spectrum_type_ == SecondSpectrumType::none) {
    range2d.visible = range.visible;
    range2d.x1 = range.l;
    range2d.x2 = range.r;
    update_peaks(false);
  }
}


void FormAnalysis2D::matrix_selection() {
  PL_DBG << "User selected peaks in matrix";

  std::list<MarkerBox2D> chosen_peaks = ui->plotMatrix->get_selected_boxes();

  if (second_spectrum_type_ == SecondSpectrumType::boxes) {
   for (auto &q : chosen_peaks)
     for (auto &p : coincidences_)
       p.selected = (p == q);
   PL_DBG << "selected some boxes in matrix";
  } else if (second_spectrum_type_ == SecondSpectrumType::none) {
    std::set<double> xs, ys;
    for (auto &q : chosen_peaks) {
      xs.insert(q.x_c);
      ys.insert(q.y_c);
    }
    for (auto &q : fit_data_.peaks_)
      q.second.selected = (xs.count(q.second.center) > 0);
    update_peaks(false);
  }
}


void FormAnalysis2D::remake_gate(bool force) {
  if (!my_gates_)
    return;

  Gamma::Gate gate = my_gates_->current_gate();
  PL_DBG << "remake gate c=" << gate.centroid_chan << " w=" << gate.width_chan;

  if (gate.width_chan == 0)
    return;

  fit_data_ = gate.fit_data_;

  yc_ = xc_ = gate.centroid_chan;
  double xmin = 0;
  double xmax = res - 1;
  double ymin = 0;
  double ymax = res - 1;

  Marker xx, yy;
  double width = gate.width_chan;

  if (gate.centroid_chan == -1) {
    width = res;
    gate.width_chan = res;
    xx.channel = res / 2;
    xx.energy = gate.fit_data_.nrg_cali_.transform(res/2);
    xx.chan_valid = true;
    xx.energy_valid = true;

    yy.channel = res / 2;
    yy.energy = gate.fit_data_.nrg_cali_.transform(res/2);
    yy.chan_valid = true;
    yy.energy_valid = true;

    xmin = 0;
    xmax = res - 1;
    ymin = 0;
    ymax = res - 1;

  } else if (second_spectrum_type_ == SecondSpectrumType::none) {
    xmin = 0;
    xmax = xmax = res - 1;
    ymin = yc_ - (width / 2); if (ymin < 0) ymin = 0;
    ymax = yc_ + (width / 2); if (ymax >= res) ymax = res - 1;

    xx.channel = res / 2;
    xx.energy = gate.fit_data_.nrg_cali_.transform(res / 2);
    xx.chan_valid = true;
    xx.energy_valid = true;

    yy.channel = yc_;
    yy.energy = gate.centroid_nrg;
    yy.chan_valid = true;
    yy.energy_valid = true;

  } else {

    xmin = xc_ - width; if (xmin < 0) xmin = 0;
    xmax = xc_ + width; if (xmax >= res) xmax = res - 1;
    ymin = yc_ - width; if (ymin < 0) ymin = 0;
    ymax = yc_ + width; if (ymax >= res) ymax = res - 1;

    xx.channel = xc_;
    xx.energy = gate.fit_data_.nrg_cali_.transform(res);
    xx.chan_valid = true;
    xx.energy_valid = true;

    yy.channel = yc_;
    yy.energy = gate.centroid_nrg;
    yy.chan_valid = true;
    yy.energy_valid = true;

  }
  xx.visible = false;
  yy.visible = true;

  //ui->plotMatrix->set_boxes(boxes);

  if ((xmin != xmin_) || (xmax != xmax_) || (ymin != ymin_) || (ymax != ymax_)
      || (static_cast<double>(ui->plotMatrix->gate_width()) != width) || force) {
    xmin_ = xmin; xmax_ = xmax;
    ymin_ = ymin; ymax_ = ymax;
    ui->plotMatrix->set_gate_width(static_cast<uint16_t>(gate.width_chan));
    ui->plotMatrix->set_markers(xx, yy);
    if (fit_data_.peaks_.empty())
      make_gated_spectra();
    else
      update_peaks(true);
  }
}

void FormAnalysis2D::on_comboPlot2_currentIndexChanged(const QString &arg1)
{
  if (arg1 == "diagonal")
    second_spectrum_type_ = SecondSpectrumType::diagonal;
  else if (arg1 == "none")
    second_spectrum_type_ = SecondSpectrumType::none;
  else if (arg1 == "boxes")
    second_spectrum_type_ = SecondSpectrumType::boxes;
  else
    second_spectrum_type_ = SecondSpectrumType::second_det;

  configure_UI();
}

void FormAnalysis2D::configure_UI() {
  //while (ui->tabs->count())
//    ui->tabs->removeTab(0);
  bool visible1 = (second_spectrum_type_ != SecondSpectrumType::boxes);
  bool visible2 = ((second_spectrum_type_ == SecondSpectrumType::diagonal) ||
                   (second_spectrum_type_ == SecondSpectrumType::second_det) ||
                   (second_spectrum_type_ == SecondSpectrumType::gain_match));

  bool see_boxes = ((second_spectrum_type_ == SecondSpectrumType::none) ||
                    (second_spectrum_type_ == SecondSpectrumType::boxes));

  ui->plotSpectrum->setVisible(visible1);
  ui->plotSpectrum->blockSignals(!visible1);

  ui->plotSpectrum2->setVisible(visible2);
  ui->plotSpectrum2->blockSignals(!visible2);
  ui->plotMatrix->set_gates_movable(visible2);

  ui->plotMatrix->set_gates_visible((second_spectrum_type_ == SecondSpectrumType::second_det) ||
                                    (second_spectrum_type_ == SecondSpectrumType::gain_match),
                                    visible1,
                                    second_spectrum_type_ == SecondSpectrumType::diagonal);

  ui->plotMatrix->set_show_boxes(see_boxes);

  if (!visible1)
    ui->plotMatrix->set_markers(Marker(), Marker());

  if (visible1)
    make_gated_spectra();
  else
    show_all_coincidences();

  //if (second_spectrum_type_ == SecondSpectrumType::none) {
//    PL_DBG << "adding gates tab";
 //   ui->tabs->addTab(my_gates_, "Gates");
  //  my_gates_->blockSignals(false);
  //  my_gates_->show();
 // } else   {
 //   my_gates_->hide();
 //   //my_gates_->blockSignals(true);
 // }


//  if (second_spectrum_type_ == SecondSpectrumType::gain_match)
//   {
//    ui->tabs->addTab(my_gain_calibration_, "Gain calibration");
//    my_gain_calibration_->show();
//    my_gain_calibration_->blockSignals(false);
//  } else {
//    my_gain_calibration_->hide();
//    my_gain_calibration_->blockSignals(true);
//  }

}



void FormAnalysis2D::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
}

void FormAnalysis2D::reset() {
  initialized = false;
//  while (ui->tabs->count())
//    ui->tabs->removeTab(0);
}

void FormAnalysis2D::make_gated_spectra() {
  Qpx::Spectrum::Spectrum* source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
  if (source_spectrum == nullptr)
    return;

  this->setCursor(Qt::WaitCursor);

  Qpx::Spectrum::Metadata md = source_spectrum->metadata();

  //  PL_DBG << "Coincidence gate x[" << xmin_ << "-" << xmax_ << "]   y[" << ymin_ << "-" << ymax_ << "]";

  if ((md.total_count > 0) && (md.dimensions == 2))
  {
    sum_inclusive = 0;
    sum_exclusive = 0;

    live_seconds = source_spectrum->metadata().live_time.total_seconds();
    uint32_t adjrange = static_cast<uint32_t>(md.resolution) - 1;

    tempx->bits = md.bits;
    tempx->name_ = detector1_.name_ + "[" + to_str_precision(nrg_calibration2_.transform(ymin_), 0) + "," + to_str_precision(nrg_calibration2_.transform(ymax_), 0) + "]";

    if (gate_x != nullptr)
      delete gate_x;
    gate_x = Qpx::Spectrum::Factory::getInstance().create_from_template(*tempx);

    //if?
    Qpx::Spectrum::slice_rectangular(source_spectrum, gate_x, {{0, adjrange}, {ymin_, ymax_}}, spectra_->runInfo());

    fit_data_.clear();
    fit_data_.setData(gate_x);
    ui->plotSpectrum->update_spectrum();


    if (second_spectrum_type_ == SecondSpectrumType::diagonal) {
      tempy->bits = md.bits;
      tempy->name_ = "diag_slice_" + detector1_.name_;
      tempy->match_pattern = std::vector<int16_t>({1,0});
      tempy->add_pattern = std::vector<int16_t>({1,0});
    } else if ((second_spectrum_type_ == SecondSpectrumType::second_det) ||
                (second_spectrum_type_ == SecondSpectrumType::gain_match)) {
      tempy->bits = md.bits;
      tempy->name_ = detector2_.name_ + "[" + to_str_precision(nrg_calibration1_.transform(xmin_), 0) + "," + to_str_precision(nrg_calibration1_.transform(xmax_), 0) + "]";
      tempy->match_pattern = std::vector<int16_t>({1,1});
      tempy->add_pattern = std::vector<int16_t>({0,1});
    }

    if (second_spectrum_type_ != SecondSpectrumType::none) {

      if (gate_y != nullptr)
        delete gate_y;
      gate_y = Qpx::Spectrum::Factory::getInstance().create_from_template(*tempy);

      if (second_spectrum_type_ == SecondSpectrumType::diagonal)
        Qpx::Spectrum::slice_diagonal(source_spectrum, gate_y, xc_, yc_, ui->plotMatrix->gate_width(), spectra_->runInfo());
      else if ((second_spectrum_type_ == SecondSpectrumType::second_det) ||
               (second_spectrum_type_ == SecondSpectrumType::gain_match))
        Qpx::Spectrum::slice_rectangular(source_spectrum, gate_y, {{xmin_, xmax_}, {0, adjrange}}, spectra_->runInfo());

      fit_data_2_.clear();
      fit_data_2_.setData(gate_y);
      ui->plotSpectrum2->update_spectrum();
    }

    sum_inclusive += sum_exclusive;
    ui->labelExclusiveArea->setText(QString::number(sum_exclusive));
    ui->labelExclusiveCps->setText(QString::number(sum_exclusive / live_seconds));

//    ui->plotMatrix->refresh();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::initialize() {
  if (initialized)
    return;

  if (spectra_) {

    PL_DBG << "<Analysis2D> initializing to " << current_spectrum_.toStdString();
    Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if (spectrum && spectrum->resolution()) {
      Qpx::Spectrum::Metadata md = spectrum->metadata();
      res = md.resolution;

      xc_ = -1;
      yc_ = -1;

      xmin_ = 0;
      xmax_ = res - 1;
      ymin_ = 0;
      ymax_ = res - 1;

      detector1_ = Gamma::Detector();
      detector2_ = Gamma::Detector();

      if (md.detectors.size() > 1) {
        detector1_ = md.detectors[0];
        detector2_ = md.detectors[1];
      } else if (md.detectors.size() == 1) {
        detector2_ = detector1_ = md.detectors[0];
        //HACK!!!
      }

      PL_DBG << "det1 " << detector1_.name_;
      PL_DBG << "det2 " << detector2_.name_;

      if (detector1_.energy_calibrations_.has_a(Gamma::Calibration("Energy", md.bits)))
        nrg_calibration1_ = detector1_.energy_calibrations_.get(Gamma::Calibration("Energy", md.bits));

      if (detector2_.energy_calibrations_.has_a(Gamma::Calibration("Energy", md.bits)))
        nrg_calibration2_ = detector2_.energy_calibrations_.get(Gamma::Calibration("Energy", md.bits));


      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_, current_spectrum_);
      ui->plotMatrix->update_plot(true);

      //need better criterion
      bool symmetrized = ((detector1_ != Gamma::Detector()) && (detector2_ != Gamma::Detector()) && (detector1_.name_ == detector2_.name_));

      if (!symmetrized) {
        second_spectrum_type_ = SecondSpectrumType::gain_match;
        ui->comboPlot2->setEnabled(false);
      } else
        ui->comboPlot2->setEnabled(true);
    }
  }

  initialized = true;
  configure_UI();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());
    if (spectrum && spectrum->resolution())
      live_seconds = spectrum->metadata().live_time.total_seconds();

    ui->plotMatrix->update_plot(true);
    //ui->plotSpectrum->update_spectrum();
    //ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks(bool content_changed) {
  //update sums
  if (second_spectrum_type_ != SecondSpectrumType::none) {
    if (content_changed) {

      for (auto &q : fit_data_.peaks_) {
        if ((q.first > xmin_) && (q.first < xmax_))
          q.second.flagged = true;
        else
          q.second.flagged = false;
      }

      for (auto &q : fit_data_2_.peaks_) {
        if ((q.first > ymin_) && (q.first < ymax_))
          q.second.flagged = true;
        else
          q.second.flagged = false;
      }

    }
    ui->plotSpectrum2->update_fit(content_changed);
  }

  ui->plotSpectrum->update_fit(content_changed);

  if (my_gain_calibration_)
    my_gain_calibration_->newSpectrum();

  if (my_gates_) {

    //if (yc_ >= 0) {
    std::list<MarkerBox2D> boxes;
    double width = ui->plotMatrix->gate_width() / 2;

    for (auto &q : fit_data_.peaks_) {
      Marker xx, yy;

      xx.channel = q.second.center;
      xx.energy = q.second.energy;
      xx.chan_valid = true;
      xx.energy_valid = false;

      yy.channel = yc_;
      if (yc_ < 0)
        yy.channel = res / 2;
      yy.energy = fit_data_.nrg_cali_.transform(yc_);
      yy.chan_valid = true;
      yy.energy_valid = false;

      MarkerBox2D box;
      box.visible = true;
      box.selected = q.second.selected;
      box.x_c = xx.channel;
      box.y_c = yy.channel;
      box.x1 = xx;
      box.x2 = xx;
      box.y1 = yy;
      box.y2 = yy;
      box.x1.channel -= (q.second.gaussian_.hwhm_ * my_gates_->width_factor());
      box.x2.channel += (q.second.gaussian_.hwhm_ * my_gates_->width_factor());
      box.y1.channel -= width;
      box.y2.channel += width;

      boxes.push_back(box);
      range2d.y1 = box.y1;
      range2d.y2 = box.y2;
    }
    ui->plotMatrix->set_boxes(boxes);
    ui->plotMatrix->set_range_x(range2d);
    ui->plotMatrix->replot_markers();
    //}

    Gamma::Gate gate;
    gate.fit_data_ = fit_data_;
    gate.centroid_chan = yc_;
    gate.centroid_nrg = fit_data_.nrg_cali_.transform(yc_);
    if (yc_ >= 0) {
      gate.width_chan = ui->plotMatrix->gate_width();
      gate.width_nrg = fit_data_.nrg_cali_.transform(ymax_) - fit_data_.nrg_cali_.transform(ymin_);
    } else {
      gate.width_chan = res;
      gate.width_nrg = fit_data_.nrg_cali_.transform(res) - fit_data_.nrg_cali_.transform(0);
    }
    my_gates_->update_current_gate(gate);
  }
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  if (second_spectrum_type_ == SecondSpectrumType::none) {
    ui->plotSpectrum->make_range(xx);
    return;
  } else if (second_spectrum_type_ == SecondSpectrumType::boxes) {
    PL_DBG << "make 2D range, unimplemented";
    return;
  }

  double xmin = 0;
  double xmax = res - 1;
  double ymin = 0;
  double ymax = res - 1;
  xc_ = -1;
  yc_ = -1;

  if (xx.visible || yy.visible) {
    xc_ = xx.channel;
    yc_ = yy.channel;
    double width = ui->plotMatrix->gate_width() / 2;
    xmin = xc_ - width; if (xmin < 0) xmin = 0;
    xmax = xc_ + width; if (xmax >= res) xmax = res - 1;
    ymin = yc_ - width; if (ymin < 0) ymin = 0;
    ymax = yc_ + width; if (ymax >= res) ymax = res - 1;
  }

  if ((xmin != xmin_) || (xmax != xmax_) || (ymin != ymin_) || (ymax != ymax_)) {
    xmin_ = xmin; xmax_ = xmax;
    ymin_ = ymin; ymax_ = ymax;
    make_gated_spectra();
  }
}

void FormAnalysis2D::on_pushAddGatedSpectra_clicked()
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

  if (second_spectrum_type_ != SecondSpectrumType::none) {
    if ((gate_y != nullptr) && (gate_y->metadata().total_count > 0)) {
      if (spectra_->by_name(gate_y->name()) != nullptr)
        PL_WARN << "Spectrum " << gate_y->name() << " already exists.";
      else
      {
        gate_y->set_appearance(generateColor().rgba());
        spectra_->add_spectrum(gate_y);
        gate_y = nullptr;
        success = true;
      }
    }
  }

  if (success) {
    emit spectraChanged();
  }
  make_gated_spectra();
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::symmetrize()
{
  this->setCursor(Qt::WaitCursor);

  QString fold_spec_name = current_spectrum_ + "_sym";
  bool ok;
  QString text = QInputDialog::getText(this, "New spectrum name",
                                       "Spectrum name:", QLineEdit::Normal,
                                       fold_spec_name,
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (spectra_->by_name(fold_spec_name.toStdString()) != nullptr) {
      PL_WARN << "Spectrum " << fold_spec_name.toStdString() << " already exists. Aborting symmetrization";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    Qpx::Spectrum::Spectrum* source_spectrum = spectra_->by_name(current_spectrum_.toStdString());
    if (source_spectrum == nullptr) {
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    //gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.metadata_.bits, fit_data_.detector_.name_);
    //compare with source and replace?

    Qpx::Spectrum::Metadata md = source_spectrum->metadata();

    std::vector<uint16_t> chans;
    for (int i=0; i < md.add_pattern.size(); ++i) {
      if (md.add_pattern[i] == 1)
        chans.push_back(i);
    }

    if (chans.size() != 2) {
      PL_WARN << "<Analysis2D> " << md.name << " does not have 2 channels in pattern";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    std::vector<int16_t> pattern(chans[1] + 1, 0);
    pattern[chans[0]] = 1;
    pattern[chans[1]] = 1;

    Qpx::Spectrum::Template *temp_sym = Qpx::Spectrum::Factory::getInstance().create_template(md.type); //assume 2D?
    temp_sym->visible = false;
    temp_sym->name_ = fold_spec_name.toStdString();
    temp_sym->match_pattern = pattern;
    temp_sym->add_pattern = pattern;
    temp_sym->bits = md.bits;

    Qpx::Spectrum::Spectrum* destination = Qpx::Spectrum::Factory::getInstance().create_from_template(*temp_sym);
    delete temp_sym;

    if (destination == nullptr) {
      PL_WARN << "<Analysis2D> " << fold_spec_name.toStdString() << " could not be created";
      this->setCursor(Qt::ArrowCursor);
      return;
    }

    if (Qpx::Spectrum::symmetrize(source_spectrum, destination, spectra_->runInfo())) {
      spectra_->add_spectrum(destination);
      setSpectrum(spectra_, fold_spec_name);
      reset();
      initialize();
      emit spectraChanged();
    } else {
      PL_WARN << "<Analysis2D> could not symmetrize " << md.name;
      delete destination;
      this->setCursor(Qt::ArrowCursor);
      return;
    }

  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::apply_gain_calibration()
{
  gain_match_cali_ = fit_data_2_.detector_.get_gain_match(fit_data_2_.metadata_.bits, detector1_.name_);

  std::string msg_text("Propagating gain match calibration ");
  msg_text += detector2_.name_ + "->" + gain_match_cali_.to_ + " (" + std::to_string(gain_match_cali_.bits_) + " bits) to all spectra in current project: "
      + spectra_->status();

  std::string question_text("Do you also want to save this calibration to ");
  question_text += detector2_.name_ + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Gamma::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(detector2_)) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(detector2_.name_),
                                           &ok);
      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = detector2_;
        modified.name_ = text.toStdString();
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Gamma::Detector();
        }
      }
    } else
      modified = detectors_.get(detector2_);

    if (modified != Gamma::Detector())
    {
      PL_INFO << "   applying new gain_match calibrations for " << modified.name_ << " in detector database";
      modified.gain_match_calibrations_.replace(gain_match_cali_);
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }

  if (ret != QMessageBox::Abort) {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      Qpx::Spectrum::Metadata md = q->metadata();
      for (auto &p : md.detectors) {
        if (p.shallow_equals(detector2_)) {
          PL_INFO << "   applying new calibrations for " << detector2_.name_ << " on " << q->name();
          p.gain_match_calibrations_.replace(gain_match_cali_);
        }
      }
      q->set_detectors(md.detectors);
    }

    std::vector<Gamma::Detector> detectors = spectra_->runInfo().detectors;
    for (auto &p : detectors) {
      if (p.shallow_equals(detector2_)) {
        PL_INFO << "   applying new calibrations for " << detector2_.name_ << " in current project " << spectra_->status();
        p.gain_match_calibrations_.replace(gain_match_cali_);
      }
    }
    Qpx::RunInfo ri = spectra_->runInfo();
    ri.detectors = detectors;
    spectra_->setRunInfo(ri);
  }
}

void FormAnalysis2D::closeEvent(QCloseEvent *event) {
  ui->plotSpectrum->saveSettings(settings_);
  ui->plotSpectrum2->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormAnalysis2D::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
  ui->plotSpectrum2->loadSettings(settings_);

  settings_.beginGroup("AnalysisMatrix");
  ui->plotMatrix->set_zoom(settings_.value("zoom", 50).toDouble());
  ui->plotMatrix->set_gradient(settings_.value("gradient", "hot").toString());
  ui->plotMatrix->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plotMatrix->set_show_legend(settings_.value("show_legend", false).toBool());
  ui->plotMatrix->set_gate_width(settings_.value("gate_width", 10).toInt());
  settings_.endGroup();

}

void FormAnalysis2D::saveSettings() {
  ui->plotSpectrum->saveSettings(settings_);

  if (my_gates_)
    my_gates_->saveSettings();

  settings_.beginGroup("AnalysisMatrix");
  settings_.setValue("zoom", ui->plotMatrix->zoom());
  settings_.setValue("gradient", ui->plotMatrix->gradient());
  settings_.setValue("scale_type", ui->plotMatrix->scale_type());
  settings_.setValue("show_legend", ui->plotMatrix->show_legend());
  settings_.setValue("gate_width", ui->plotMatrix->gate_width());
  settings_.endGroup();
}

void FormAnalysis2D::clear() {
  res = 0;
  xmin_ = ymin_ = 0;
  xmax_ = ymax_ = 0;

  xc_ = -1;
  yc_ = -1;


  live_seconds = 0;
  sum_inclusive = 0;
  sum_exclusive = 0;

  ui->plotMatrix->reset_content();

  current_spectrum_.clear();
  detector1_ = Gamma::Detector();
  nrg_calibration1_ = Gamma::Calibration();

  detector2_ = Gamma::Detector();
  nrg_calibration2_ = Gamma::Calibration();

  if (my_gain_calibration_->isVisible())
    my_gain_calibration_->clear();
  if (my_gates_->isVisible())
    my_gates_->clear();
}

FormAnalysis2D::~FormAnalysis2D()
{
  if (tempx != nullptr)
    delete tempx;
  if (tempy != nullptr)
    delete tempy;

  if (gate_x != nullptr)
    delete gate_x;
  if (gate_y != nullptr)
    delete gate_y;

  delete ui;
}
