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

FormAnalysis2D::FormAnalysis2D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis2D),
  detectors_(newDetDB),
  settings_(settings),
  gate_x(nullptr),
  gate_y(nullptr)
{
  ui->setupUi(this);

  loadSettings();

  ui->plotSpectrum->setFit(&fit_data_);
  ui->plotSpectrum2->setFit(&fit_data_2_);

  ui->plotMatrix->set_show_analyse(false);
  ui->plotMatrix->set_show_selector(false);
  connect(ui->plotMatrix, SIGNAL(markers_set(Marker,Marker)), this, SLOT(update_gates(Marker,Marker)));

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

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

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
}

void FormAnalysis2D::saveSettings() {
  ui->plotSpectrum->saveSettings(settings_);
}

void FormAnalysis2D::clear() {
  ui->plotSpectrum->setSpectrum(nullptr);
  current_spectrum_.clear();
  detector_ = Gamma::Detector();
  nrg_calibration_ = Gamma::Calibration();
  fwhm_calibration_ = Gamma::Calibration();
}


void FormAnalysis2D::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  ui->plotSpectrum->setSpectrum(nullptr);
  ui->plotSpectrum2->setSpectrum(nullptr);

  if (!spectra_) {
    return;
  }

  current_spectrum_ = name;
  Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

  if (spectrum && spectrum->resolution()) {
    int bits = spectrum->bits();
    nrg_calibration_ = Gamma::Calibration("Energy", bits);
    fwhm_calibration_ = Gamma::Calibration("FWHM", bits);
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];

        //what if multiple available?

        if (detector_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits))) {
          nrg_calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
          PL_INFO << "<Analysis> Energy calibration drawn from detector \"" << detector_.name_ << "\"   coefs = "
                  << nrg_calibration_.coef_to_string();
        } else
          PL_INFO << "<Analysis> No existing calibration for this resolution";

        if (detector_.fwhm_calibrations_.has_a(Gamma::Calibration("FWHM", bits))) {
          fwhm_calibration_ = detector_.fwhm_calibrations_.get(Gamma::Calibration("FWHM", bits));
          PL_INFO << "<Analysis> FWHM calibration drawn from detector \"" << detector_.name_ << "\"   coefs = "
                  << fwhm_calibration_.coef_to_string();
        } else
          PL_INFO << "<Analysis> No existing FWHM calibration for this resolution";

      }
    }
    ui->plotMatrix->setSpectra(*spectra_);
    ui->plotMatrix->set_spectrum(current_spectrum_);
    ui->plotMatrix->update_plot();

    ui->plotSpectrum->setData(nrg_calibration_, fwhm_calibration_);
    ui->plotSpectrum2->setData(nrg_calibration_, fwhm_calibration_);
  }
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    ui->plotMatrix->update_plot();
    ui->plotSpectrum->update_spectrum();
    ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::update_peaks(bool content_changed) {
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  Pixie::Spectrum::Spectrum* some_spectrum = spectra_->by_name(current_spectrum_.toStdString());

  int width = ui->plotMatrix->gate_width() / 2;

  int xmin = xx.channel - width;
  int xmax = xx.channel + width;
  int ymin = yy.channel - width;
  int ymax = yy.channel + width;

  PL_DBG << "Coincidence gate x[" << xmin << "-" << xmax << "]   y[" << ymin << "-" << ymax << "]";

  if ((some_spectrum != nullptr)
      && some_spectrum->total_count()
      && (some_spectrum->dimensions() == 2)
      )
  {
      int bits = some_spectrum->bits();
      uint32_t adjrange = static_cast<uint32_t>(some_spectrum->resolution()) - 1;
      PL_DBG <<  "spectrum " << current_spectrum_.toStdString() << " with " << bits << " bits up to " << adjrange << " channels";


      tempx->bits = bits;
      tempy->bits = bits;

      if (gate_x != nullptr)
        delete gate_x;
      gate_x = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempx);

      if (gate_y != nullptr)
        delete gate_y;
      gate_y = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempy);


      Pixie::Spill spillx, spilly;
      std::bitset<Pixie::kNumChans> pattern;
      pattern[0] = 1;
      pattern[1] = 1;

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {0, adjrange}}));
      for (auto it : *spectrum_data) {
        Pixie::Hit hit;
        hit.pattern = pattern;
        hit.energy = std::vector<uint16_t>({it.first[0] << 2, it.first[1] << 2, 0, 0});
        if ((it.first[0] > xmin) && (it.first[0] < xmax))
          for (int i=0; i < it.second; ++i)
            spilly.hits.push_back(hit);
        if ((it.first[1] > ymin) && (it.first[1] < ymax))
          for (int i=0; i < it.second; ++i)
            spillx.hits.push_back(hit);
      }

      PL_DBG << "spillx has " << spillx.hits.size();
      PL_DBG << "spilly has " << spilly.hits.size();

      gate_x->addSpill(spillx);
      gate_y->addSpill(spilly);

      PL_DBG << "gatex count = " << gate_x->total_count();
      PL_DBG << "gatey count = " << gate_x->total_count();

      ui->plotSpectrum->setSpectrum(gate_x);
      ui->plotSpectrum->update_spectrum();
      ui->plotSpectrum2->setSpectrum(gate_y);
      ui->plotSpectrum2->update_spectrum();
  }
}
