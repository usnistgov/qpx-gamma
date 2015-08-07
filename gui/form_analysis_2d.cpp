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
#include "widget_detectors.h"
#include "ui_form_analysis_2d.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include <QInputDialog>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>

FormAnalysis2D::FormAnalysis2D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis2D),
  detectors_(newDetDB),
  settings_(settings),
  gate_x(nullptr),
  gate_y(nullptr)
{
  ui->setupUi(this);

  initialized = false;

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);
  ui->plotSpectrum2->setFit(&fit_data_2_);

  ui->plotMatrix->set_show_analyse(false);
  ui->plotMatrix->set_show_selector(false);
  connect(ui->plotMatrix, SIGNAL(markers_set(Marker,Marker)), this, SLOT(update_gates(Marker,Marker)));

  connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(ui->plotSpectrum2, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  tempx = Pixie::Spectrum::Factory::getInstance().create_template("1D");
  tempx->visible = true;
  tempx->name_ = "tempx";
  tempx->match_pattern = std::vector<int16_t>({1,1,0,0});
  tempx->add_pattern = std::vector<int16_t>({1,0,0,0});

  tempy = Pixie::Spectrum::Factory::getInstance().create_template("1D");
  tempy->visible = true;
  tempy->name_ = "tempy";
  tempy->match_pattern = std::vector<int16_t>({1,1,0,0});
  tempy->add_pattern = std::vector<int16_t>({0,1,0,0});

  res = 0;
  xmin_ = 0;
  xmax_ = 0;
  ymin_ = 0;
  ymax_ = 0;

  live_seconds = 0;
  sum_inclusive = 0;
  sum_exclusive = 0;
  sum_no_peaks = 0;
  sum_prism = 0;

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.default_pen = QPen(Qt::darkBlue, 7);

  gatex_in_spectra = false;
  gatey_in_spectra = false;

  ui->tableCoincResults->verticalHeader()->hide();
  ui->tableCoincResults->setColumnCount(3);
  ui->tableCoincResults->setRowCount(4);
  ui->tableCoincResults->setItem(0, 0, new QTableWidgetItem( "gates inclusive" ));
  ui->tableCoincResults->setItem(1, 0, new QTableWidgetItem( "gates exclusive" ));
  ui->tableCoincResults->setItem(2, 0, new QTableWidgetItem( "extended baselines no peaks" ));
  ui->tableCoincResults->setItem(3, 0, new QTableWidgetItem( "prism under exclusive peak" ));

  ui->tableCoincResults->setHorizontalHeaderLabels({"geometry", "area", "cps"});
  ui->tableCoincResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableCoincResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableCoincResults->show();

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

}

FormAnalysis2D::~FormAnalysis2D()
{
  if (tempx != nullptr)
    delete tempx;
  if (tempy != nullptr)
    delete tempy;

  if ((!gatex_in_spectra) && (gate_x != nullptr))
    delete gate_x;
  if ((!gatey_in_spectra) && (gate_y != nullptr))
    delete gate_y;


  delete ui;
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

  live_seconds = 0;
  sum_inclusive = 0;
  sum_exclusive = 0;
  sum_no_peaks = 0;
  sum_prism = 0;

  ui->plotSpectrum->setSpectrum(nullptr);
  ui->plotSpectrum2->setSpectrum(nullptr);
  ui->plotMatrix->reset_content();

  current_spectrum_.clear();
  detector1_ = Gamma::Detector();
  nrg_calibration1_ = Gamma::Calibration();
  fwhm_calibration1_ = Gamma::Calibration();

  detector2_ = Gamma::Detector();
  nrg_calibration2_ = Gamma::Calibration();
  fwhm_calibration2_ = Gamma::Calibration();

  ui->plotCalib->clearGraphs();
  ui->plotCalib->setLabels("", "");
  ui->plotCalib->setFloatingText("");

  gain_match_cali_ = Gamma::Calibration("Gain", 0);
}


void FormAnalysis2D::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  PL_DBG << "<FormAnalysis2D> setSpectrum";

  clear();
  spectra_ = newset;
  current_spectrum_ = name;
}

void FormAnalysis2D::reset() {
  initialized = false;
}

void FormAnalysis2D::initialize() {
  if (initialized)
    return;

  if (spectra_) {

    Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if (spectrum && spectrum->resolution()) {
      int bits = spectrum->bits();
      res = spectrum->resolution();

      xmin_ = 0;
      xmax_ = res - 1;
      ymin_ = 0;
      ymax_ = res - 1;

      detector1_ = spectrum->get_detector(0);
      if (detector1_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
        nrg_calibration1_ = detector1_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      fwhm_calibration1_ = detector1_.fwhm_calibration_;
      PL_DBG << "<FormAnalysis2D> NRGCALIB1 " << nrg_calibration1_.to_string();
      PL_DBG << "<FormAnalysis2D> FWHMCALIB1 " << fwhm_calibration1_.to_string();

      detector2_ = spectrum->get_detector(1);
      if (detector2_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
        nrg_calibration2_ = detector2_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      fwhm_calibration2_ = detector2_.fwhm_calibration_;
      PL_DBG << "<FormAnalysis2D> NRGCALIB2 " << nrg_calibration2_.to_string();
      PL_DBG << "<FormAnalysis2D> FWHMCALIB2 " << fwhm_calibration2_.to_string();

      gain_match_cali_ = detector2_.get_gain_match(bits, detector1_.name_);
      PL_DBG << "<FormAnalysis2D> gain match cali from " << detector2_.name_ << " to " << detector1_.name_ << " " << gain_match_cali_.to_string();

      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_);
      ui->plotMatrix->set_spectrum(current_spectrum_);
      ui->plotMatrix->update_plot();

      ui->plotSpectrum->setData(nrg_calibration1_, fwhm_calibration1_);
      ui->plotSpectrum2->setData(nrg_calibration2_, fwhm_calibration2_);

      ui->plotCalib->setLabels("channel (" + QString::fromStdString(detector2_.name_) + ")", "channel (" + QString::fromStdString(detector1_.name_) + ")");
    }
  }

  update_spectrum();

  initialized = true;
  make_gated_spectra();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());
    if (spectrum && spectrum->resolution())
      live_seconds = spectrum->live_time().total_seconds();

    //ui->plotMatrix->update_plot(true);
    ui->plotSpectrum->update_spectrum();
    ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks(bool content_changed) {
  //update sums
  plot_calib();
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  int xmin = 0;
  int xmax = res - 1;
  int ymin = 0;
  int ymax = res - 1;


  if (xx.visible || yy.visible) {
    int width = ui->plotMatrix->gate_width() / 2;
    xmin = xx.channel - width; if (xmin < 0) xmin = 0;
    xmax = xx.channel + width; if (xmax >= res) xmax = res - 1;
    ymin = yy.channel - width; if (ymin < 0) ymin = 0;
    ymax = yy.channel + width; if (ymax >= res) ymax = res - 1;
  }

  if ((xmin != xmin_) || (xmax != xmax_) || (ymin != ymin_) || (ymax != ymax_)) {
    xmin_ = xmin; xmax_ = xmax;
    ymin_ = ymin; ymax_ = ymax;
    make_gated_spectra();
  }
}

void FormAnalysis2D::make_gated_spectra() {
  this->setCursor(Qt::WaitCursor);

  Pixie::Spectrum::Spectrum* some_spectrum = spectra_->by_name(current_spectrum_.toStdString());

//  PL_DBG << "Coincidence gate x[" << xmin_ << "-" << xmax_ << "]   y[" << ymin_ << "-" << ymax_ << "]";

  if ((some_spectrum != nullptr)
      && some_spectrum->total_count()
      && (some_spectrum->dimensions() == 2)
      )
  {
      int bits = some_spectrum->bits();
      uint32_t adjrange = static_cast<uint32_t>(some_spectrum->resolution()) - 1;

      tempx->bits = bits;
      tempx->name_ = detector1_.name_ + "[" + to_str_precision(nrg_calibration2_.transform(ymin_), 0) + "," + to_str_precision(nrg_calibration2_.transform(ymax_), 0) + "]";

      tempy->bits = bits;
      tempy->name_ = detector2_.name_ + "[" + to_str_precision(nrg_calibration1_.transform(xmin_), 0) + "," + to_str_precision(nrg_calibration1_.transform(xmax_), 0) + "]";

      if ((!gatex_in_spectra) && (gate_x != nullptr))
        delete gate_x;
      gate_x = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempx);
      gatex_in_spectra = false;

      if ((!gatey_in_spectra) && (gate_y != nullptr))
        delete gate_y;
      gate_y = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempy);
      gatey_in_spectra = false;

      sum_inclusive = 0;
      sum_exclusive = 0;

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{xmin_, xmax_}, {0, adjrange}}));
      for (auto it : *spectrum_data) {
        if ((it.first[0] >= xmin_) && (it.first[0] <= xmax_)) {
          gate_y->add_bulk(it);
          if ((it.first[1] >= ymin_) && (it.first[1] <= ymax_))
            sum_exclusive += it.second;
          else
            sum_inclusive += it.second;
        }
      }

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data2 =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {ymin_, ymax_}}));
      for (auto it : *spectrum_data2) {
        if ((it.first[1] >= ymin_) && (it.first[1] <= ymax_)) {
          gate_x->add_bulk(it);
          if ((it.first[0] < xmin_) || (it.first[0] > xmax_))
            sum_inclusive += it.second;
        }
      }

      sum_inclusive += sum_exclusive;
      fill_table();


      std::vector<Gamma::Detector> detectors = some_spectrum->get_detectors();
      Pixie::Spill spill;
      spill.run = new Pixie::RunInfo(spectra_->runInfo());
      for (int i=0; i < detectors.size(); ++i)
        spill.run->p4_state.set_detector(Pixie::Channel(i), detectors[i]);
      gate_x->addSpill(spill);
      gate_y->addSpill(spill);
      delete spill.run;

      ui->plotSpectrum->setData(nrg_calibration1_, fwhm_calibration1_);
      ui->plotSpectrum->setSpectrum(gate_x, xmin_, xmax_);

      ui->plotSpectrum2->setData(nrg_calibration2_, fwhm_calibration2_);
      ui->plotSpectrum2->setSpectrum(gate_y, ymin_, ymax_);

      ui->plotMatrix->refresh();

      plot_calib();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::fill_table()
{
  ui->tableCoincResults->setItem(0, 1, new QTableWidgetItem( QString::number(sum_inclusive) ));
  ui->tableCoincResults->setItem(0, 2, new QTableWidgetItem( QString::number(sum_inclusive / live_seconds) ));

  ui->tableCoincResults->setItem(1, 1, new QTableWidgetItem( QString::number(sum_exclusive) ));
  ui->tableCoincResults->setItem(1, 2, new QTableWidgetItem( QString::number(sum_exclusive / live_seconds) ));
}

void FormAnalysis2D::plot_calib()
{
  ui->plotCalib->clearGraphs();

  int total = fit_data_.peaks_.size();
  if (total != fit_data_2_.peaks_.size())
  {
    //make invisible?
    return;
  }

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_2_.peaks_) {
    xx.push_back(q.first);
    if (q.first < xmin)
      xmin = q.first;
    if (q.first > xmax)
      xmax = q.first;
  }

  for (auto &q : fit_data_.peaks_)
    yy.push_back(q.first);

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size()) {
    ui->plotCalib->addPoints(xx, yy, style_pts);
    if ((gain_match_cali_.to_ == detector1_.name_) && (gain_match_cali_.coefficients_ != std::vector<double>({0, 1}))) {

      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i < xmax; i+=step) {
        xx.push_back(i);
        yy.push_back(gain_match_cali_.transform(i));
      }
      ui->plotCalib->addFit(xx, yy, style_fit);
    }
  }
}

void FormAnalysis2D::on_pushCalibGain_clicked()
{
  int total = fit_data_.peaks_.size();
  if (total != fit_data_2_.peaks_.size())
  {
    //make invisible?
    return;
  }

  std::vector<double> x, y;

  for (auto &q : fit_data_2_.peaks_)
    x.push_back(q.first);
  for (auto &q : fit_data_.peaks_)
    y.push_back(q.first);

  Polynomial p = Polynomial(x, y, ui->spinPolyOrder->value());

  if (p.coeffs_.size()) {
    gain_match_cali_.coefficients_ = p.coeffs_;
    gain_match_cali_.to_ = detector1_.name_;
    gain_match_cali_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    gain_match_cali_.model_ = Gamma::CalibrationModel::polynomial;
    ui->plotCalib->setFloatingText(QString::fromStdString(p.to_UTF8(3, true)));
//    ui->pushApplyCalib->setEnabled(gain_match_calib_ != old_calibration_);
  }
  else
    PL_INFO << "<Energy calibration> Gamma::Calibration failed";

//  toggle_push();
  plot_calib();
}

void FormAnalysis2D::on_pushCull_clicked()
{
  //while (cull_mismatch()) {}

  std::set<double> to_remove;

  for (auto &q: fit_data_.peaks_) {
    double nrg = q.second.energy;
    bool has = false;
    for (auto &p : fit_data_2_.peaks_)
      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
        has = true;
    if (!has)
      to_remove.insert(q.first);
  }
  for (auto &q : to_remove)
    fit_data_.remove_peaks(to_remove);

  for (auto &q: fit_data_2_.peaks_) {
    double nrg = q.second.energy;
    bool has = false;
    for (auto &p : fit_data_.peaks_)
      if (std::abs(p.second.energy - nrg) < ui->doubleCullDelta->value())
        has = true;
    if (!has)
      to_remove.insert(q.first);
  }
  for (auto &q : to_remove)
    fit_data_2_.remove_peaks(to_remove);

  ui->plotSpectrum->update_fit(true);
  ui->plotSpectrum2->update_fit(true);
  plot_calib();
}

void FormAnalysis2D::on_pushSymmetrize_clicked()
{
  this->setCursor(Qt::WaitCursor);

  QString sym_spec_name = current_spectrum_ + "_sym";
  bool ok;
  QString text = QInputDialog::getText(this, "New spectrum name",
                                       "Spectrum name:", QLineEdit::Normal,
                                       sym_spec_name,
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (spectra_->by_name(sym_spec_name.toStdString()) != nullptr) {
      PL_WARN << "Spectrum " << sym_spec_name.toStdString() << " already exists. Aborting symmetrization";
      return;
    }

    Pixie::Spectrum::Spectrum* some_spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if ((some_spectrum == nullptr)
        || (some_spectrum->total_count() == 0)
        || (some_spectrum->dimensions() != 2)
        ) {
      PL_WARN << "Original spectrum " << current_spectrum_.toStdString() << " does not exist or has no events or is not 2d";
      return;
    }

    int bits = some_spectrum->bits();
    uint32_t adjrange = static_cast<uint32_t>(some_spectrum->resolution());

    Pixie::Spectrum::Template *temp_sym = Pixie::Spectrum::Factory::getInstance().create_template("2D");
    temp_sym->visible = false;
    temp_sym->name_ = sym_spec_name.toStdString();
    temp_sym->match_pattern = std::vector<int16_t>({1,1,0,0});
    temp_sym->add_pattern = std::vector<int16_t>({1,1,0,0});
    temp_sym->bits = bits;

    Pixie::Spectrum::Spectrum *symspec = Pixie::Spectrum::Factory::getInstance().create_from_template(*temp_sym);
    delete temp_sym;

    PL_INFO << "Created spectrum " << sym_spec_name.toStdString();

    if (gain_match_cali_.to_ == detector1_.name_)
      PL_INFO << "will use gain match cali from " << detector2_.name_ << " to " << detector1_.name_ << " " << gain_match_cali_.to_string();
    else {
      PL_WARN << "no appropriate gain match calibration";
      return;
    }

    boost::random::mt19937 gen;
    boost::random::uniform_real_distribution<> dist(-0.5, 0.5);

    uint16_t e2 = 0;
    std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
        std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
    for (auto it : *spectrum_data) {
      uint64_t count = it.second;

      //PL_DBG << "adding " << it.first[0] << "+" << it.first[1] << "  x" << it.second;
      double xformed = gain_match_cali_.transform(it.first[1]);

      it.second = 1;
      for (int i=0; i < count; ++i) {
        double plus = dist(gen);
        double xfp = xformed + plus;

        if (xfp > 0)
          e2 = static_cast<uint16_t>(std::round(xfp));
        else
          e2 = 0;

        //PL_DBG << xformed << " plus " << plus << " = " << xfp << " round to " << e2;

        it.first[1] = e2;

        symspec->add_bulk(it);
      }
    }

    std::vector<Gamma::Detector> detectors = some_spectrum->get_detectors();
    for (auto &p : detectors) {
      if (p.shallow_equals(detector2_)) {
        p = Gamma::Detector(std::string("*") + detector2_.name_);
        p.energy_calibrations_.add(nrg_calibration1_);
      }
    }

    Pixie::Spill spill;
    spill.run = new Pixie::RunInfo(spectra_->runInfo());
    for (int i=0; i < detectors.size(); ++i) {
      PL_DBG << "push det " << i << detectors[i].name_;
      spill.run->p4_state.set_detector(Pixie::Channel(i), detectors[i]);
    }
    symspec->addSpill(spill);
    delete spill.run;

    spectra_->add_spectrum(symspec);

    emit spectraChanged();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::on_pushFoldData_clicked()
{
  this->setCursor(Qt::WaitCursor);

  QString fold_spec_name = current_spectrum_ + "_fold";
  bool ok;
  QString text = QInputDialog::getText(this, "New spectrum name",
                                       "Spectrum name:", QLineEdit::Normal,
                                       fold_spec_name,
                                       &ok);
  if (ok && !text.isEmpty()) {
    if (spectra_->by_name(fold_spec_name.toStdString()) != nullptr) {
      PL_WARN << "Spectrum " << fold_spec_name.toStdString() << " already exists. Aborting folding";
      return;
    }

    Pixie::Spectrum::Spectrum* some_spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if ((some_spectrum == nullptr)
        || (some_spectrum->total_count() == 0)
        || (some_spectrum->dimensions() != 2)
        ) {
      PL_WARN << "Original spectrum " << current_spectrum_.toStdString() << " does not exist or has no events or is not 2d";
      return;
    }

    int bits = some_spectrum->bits();
    uint32_t adjrange = static_cast<uint32_t>(some_spectrum->resolution());

    Pixie::Spectrum::Template *temp_fold = Pixie::Spectrum::Factory::getInstance().create_template("2D");
    temp_fold->visible = false;
    temp_fold->name_ = fold_spec_name.toStdString();
    temp_fold->match_pattern = std::vector<int16_t>({1,1,0,0});
    temp_fold->add_pattern = std::vector<int16_t>({1,1,0,0});
    temp_fold->bits = bits;

    Pixie::Spectrum::Spectrum *foldspec = Pixie::Spectrum::Factory::getInstance().create_from_template(*temp_fold);
    delete temp_fold;

    PL_INFO << "Created spectrum " << fold_spec_name.toStdString();

    std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
        std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
    for (auto it : *spectrum_data) {

      foldspec->add_bulk(it);

      double e1 = it.first[0];
      double e2 = it.first[1];
      it.first[0] = e2;
      it.first[1] = e1;

      foldspec->add_bulk(it);
    }


    std::vector<Gamma::Detector> detectors = some_spectrum->get_detectors();
    for (auto &p : detectors) {
      if (p.shallow_equals(detector2_) || p.shallow_equals(detector1_)) {
        p = Gamma::Detector(detector1_.name_ + std::string("*") + detector2_.name_);
        p.energy_calibrations_.add(nrg_calibration1_);
      }
    }

    Pixie::Spill spill;
    spill.run = new Pixie::RunInfo(spectra_->runInfo());
    for (int i=0; i < detectors.size(); ++i)
      spill.run->p4_state.set_detector(Pixie::Channel(i), detectors[i]);
    foldspec->addSpill(spill);
    delete spill.run;

    spectra_->add_spectrum(foldspec);

    emit spectraChanged();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::on_pushAddGatedSpectra_clicked()
{
  this->setCursor(Qt::WaitCursor);
  bool success = false;

  if ((gate_x != nullptr) && (gate_x->total_count() > 0)) {
    if (spectra_->by_name(gate_x->name()) != nullptr)
      PL_WARN << "Spectrum " << gate_x->name() << " already exists.";
    else
    {
      gate_x->set_appearance(generateColor().rgba());
      spectra_->add_spectrum(gate_x);
      gatex_in_spectra = true;
      success = true;
    }
  }

  if ((gate_y != nullptr) && (gate_y->total_count() > 0)) {
    if (spectra_->by_name(gate_y->name()) != nullptr)
      PL_WARN << "Spectrum " << gate_y->name() << " already exists.";
    else
    {
      gate_y->set_appearance(generateColor().rgba());
      spectra_->add_spectrum(gate_y);
      gatey_in_spectra = true;
      success = true;
    }
  }

  if (success) {
    emit spectraChanged();
  }
  this->setCursor(Qt::ArrowCursor);
}

void FormAnalysis2D::on_pushSaveCalib_clicked()
{
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
      if (ok && !text.isEmpty()) {
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
      std::vector<Gamma::Detector> detectors = q->get_detectors();
      for (auto &p : detectors) {
        if (p.shallow_equals(detector2_)) {
          PL_INFO << "   applying new calibrations for " << detector2_.name_ << " on " << q->name();
          p.gain_match_calibrations_.replace(gain_match_cali_);
        }
      }
      q->set_detectors(detectors);
    }

    std::vector<Gamma::Detector> detectors = spectra_->runInfo().p4_state.get_detectors();
    for (auto &p : detectors) {
      if (p.shallow_equals(detector2_)) {
        PL_INFO << "   applying new calibrations for " << detector2_.name_ << " in current project " << spectra_->status();
        p.gain_match_calibrations_.replace(gain_match_cali_);
      }
    }
    Pixie::RunInfo ri = spectra_->runInfo();
    for (int i=0; i < detectors.size(); ++i)
      ri.p4_state.set_detector(Pixie::Channel(i), detectors[i]);
    spectra_->setRunInfo(ri);
  }
}
