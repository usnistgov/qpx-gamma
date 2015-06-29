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
 *      FormFwhmCalibration -
 *
 ******************************************************************************/

#include "form_fwhm_calibration.h"
#include "widget_detectors.h"
#include "ui_form_fwhm_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormFwhmCalibration::FormFwhmCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormFwhmCalibration),
  settings_(settings),
  detectors_(dets)
{
  ui->setupUi(this);

  loadSettings();

  others_have_det_ = false;
  DB_has_detector_ = false;

  calib_point.default_pen = QPen(Qt::darkCyan, 7);
  calib_selected.default_pen = QPen(Qt::darkRed, 7);
  calib_line.default_pen = QPen(Qt::blue, 0);

  ui->PlotCalib->set_scale_type("Linear");
  ui->PlotCalib->showButtonColorThemes(false);
  ui->PlotCalib->showButtonMarkerLabels(false);
  ui->PlotCalib->showButtonPlotStyle(false);
  ui->PlotCalib->showButtonScaleType(false);
  ui->PlotCalib->setZoomable(false);
  ui->PlotCalib->showTitle(false);
  ui->PlotCalib->setLabels("energy", "FWHM");

  ui->tableFWHM->verticalHeader()->hide();
  ui->tableFWHM->setColumnCount(4);
  ui->tableFWHM->setHorizontalHeaderLabels({"chan", "energy", "fwmw (gaussian)", "fwhm (pseudo-Voigt)"});
  ui->tableFWHM->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableFWHM->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tableFWHM->horizontalHeader()->setStretchLastSection(true);
  ui->tableFWHM->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableFWHM->show();

  connect(ui->tableFWHM, SIGNAL(itemSelectionChanged()),
          this, SLOT(selection_changed()));

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
}

FormFwhmCalibration::~FormFwhmCalibration()
{
  delete ui;
}

void FormFwhmCalibration::closeEvent(QCloseEvent *event) {
  //clear();

  saveSettings();
  event->accept();
}

void FormFwhmCalibration::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();


  settings_.beginGroup("FWHM_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());

  settings_.endGroup();

}

void FormFwhmCalibration::saveSettings() {

  settings_.beginGroup("FWHM_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.endGroup();
}

/*void FormFwhmCalibration::setData(XMLableDB<Gamma::Detector>& newDetDB) {
  detectors_ = &newDetDB;
  if (detectors_ && detectors_.has_a(detector_))
    DB_has_detector_ = true;
  toggle_radio();
}*/

void FormFwhmCalibration::clear() {
  spectrum_name_.clear();
  detector_ = Gamma::Detector();
  peaks_.clear();
  ui->tableFWHM->clearContents();
  toggle_push();
  others_have_det_ = false;
  DB_has_detector_ = false;
  toggle_radio();

  ui->pushFromDB->setEnabled(false);
}


void FormFwhmCalibration::setSpectrum(Pixie::Spectrum::Spectrum *spectrum) {
  clear();
  ui->PlotCalib->clearGraphs();
  ui->PlotCalib->redraw();

  spectrum_ = spectrum;

  if (spectrum_) {
    spectrum_name_ = QString::fromStdString(spectrum_->name());

    int bits = spectrum->bits();
    nrg_calibration_ = Gamma::Calibration(bits);
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];
        if (detector_.energy_calibrations_.has_a(Gamma::Calibration(bits))) {
          nrg_calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration(bits));
          PL_INFO << "<WFHM calibration> Energy calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "<WFHM calibration> No existing energy calibration for this resolution";
      }
    }

    new_fwhm_calibration_ = old_fwhm_calibration_ = Gamma::Calibration(bits);
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];
        if (detector_.fwhm_calibrations_.has_a(Gamma::Calibration(bits))) {
          new_fwhm_calibration_ = old_fwhm_calibration_ = detector_.fwhm_calibrations_.get(Gamma::Calibration(bits));
          PL_INFO << "<WFHM calibration> Existing FWHM calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "<WFHM calibration> No existing FWHM calibration for this resolution";
      }
    }


  }

}

void FormFwhmCalibration::update_peaks(std::vector<Gamma::Peak> pks) {
  peaks_ = pks;

  std::sort(peaks_.begin(), peaks_.end(), Gamma::Peak::by_centroid_gaussian);

  ui->tableFWHM->clearContents();
  ui->tableFWHM->setRowCount(peaks_.size());
  for (int i=0; i<peaks_.size(); ++i) {
    ui->tableFWHM->setItem(i, 0, new QTableWidgetItem( QString::number(peaks_[i].center) ));
    ui->tableFWHM->setItem(i, 1, new QTableWidgetItem( QString::number(peaks_[i].energy) ));
    ui->tableFWHM->setItem(i, 2, new QTableWidgetItem( QString::number(peaks_[i].fwhm_gaussian) ));
    ui->tableFWHM->setItem(i, 3, new QTableWidgetItem( QString::number(peaks_[i].fwhm_pseudovoigt) ));
  }
  toggle_push();
  replot_markers();
}

void FormFwhmCalibration::replot_markers() {
  PL_INFO << "<WFHM calibration> Replotting markers";
  ui->PlotCalib->clearGraphs();
  ui->PlotCalib->reset_scales();

  QVector<double> xx, yy;

  double xmin = 0, xmax = 0;
  std::map<double, double> minima, maxima;

  if (peaks_.size()) {
    xmax = xmin = nrg_calibration_.transform(peaks_[0].gaussian_.center_);
  }

  for (auto &q : peaks_) {
    double x = nrg_calibration_.transform(q.gaussian_.center_);
    double y = nrg_calibration_.transform(q.pseudovoigt_.hwhm_l + q.pseudovoigt_.hwhm_r);

    xx.push_back(x);
    yy.push_back(y);

    if (!minima.count(x) || (minima[x] > y))
      minima[x] = y;
    if (!maxima.count(x) || (maxima[x] < y))
      maxima[x] = y;

    if (x < xmin)
      xmin = x;
    if (x > xmax)
      xmax = x;
  }

  double x_margin = (xmax -xmin) / 8;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size()) {
    ui->PlotCalib->addPoints(xx, yy, calib_point, QCPScatterStyle::ssSquare);
    if ((new_fwhm_calibration_ != Gamma::Calibration()) && (new_fwhm_calibration_.units_ == "keV")) {
      PL_INFO << "<WFHM calibration> Will plot calibration curve";


      double step = xmax-xmin;
      xx.clear(); yy.clear();

      for (double x=xmin; x <= xmax; x+=step) {
        double y = new_fwhm_calibration_.transform(x);
        xx.push_back(x);
        yy.push_back(y);

        if (!minima.count(x) || (minima[x] > y))
          minima[x] = y;
        if (!maxima.count(x) || (maxima[x] < y))
          maxima[x] = y;
      }

      ui->PlotCalib->addGraph(xx, yy, calib_line);
    }
  }

  if (!ui->tableFWHM->selectionModel()->selectedIndexes().empty()) {
    xx.clear(); yy.clear();
    std::set<double> chosen;
    foreach (QModelIndex idx, ui->tableFWHM->selectionModel()->selectedRows()) {
      double chan = peaks_[idx.row()].gaussian_.center_;
      //double nrg = marker_table_.data(nrg_ix, Qt::DisplayRole).toDouble();
      xx.push_back(nrg_calibration_.transform(chan));
      yy.push_back(nrg_calibration_.transform(peaks_[idx.row()].pseudovoigt_.hwhm_l + peaks_[idx.row()].pseudovoigt_.hwhm_r));
      chosen.insert(chan);
    }
    ui->PlotCalib->addPoints(xx, yy, calib_selected, QCPScatterStyle::ssCrossSquare);
    if (isVisible())
      emit peaks_chosen(chosen);
  }

  ui->PlotCalib->setYBounds(minima, maxima);
  ui->PlotCalib->rescale();
  ui->PlotCalib->redraw();
}

void FormFwhmCalibration::update_peak_selection(std::set<double> pks) {
  ui->tableFWHM->blockSignals(true);
  ui->tableFWHM->clearSelection();
  for (int i=0; i < peaks_.size(); ++i) {
    if (pks.count(peaks_[i].gaussian_.center_))
      ui->tableFWHM->selectRow(i);
  }
  ui->tableFWHM->blockSignals(false);
}

void FormFwhmCalibration::selection_changed() {
  toggle_push();
  replot_markers();

}

void FormFwhmCalibration::toggle_push() {
  if (static_cast<int>(peaks_.size()) >= 2) {
    ui->pushFit->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
  }

  if (detectors_.has_a(detector_) && detectors_.get(detector_).fwhm_calibrations_.has_a(old_fwhm_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
}

void FormFwhmCalibration::set_conditions(bool others_have_det, bool DB_has_detector) {
  bool others_have_det_ = others_have_det;
  bool DB_has_detector_ = DB_has_detector;
  toggle_radio();
}

void FormFwhmCalibration::toggle_radio() {
  //Polynomial thispoly(new_calibration_.coefficients_);
  //ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(thispoly.to_UTF8()) + " Rsq=" + QString::number(thispoly.rsq));

  ui->pushApplyCalib->setEnabled(false);
  ui->comboApplyTo->clear();

/*  if (!new_calibration_.shallow_equals(Gamma::Calibration()) && !detector_.shallow_equals(Gamma::Detector())) {
    if (others_have_det_)
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on all open spectra", QVariant::fromValue((int)CalibApplyTo::DetOnAllOpen));
    if (spectrum_) {
      ui->comboApplyTo->addItem("all detectors on " + spectrum_name_, QVariant::fromValue((int)CalibApplyTo::AllDetsOnCurrentSpec));
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on " + spectrum_name_, QVariant::fromValue((int)CalibApplyTo::DetOnCurrentSpec));
    }
    if (DB_has_detector_)
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " in detector database", QVariant::fromValue((int)CalibApplyTo::DetInDB));
  }*/

}

void FormFwhmCalibration::on_pushFit_clicked()
{  
  std::vector<double> xx, yy;
  for (auto &q : peaks_) {
    double x = nrg_calibration_.transform(q.gaussian_.center_);
    double y = nrg_calibration_.transform(q.pseudovoigt_.hwhm_l + q.pseudovoigt_.hwhm_r);
    xx.push_back(x);
    yy.push_back(y);
  }

  Polynomial p = Polynomial(xx, yy, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    new_fwhm_calibration_.coefficients_ = p.coeffs_;
    new_fwhm_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_fwhm_calibration_.units_ = "keV";
    new_fwhm_calibration_.model_ = Gamma::CalibrationModel::polynomial;
    PL_INFO << "<WFHM calibration> New calibration coefficients = " << new_fwhm_calibration_.coef_to_string();
    toggle_radio();
  }
  else
    PL_INFO << "<WFHM calibration> Gamma::Calibration failed";

  toggle_push();
  replot_markers();
}

void FormFwhmCalibration::on_pushApplyCalib_clicked()
{
  PL_INFO << "<WFHM calibration> Applying calibration";
/*
  if ((ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetInDB) && detectors_.has_a(detector_)) {
    Gamma::Detector modified = detectors_.get(detector_);
    if (modified.energy_calibrations_.has_a(new_calibration_))
      modified.energy_calibrations_.replace(new_calibration_);
    else
      modified.energy_calibrations_.add(new_calibration_);
    detectors_.replace(modified);
    emit detectorsChanged();
  } else {
    for (auto &q : spectra_->spectra()) {
      if (q == nullptr)
        continue;
      bool is_selected_spectrum = (q->name() == spectrum_name_.toStdString());
      if ((ui->comboApplyTo->currentData() == (int)CalibApplyTo::DetOnAllOpen) || is_selected_spectrum) {
        std::vector<Gamma::Detector> detectors = q->get_detectors();
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
  }*/

}

void FormFwhmCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(detector_);
  new_fwhm_calibration_ = newdet.energy_calibrations_.get(old_fwhm_calibration_);
}

void FormFwhmCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}
