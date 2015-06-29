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
 *      FormEnergyCalibration -
 *
 ******************************************************************************/

#include "form_energy_calibration.h"
#include "widget_detectors.h"
#include "ui_form_energy_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormEnergyCalibration::FormEnergyCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& dets, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEnergyCalibration),
  marker_table_(my_markers_),
  settings_(settings),
  detectors_(dets),
  selection_model_(&marker_table_)
{
  ui->setupUi(this);

  loadSettings();

  others_have_det_ = false;
  DB_has_detector_ = false;

  ui->PlotCalib->set_scale_type("Linear");
  ui->PlotCalib->showButtonColorThemes(false);
  ui->PlotCalib->showButtonMarkerLabels(false);
  ui->PlotCalib->showButtonPlotStyle(false);
  ui->PlotCalib->showButtonScaleType(false);
  ui->PlotCalib->setZoomable(false);
  ui->PlotCalib->showTitle(false);
  ui->PlotCalib->setLabels("channel", "energy");

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

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
}

FormEnergyCalibration::~FormEnergyCalibration()
{
  delete ui;
}

void FormEnergyCalibration::closeEvent(QCloseEvent *event) {
  if (!ui->isotopes->save_close()) {
    event->ignore();
    return;
  }

  //clear();

  saveSettings();
  event->accept();
}

void FormEnergyCalibration::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->isotopes->setDir(data_directory_);

  settings_.beginGroup("Energy_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->isotopes->set_current_isotope(settings_.value("current_isotope", "Co-60").toString());

  settings_.endGroup();

}

void FormEnergyCalibration::saveSettings() {

  settings_.beginGroup("Energy_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("current_isotope", ui->isotopes->current_isotope());
  settings_.endGroup();
}

/*void FormEnergyCalibration::setData(XMLableDB<Gamma::Detector>& newDetDB) {
  detectors_ = &newDetDB;
  if (detectors_ && detectors_.has_a(detector_))
    DB_has_detector_ = true;
  toggle_radio();
}*/

void FormEnergyCalibration::clear() {
  my_markers_.clear();
  spectrum_name_.clear();
  new_calibration_ = Gamma::Calibration();
  old_calibration_ = Gamma::Calibration();
  detector_ = Gamma::Detector();
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  others_have_det_ = false;
  DB_has_detector_ = false;
  toggle_radio();

  ui->pushFromDB->setEnabled(false);
}


void FormEnergyCalibration::setSpectrum(Pixie::Spectrum::Spectrum *spectrum) {
  clear();
  ui->PlotCalib->clearGraphs();
  ui->PlotCalib->redraw();
  spectrum_ = spectrum;

  if (spectrum_) {
    spectrum_name_ = QString::fromStdString(spectrum_->name());

    int bits = spectrum->bits();
    old_calibration_ = new_calibration_ = Gamma::Calibration(bits);
    for (std::size_t i=0; i < spectrum->add_pattern().size(); i++) {
      if (spectrum->add_pattern()[i] == 1) {
        detector_ = spectrum->get_detectors()[i];
        if (detector_.energy_calibrations_.has_a(Gamma::Calibration(bits))) {
          new_calibration_ = old_calibration_ = detector_.energy_calibrations_.get(Gamma::Calibration(bits));
          PL_INFO << "<Energy calibration> Existing energy calibration drawn from detector \"" << detector_.name_ << "\"";
        } else
          PL_INFO << "<Energy calibration> No existing calibration for this resolution";
      }
    }
  }
}

void FormEnergyCalibration::update_peaks(std::vector<Gamma::Peak> pks) {
  my_markers_.clear();
  for (auto &q : pks)
    my_markers_[q.gaussian_.center_] = old_calibration_.transform(q.gaussian_.center_);

  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormEnergyCalibration::replot_markers() {
  ui->PlotCalib->clearGraphs();

  QVector<double> xx, yy, ycab;

  for (auto &q : my_markers_) {
    xx.push_back(q.first);
    yy.push_back(q.second);
  }

  if (xx.size()) {
    AppearanceProfile pt;
    pt.default_pen = QPen(Qt::darkBlue, 7);

    ui->PlotCalib->reset_scales();
    ui->PlotCalib->addPoints(xx, yy, pt, QCPScatterStyle::ssDiamond);
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
      //expand axes to this even if there is no fit

      xx.clear();
      for (double i=min; i <= max; i+=step) {
        xx.push_back(i);
        ycab.push_back(new_calibration_.transform(i));
      }
      AppearanceProfile ln;
      ln.default_pen = QPen(Qt::blue, 2);

      ui->PlotCalib->addGraph(xx, ycab, ln);
    }
    ui->PlotCalib->rescale();
  }
  ui->PlotCalib->redraw();

  if (!selection_model_.selectedIndexes().empty()) {
    std::set<double> chosen;
    foreach (QModelIndex idx, selection_model_.selectedRows()) {
      QModelIndex chan_ix = marker_table_.index(idx.row(), 0);
      QModelIndex nrg_ix = marker_table_.index(idx.row(), 1);
      double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
      //double nrg = marker_table_.data(nrg_ix, Qt::DisplayRole).toDouble();
      chosen.insert(chan);
    }

    if (isVisible())
      emit peaks_chosen(chosen);
  }

}

void FormEnergyCalibration::update_peak_selection(std::set<double> pks) {
  ui->tableMarkers->blockSignals(true);
  ui->tableMarkers->clearSelection();
  int i=0;
  for (auto &q : my_markers_) {
    if (pks.count(q.first))
      ui->tableMarkers->selectRow(i);
    ++i;
  }
  ui->tableMarkers->blockSignals(false);
}

void FormEnergyCalibration::selection_changed(QItemSelection, QItemSelection) {
  //send selection to formpeak

  toggle_push();
  replot_markers();
}

void FormEnergyCalibration::toggle_push() {
  ui->pushAllEnergies->setEnabled(false);
  ui->pushAllmarkers->setEnabled(false);

  if (!selection_model_.selectedIndexes().empty()) {
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

  if (detectors_.has_a(detector_) && detectors_.get(detector_).energy_calibrations_.has_a(old_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
}

void FormEnergyCalibration::set_conditions(bool others_have_det, bool DB_has_detector) {
  bool others_have_det_ = others_have_det;
  bool DB_has_detector_ = DB_has_detector;
  toggle_radio();
}

void FormEnergyCalibration::toggle_radio() {
  Polynomial thispoly(new_calibration_.coefficients_);
  ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(thispoly.to_UTF8()) + " Rsq=" + QString::number(thispoly.rsq));

  ui->pushApplyCalib->setEnabled(false);
  ui->comboApplyTo->clear();

  if (!new_calibration_.shallow_equals(Gamma::Calibration()) && !detector_.shallow_equals(Gamma::Detector())) {
    if (others_have_det_)
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on all open spectra", QVariant::fromValue((int)Gamma::CalibApplyTo::DetOnAllOpen));
    if (spectrum_) {
      ui->comboApplyTo->addItem("all detectors on " + spectrum_name_, QVariant::fromValue((int)Gamma::CalibApplyTo::AllDetsOnCurrentSpec));
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " on " + spectrum_name_, QVariant::fromValue((int)Gamma::CalibApplyTo::DetOnCurrentSpec));
    }
    if (DB_has_detector_)
      ui->comboApplyTo->addItem(QString::fromStdString(detector_.name_) + " in detector database", QVariant::fromValue((int)Gamma::CalibApplyTo::DetInDB));
  }

}

void FormEnergyCalibration::on_pushFit_clicked()
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
    new_calibration_.model_ = Gamma::CalibrationModel::polynomial;
    PL_INFO << "<Energy calibration> New calibration coefficients = " << new_calibration_.coef_to_string();
    toggle_radio();
  }
  else
    PL_INFO << "<Energy calibration> Gamma::Calibration failed";

  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormEnergyCalibration::isotope_energies_chosen() {
  toggle_push();
}

void FormEnergyCalibration::on_pushApplyCalib_clicked()
{
  PL_INFO << "<Gamma::Calibration> Applying calibration";
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

void FormEnergyCalibration::on_pushFromDB_clicked()
{
  Gamma::Detector newdet = detectors_.get(detector_);
  new_calibration_ = newdet.energy_calibrations_.get(old_calibration_);
}

void FormEnergyCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormEnergyCalibration::on_pushAllmarkers_clicked()
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

void FormEnergyCalibration::on_pushAllEnergies_clicked()
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

