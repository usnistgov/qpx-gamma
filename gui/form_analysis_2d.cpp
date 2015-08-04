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

  res = 0;
  xmin = 0;
  xmax = 0;
  ymin = 0;
  ymax = 0;

  live_seconds = 0;
  sum_inclusive = 0;
  sum_exclusive = 0;
  sum_no_peaks = 0;
  sum_prism = 0;

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
  xmin = ymin = 0;
  xmax = ymax = 0;

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
}


void FormAnalysis2D::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;

  if (spectra_) {

    current_spectrum_ = name;
    Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if (spectrum && spectrum->resolution()) {
      int bits = spectrum->bits();
      res = spectrum->resolution();

      xmin = 0;
      xmax = res - 1;
      ymin = 0;
      ymax = res - 1;

      detector1_ = spectrum->get_detector(0);
      if (detector1_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
        nrg_calibration1_ = detector1_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      fwhm_calibration1_ = detector1_.fwhm_calibration_;

      detector2_ = spectrum->get_detector(1);
      if (detector2_.energy_calibrations_.has_a(Gamma::Calibration("Energy", bits)))
        nrg_calibration2_ = detector2_.energy_calibrations_.get(Gamma::Calibration("Energy", bits));
      fwhm_calibration2_ = detector2_.fwhm_calibration_;

      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_);
      ui->plotMatrix->set_spectrum(current_spectrum_);
      ui->plotMatrix->update_plot();

      ui->plotSpectrum->setData(nrg_calibration1_, fwhm_calibration1_);
      ui->plotSpectrum2->setData(nrg_calibration2_, fwhm_calibration2_);
    }
  }

  make_gated_spectra();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    Pixie::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());
    if (spectrum && spectrum->resolution())
      live_seconds = spectrum->live_time().total_seconds();

    ui->plotMatrix->update_plot(true);
    ui->plotSpectrum->update_spectrum();
    ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
    QWidget::showEvent(event);
    //ui->plotMatrix->update_plot(true);
}

void FormAnalysis2D::update_peaks(bool content_changed) {
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  if (!xx.visible || !yy.visible) {
    xmin = 0;
    xmax = res - 1;
    ymin = 0;
    ymax = res - 1;
  } else {
    int width = ui->plotMatrix->gate_width() / 2;
    xmin = xx.channel - width; if (xmin < 0) xmin = 0;
    xmax = xx.channel + width; if (xmax >= res) xmax = res - 1;
    ymin = yy.channel - width; if (ymin < 0) ymin = 0;
    ymax = yy.channel + width; if (ymax >= res) ymax = res - 1;
  }

  make_gated_spectra();

}

void FormAnalysis2D::make_gated_spectra() {

  Pixie::Spectrum::Spectrum* some_spectrum = spectra_->by_name(current_spectrum_.toStdString());

  PL_DBG << "Coincidence gate x[" << xmin << "-" << xmax << "]   y[" << ymin << "-" << ymax << "]";

  if ((some_spectrum != nullptr)
      && some_spectrum->total_count()
      && (some_spectrum->dimensions() == 2)
      )
  {
      int bits = some_spectrum->bits();
      uint32_t adjrange = static_cast<uint32_t>(some_spectrum->resolution()) - 1;

      tempx->bits = bits;
      tempx->name_ = detector1_.name_ + "[" + std::to_string(nrg_calibration2_.transform(ymin)) + "," + std::to_string(nrg_calibration2_.transform(ymax)) + "]";

      tempy->bits = bits;
      tempy->name_ = detector2_.name_ + "[" + std::to_string(nrg_calibration1_.transform(xmin)) + "," + std::to_string(nrg_calibration1_.transform(xmax)) + "]";

      if (gate_x != nullptr)
        delete gate_x;
      gate_x = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempx);

      if (gate_y != nullptr)
        delete gate_y;
      gate_y = Pixie::Spectrum::Factory::getInstance().create_from_template(*tempy);

      sum_inclusive = 0;
      sum_exclusive = 0;

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(some_spectrum->get_spectrum({{xmin, xmax}, {0, adjrange}}));
      for (auto it : *spectrum_data) {
        if ((it.first[0] >= xmin) && (it.first[0] <= xmax)) {
          gate_y->add_bulk(it);
          if ((it.first[1] >= ymin) && (it.first[1] <= ymax))
            sum_exclusive += it.second;
          else
            sum_inclusive += it.second;
        }
      }

      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data2 =
          std::move(some_spectrum->get_spectrum({{0, adjrange}, {ymin, ymax}}));
      for (auto it : *spectrum_data2) {
        if ((it.first[1] >= ymin) && (it.first[1] <= ymax)) {
          gate_x->add_bulk(it);
          if ((it.first[0] < xmin) || (it.first[0] > xmax))
            sum_inclusive += it.second;
        }
      }

      sum_inclusive += sum_exclusive;
      fill_table();

      Pixie::Spill spill;
      spill.run = new Pixie::RunInfo(spectra_->runInfo());
      gate_x->addSpill(spill);
      gate_y->addSpill(spill);
      delete spill.run;

      ui->plotSpectrum->setSpectrum(gate_x, xmin, xmax);
      ui->plotSpectrum->setData(nrg_calibration1_, fwhm_calibration1_);
      ui->plotSpectrum->update_spectrum();

      ui->plotSpectrum2->setSpectrum(gate_y, ymin, ymax);
      ui->plotSpectrum2->setData(nrg_calibration2_, fwhm_calibration2_);
      ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::fill_table()
{
  ui->tableCoincResults->setItem(0, 1, new QTableWidgetItem( QString::number(sum_inclusive) ));
  ui->tableCoincResults->setItem(0, 2, new QTableWidgetItem( QString::number(sum_inclusive / live_seconds) ));

  ui->tableCoincResults->setItem(1, 1, new QTableWidgetItem( QString::number(sum_exclusive) ));
  ui->tableCoincResults->setItem(1, 2, new QTableWidgetItem( QString::number(sum_exclusive / live_seconds) ));
}
