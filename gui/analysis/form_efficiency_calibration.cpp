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
 *      FormEfficiencyCalibration -
 *
 ******************************************************************************/

#include "form_efficiency_calibration.h"
#include "widget_detectors.h"
#include "ui_form_efficiency_calibration.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include <QInputDialog>
#include "dialog_spectrum.h"

#include "polynomial.h"
#include "polylog.h"
#include "log_inverse.h"
#include "effit.h"

FormEfficiencyCalibration::FormEfficiencyCalibration(QSettings &settings, XMLableDB<Qpx::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEfficiencyCalibration),
  detectors_(newDetDB),
  settings_(settings)
{
  ui->setupUi(this);
  this->setWindowTitle("Efficiency calib");

  loadSettings();

  style_fit.default_pen = QPen(Qt::blue, 0);
  style_pts.themes["selected"] = QPen(Qt::black, 7);

  ui->PlotCalib->setLabels("channel", "energy");

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(4);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "cps", QString(QChar(0x03B5)) + "-rel"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_calib_plot()));
  ui->PlotCalib->set_scale_type_x("Logarithmic");
  ui->PlotCalib->set_scale_type_y("Logarithmic");

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(isotopeSelected()), this, SLOT(isotope_chosen()));

  QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tablePeaks);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushMarkerRemove_clicked()));



  ui->isotopes->set_editable(false);



  //file formats for opening mca spectra
  std::vector<std::string> spectypes = Qpx::Spectrum::Factory::getInstance().types();
  QStringList filetypes;
  for (auto &q : spectypes) {
    Qpx::Spectrum::Template* type_template = Qpx::Spectrum::Factory::getInstance().create_template("1D");
    if (!type_template->input_types.empty())
      filetypes.push_back("Spectrum " + QString::fromStdString(q) + "(" + catExtensions(type_template->input_types) + ")");
    delete type_template;
  }
  mca_load_formats_ = catFileTypes(filetypes);

  ui->plotSpectrum->setFit(&fit_data_);


  connect(ui->plotSpectrum, SIGNAL(selection_changed(std::set<double>)), this, SLOT(update_selection(std::set<double>)));
  connect(ui->plotSpectrum, SIGNAL(data_changed()), this, SLOT(update_data()));

  connect(ui->spectrumSelector, SIGNAL(itemSelected(SelectorItem)), this, SLOT(spectrumDetails(SelectorItem)));
  connect(ui->spectrumSelector, SIGNAL(itemToggled(SelectorItem)), this, SLOT(spectrumLooksChanged(SelectorItem)));
}

FormEfficiencyCalibration::~FormEfficiencyCalibration()
{
  delete ui;
}

void FormEfficiencyCalibration::closeEvent(QCloseEvent *event) {
  //if (!ui->isotopes->save_close()) {
  //  event->ignore();
  //  return;
  //}

  ui->plotSpectrum->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormEfficiencyCalibration::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
  ui->isotopes->setDir(settings_directory_);

  settings_.beginGroup("Efficiency_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->isotopes->set_current_isotope(settings_.value("current_isotope", "Co-60").toString());
  ui->doubleEpsilonE->setValue(settings_.value("epsilon_e", 2.0).toDouble());
  settings_.endGroup();
}

void FormEfficiencyCalibration::saveSettings() {
  ui->plotSpectrum->saveSettings(settings_);

  settings_.beginGroup("Efficiency_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("current_isotope", ui->isotopes->current_isotope());
  settings_.setValue("epsilon_e", ui->doubleEpsilonE->value());
  settings_.endGroup();
}


void FormEfficiencyCalibration::setDetector(Qpx::SpectraSet *newset, QString detector) {
//  my_peak_fitter_->clear();

  current_detector_ = detector.toStdString();
  ui->labelCalibForDet->setText("Efficiency calibration for " + QString::fromStdString(current_detector_));

  spectra_ = newset;

  update_spectra();
}

void FormEfficiencyCalibration::setSpectrum(QString name) {
  if (!fit_data_.metadata_.name.empty()) //should be ==Qpx::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;

  fit_data_ = Qpx::Fitter();

  Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(name.toStdString());

  if (spectrum && spectrum->resolution()) {
    if (peak_sets_.count(name.toStdString())) {
      fit_data_ = peak_sets_.at(name.toStdString());
      ui->isotopes->set_current_isotope(QString::fromStdString(fit_data_.sample_name_));
    }  else {
      Qpx::Spectrum::Metadata md = spectrum->metadata();
      if (!md.description.empty()) {
        //find among data
        ui->isotopes->set_current_isotope(QString::fromStdString(md.description));
      }

      fit_data_.clear();
      fit_data_.setData(spectrum);
      peak_sets_[name.toStdString()] = fit_data_;
    }
    ui->doubleScaleFactor->setValue(fit_data_.activity_scale_factor_);
    ui->doubleScaleFactor->setEnabled(true);
  } else {
    spectrum = nullptr;
    fit_data_ = Qpx::Fitter();
    ui->plotSpectrum->update_spectrum();
    ui->doubleScaleFactor->setEnabled(false);
  }

  ui->plotSpectrum->update_spectrum();

  update_data();
}

void FormEfficiencyCalibration::update_spectrum() {
  if (this->isVisible())
    ui->plotSpectrum->update_spectrum();
}

void FormEfficiencyCalibration::update_data() {
  double max = 0;
  std::set<double> flagged;
  fit_data_.sample_name_ = ui->isotopes->current_isotope().toStdString();
  for (auto &q : fit_data_.peaks()) {
//    q.second.flagged = false;
    for (auto &p : ui->isotopes->current_isotope_gammas()) {
      double diff = abs(q.second.energy - p.energy);
      if (diff < ui->doubleEpsilonE->value()) {
        Qpx::Peak pk = q.second; //BROKEN
//        pk.flagged = true;
        pk.intensity_theoretical_ = p.abundance;
        pk.efficiency_relative_ = (pk.cps_best / pk.intensity_theoretical_);
        if (pk.efficiency_relative_ > max)
          max = pk.efficiency_relative_;
        flagged.insert(pk.energy);
      }
    }
  }

  ui->isotopes->select_energies(flagged);

  //BROKEN
  if (max > 0) {
    for (auto &q : fit_data_.peaks()) {
//      if (q.second.intensity_theoretical_ > 0)
//        q.second.efficiency_relative_ = q.second.efficiency_relative_ / max;
    }
  }

  if (!fit_data_.metadata_.name.empty()) //should be ==Qpx::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;

//  ui->plotSpectrum->update_fit(contents_changed);
  replot_calib();
  rebuild_table(true);
  toggle_push();
}

void FormEfficiencyCalibration::update_detector_calibs()
{
  std::string msg_text("Propagating calibration ");
  msg_text +=  "<nobr>" + new_calibration_.to_string() + "</nobr><br/>";

  std::string question_text("Do you want to save this calibration to ");
  question_text += current_detector_ + " in detector database?";

  QMessageBox msgBox;
  msgBox.setText(QString::fromStdString(msg_text));
  msgBox.setInformativeText(QString::fromStdString(question_text));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();

  Qpx::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(Qpx::Detector(current_detector_))) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(current_detector_),
                                           &ok);

      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = Qpx::Detector(current_detector_);
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Qpx::Detector();
        }
      }
    } else
      modified = detectors_.get(Qpx::Detector(current_detector_));

    if (modified != Qpx::Detector())
    {
      PL_INFO << "   applying new calibrations for " << modified.name_ << " in detector database";
      modified.efficiency_calibration_ = new_calibration_;
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }
}

void FormEfficiencyCalibration::update_spectra() {
  QVector<SelectorItem> items;

  for (auto &q : spectra_->spectra(1, -1)) {
    Qpx::Spectrum::Metadata md;
    if (q != nullptr)
      md = q->metadata();

    if (!md.detectors.empty() && (md.detectors.front().name_ == current_detector_)) {
      SelectorItem new_spectrum;
      new_spectrum.text = QString::fromStdString(md.name);
      new_spectrum.color = QColor(QString::fromStdString(md.attributes.get(Qpx::Setting("appearance")).value_text));
      new_spectrum.visible = md.attributes.get(Qpx::Setting("visible")).value_int;
      items.push_back(new_spectrum);
    }
  }

  ui->spectrumSelector->setItems(items);

  setSpectrum(ui->spectrumSelector->selected().text);
}

void FormEfficiencyCalibration::spectrumLooksChanged(SelectorItem item) {
  replot_calib();
}

void FormEfficiencyCalibration::spectrumDetails(SelectorItem item)
{
  setSpectrum(item.text);
}

void FormEfficiencyCalibration::add_peak_to_table(const Qpx::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  QTableWidgetItem *chn = new QTableWidgetItem( QString::number(p.center) );
  chn->setData(Qt::EditRole, QVariant::fromValue(p.center));
  chn->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, chn);

  QTableWidgetItem *nrg = new QTableWidgetItem( QString::number(p.energy) );
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);

  QTableWidgetItem *cps = new QTableWidgetItem(QString::number(p.cps_best));
  cps->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, cps);

  QTableWidgetItem *norm = new QTableWidgetItem(QString::number(p.efficiency_relative_));
  norm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, norm);
}


void FormEfficiencyCalibration::replot_calib() {
//  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearPoints();

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  QVector<SelectorItem> items = ui->spectrumSelector->items();

  bool have_data = false;

  for (auto &fit : peak_sets_) {
    bool visible = false;
    QColor color;
    for (auto &i : items) {
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString())) {
        visible = true;
        color = i.color;
      }
    }

    if (visible) {
      have_data = true;
      xx.clear(); yy.clear();

      for (auto &q : fit.second.peaks()) {
        if (q.second.intensity_theoretical_ == 0)
          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_ * fit.second.activity_scale_factor_;

        xx.push_back(x);
        yy.push_back(y);

        if (x < xmin)
          xmin = x;
        if (x > xmax)
          xmax = x;
      }

      QVector<double> yy_sigma(yy.size(), 0);

      style_pts.default_pen = QPen(color, 7);
      ui->PlotCalib->addPoints(xx, yy, yy_sigma, style_pts);
    }
  }


  if (have_data) {
    ui->PlotCalib->set_selected_pts(selected_peaks_);
    if (new_calibration_.valid()) {
      xx.clear(); yy.clear();
      double step = (xmax-xmin) / 300.0;
      xmin -= step;
      if (xmin < 0)
        xmin = 0;

      xmax += (step * 100);

      for (double i=xmin; i < xmax; i+=step) {
        double y = new_calibration_.transform(i);
        xx.push_back(i);
        yy.push_back(y);
      }
      ui->PlotCalib->addFit(xx, yy, style_fit);
      ui->PlotCalib->setFloatingText("ε = " + QString::fromStdString(new_calibration_.fancy_equation(3, true)));
    }
  }

}

void FormEfficiencyCalibration::rebuild_table(bool contents_changed) {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  if (contents_changed) {
    ui->tablePeaks->clearContents();
    ui->tablePeaks->setRowCount(fit_data_.peaks().size());

    bool gray = false;
    QColor background_col;
    int i=0;
    for (auto &q : fit_data_.peaks()) {
/*      if (q.second.flagged && gray)
        background_col = Qt::darkGreen;
      else */if (gray)
        background_col = Qt::lightGray;
//      else if (q.second.flagged)
//        background_col = Qt::green;
      else
        background_col = Qt::white;
      add_peak_to_table(q.second, i, background_col);
      ++i;
//      if (!q.second.intersects_R)
//        gray = !gray;
    }
  }

  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks()) {
    if (selected_peaks_.count(q.second.center) > 0) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormEfficiencyCalibration::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed)
    select_in_table();
}

void FormEfficiencyCalibration::select_in_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks()) {
    if (selected_peaks_.count(q.second.center) > 0) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}


void FormEfficiencyCalibration::selection_changed_in_calib_plot() {
  selected_peaks_ = ui->PlotCalib->get_selected_pts();
  ui->plotSpectrum->set_selected_peaks(ui->PlotCalib->get_selected_pts());
  rebuild_table(false);
  toggle_push();
}

void FormEfficiencyCalibration::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble());
  ui->plotSpectrum->set_selected_peaks(selected_peaks_);
  replot_calib();
  toggle_push();
}

void FormEfficiencyCalibration::toggle_push() {
  int unflagged = 0; //BROKEN


  ui->pushCullPeaks->setEnabled(unflagged > 0);

  int points_for_calib = 0;
  QVector<SelectorItem> items = ui->spectrumSelector->items();
  for (auto &fit : peak_sets_) {
    bool visible = false;

    for (auto &i : items)
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString()))
        visible = true;

    if (visible) { //BROKEN
//      for (auto &q : fit.second.peaks())
//        if (q.second.flagged)
//          points_for_calib++;
    }
  }

  ui->pushFit->setEnabled(points_for_calib > 1);
  ui->pushFit_2->setEnabled(points_for_calib > 1);
  ui->pushFitEffit->setEnabled(points_for_calib > 1);
  ui->spinTerms->setEnabled(points_for_calib > 1);

  if (new_calibration_ != Qpx::Calibration())
    ui->pushApplyCalib->setEnabled(true);
  else
    ui->pushApplyCalib->setEnabled(false);
}

void FormEfficiencyCalibration::on_pushFit_clicked()
{
  QVector<SelectorItem> items = ui->spectrumSelector->items();
  std::vector<double> xx, yy;

  for (auto &fit : peak_sets_) {
    bool visible = false;

    for (auto &i : items)
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString()))
        visible = true;

    if (visible) {
      for (auto &q : fit.second.peaks()) { //bROKEN
//        if (!q.second.flagged) //BROKEN
//          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_ * fit.second.activity_scale_factor_;

        xx.push_back(x);
        yy.push_back(y);
      }

    }
  }

//  PolyLog p = PolyLog(xx, yy, ui->spinTerms->value());

  std::vector<double> sigmas(yy.size(), 1);

  PolyLog p;
  p.add_coeff(0, -50, 50, 1);
  for (int i=1; i <= ui->spinTerms->value(); ++i) {
    if (i==1)
      p.add_coeff(i, -50, 50, 1);
    else
      p.add_coeff(i, -50, 50, 0);
  }

  p.fit(xx,yy,sigmas);

  if (p.coeffs_.size()) {
    new_calibration_.type_ = "Efficiency";
    new_calibration_.bits_ = fit_data_.metadata_.bits;
    new_calibration_.coefficients_ = p.coeffs();
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "ratio";
    new_calibration_.model_ = Qpx::CalibrationModel::polylog;
    PL_DBG << "<Efficiency calibration> new calibration fit " << new_calibration_.to_string();
  }
  else
    PL_INFO << "<Efficiency calibration> Qpx::Calibration failed";

  replot_calib();
  toggle_push();
}

void FormEfficiencyCalibration::isotope_chosen() {
  update_data();
}

void FormEfficiencyCalibration::on_pushApplyCalib_clicked()
{
  update_detector_calibs();
}

void FormEfficiencyCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, settings_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormEfficiencyCalibration::on_pushCullPeaks_clicked()
{ //BROKEN
//  for (auto &q : fit_data_.peaks())
//    if (!q.second.flagged)
//      q.second.selected = true;
//  on_pushMarkerRemove_clicked();
}

void FormEfficiencyCalibration::on_doubleEpsilonE_editingFinished()
{
  update_data();
}

void FormEfficiencyCalibration::on_doubleScaleFactor_editingFinished()
{
  fit_data_.activity_scale_factor_ = ui->doubleScaleFactor->value();
  if (!fit_data_.metadata_.name.empty()) //should be ==Qpx::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;
  replot_calib();
}

void FormEfficiencyCalibration::on_doubleScaleFactor_valueChanged(double arg1)
{
  fit_data_.activity_scale_factor_ = ui->doubleScaleFactor->value();
  if (!fit_data_.metadata_.name.empty()) //should be ==Qpx::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;
  replot_calib();
}

void FormEfficiencyCalibration::on_pushFit_2_clicked()
{
  QVector<SelectorItem> items = ui->spectrumSelector->items();
  std::vector<double> xx, yy;

  for (auto &fit : peak_sets_) {
    bool visible = false;

    for (auto &i : items)
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString()))
        visible = true;

    if (visible) {
      for (auto &q : fit.second.peaks()) {
        //BROKEN

//        if (!q.second.flagged)
//          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_ * fit.second.activity_scale_factor_;

        xx.push_back(x);
        yy.push_back(y);
      }

    }
  }

//  LogInverse p = LogInverse(xx, yy, ui->spinTerms->value());

  std::vector<double> sigmas(yy.size(), 1);

  LogInverse p;
  p.add_coeff(0, -50, 50, 1);
  for (int i=1; i <= ui->spinTerms->value(); ++i) {
    if (i==1)
      p.add_coeff(i, -50, 50, 1);
    else
      p.add_coeff(i, -50, 50, 0);
  }

  p.fit(xx,yy,sigmas);

  if (p.coeffs_.size()) {
    new_calibration_.type_ = "Efficiency";
    new_calibration_.bits_ = fit_data_.metadata_.bits;
    new_calibration_.coefficients_ = p.coeffs();
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "ratio";
    new_calibration_.model_ = Qpx::CalibrationModel::loginverse;
    PL_DBG << "<Efficiency calibration> new calibration fit " << new_calibration_.to_string();
  }
  else
    PL_INFO << "<Efficiency calibration> Qpx::Calibration failed";

  replot_calib();
  toggle_push();
}

void FormEfficiencyCalibration::on_pushFitEffit_clicked()
{
  QVector<SelectorItem> items = ui->spectrumSelector->items();
  std::vector<double> xx, yy;

  for (auto &fit : peak_sets_) {
    bool visible = false;

    for (auto &i : items)
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString()))
        visible = true;

    if (visible) {
      for (auto &q : fit.second.peaks()) {

// BROKEN
//        if (!q.second.flagged)
//          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_ * fit.second.activity_scale_factor_;

        xx.push_back(x);
        yy.push_back(y);
      }

    }
  }

  Effit p = Effit(xx, yy);

    new_calibration_.type_ = "Efficiency";
    new_calibration_.bits_ = fit_data_.metadata_.bits;
    new_calibration_.coefficients_ = p.coeffs();
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "ratio";
    new_calibration_.model_ = Qpx::CalibrationModel::effit;
    PL_DBG << "<Efficiency calibration> new calibration fit " << new_calibration_.to_string();

  replot_calib();
  toggle_push();
}
