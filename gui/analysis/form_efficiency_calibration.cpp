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

FormEfficiencyCalibration::FormEfficiencyCalibration(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEfficiencyCalibration),
  detectors_(newDetDB),
  settings_(settings)
{
  ui->setupUi(this);

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

  connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));


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
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  ui->plotSpectrum->loadSettings(settings_);
  ui->isotopes->setDir(data_directory_);

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

void FormEfficiencyCalibration::setSpectrum(QString name) {
  if (!fit_data_.metadata_.name.empty()) //should be ==Gamma::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;

  fit_data_ = Gamma::Fitter();

  Qpx::Spectrum::Spectrum *spectrum = spectra_.by_name(name.toStdString());

  if (spectrum && spectrum->resolution()) {
    if (peak_sets_.count(name.toStdString())) {
      fit_data_ = peak_sets_.at(name.toStdString());
      ui->isotopes->set_current_isotope(QString::fromStdString(fit_data_.sample_name_));
    }  else {
      fit_data_.clear();
      fit_data_.setData(spectrum);
      peak_sets_[name.toStdString()] = fit_data_;
    }
    ui->doubleScaleFactor->setValue(fit_data_.activity_scale_factor_);
    ui->doubleScaleFactor->setEnabled(true);
  } else
    spectrum = nullptr;

  ui->plotSpectrum->new_spectrum();

  update_peaks(true);
}

void FormEfficiencyCalibration::update_spectrum() {
  if (this->isVisible())
    ui->plotSpectrum->new_spectrum();
}

void FormEfficiencyCalibration::update_peaks(bool contents_changed) {
  double max = 0;
  std::set<double> flagged;
  fit_data_.sample_name_ = ui->isotopes->current_isotope().toStdString();
  for (auto &q : fit_data_.peaks_) {
    q.second.flagged = false;
    for (auto &p : ui->isotopes->current_isotope_gammas()) {
      double diff = abs(q.second.energy - p.energy);
      if (diff < ui->doubleEpsilonE->value()) {
        q.second.flagged = true;
        q.second.intensity_theoretical_ = p.abundance;
        q.second.efficiency_relative_ = (q.second.cps_best_ / q.second.intensity_theoretical_);
        if (q.second.efficiency_relative_ > max)
          max = q.second.efficiency_relative_;
        flagged.insert(p.energy);
      }
    }
  }

  ui->isotopes->select_energies(flagged);

  if (max > 0) {
    for (auto &q : fit_data_.peaks_) {
      if (q.second.intensity_theoretical_ > 0)
        q.second.efficiency_relative_ = q.second.efficiency_relative_ / max;
    }
  }

  if (!fit_data_.metadata_.name.empty()) //should be ==Gamma::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;

  ui->plotSpectrum->update_fit(contents_changed);
  replot_calib();
  PL_DBG << "will rebuild table " << contents_changed;
  rebuild_table(contents_changed);
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

  Gamma::Detector modified;

  if (ret == QMessageBox::Yes) {
    if (!detectors_.has_a(Gamma::Detector(current_detector_))) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Detector name:", QLineEdit::Normal,
                                           QString::fromStdString(current_detector_),
                                           &ok);

      if (!ok)
        return;

      if (!text.isEmpty()) {
        modified = Gamma::Detector(current_detector_);
        if (detectors_.has_a(modified)) {
          QMessageBox::warning(this, "Already exists", "Detector " + text + " already exists. Will not save to database.", QMessageBox::Ok);
          modified = Gamma::Detector();
        }
      }
    } else
      modified = detectors_.get(Gamma::Detector(current_detector_));

    if (modified != Gamma::Detector())
    {
      PL_INFO << "   applying new calibrations for " << modified.name_ << " in detector database";
      modified.efficiency_calibration_ = new_calibration_;
      detectors_.replace(modified);
      emit detectorsChanged();
    }
  }
}

void FormEfficiencyCalibration::on_pushImport_clicked()
{
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load spectra", data_directory_, mca_load_formats_);

  if (fileNames.empty())
    return;

  for (int i=0; i<fileNames.size(); i++)
    if (!validateFile(this, fileNames.at(i), false))
      return;

  if ((!spectra_.empty()) && (QMessageBox::warning(this, "Append?", "Spectra already open. Append to existing?",
                                                    QMessageBox::Yes|QMessageBox::Cancel) != QMessageBox::Yes))
    return;

  //toggle_push(false, false);
  this->setCursor(Qt::WaitCursor);

  for (int i=0; i<fileNames.size(); i++) {
    PL_INFO << "Constructing spectrum from " << fileNames.at(i).toStdString();
    Qpx::Spectrum::Spectrum* newSpectrum = Qpx::Spectrum::Factory::getInstance().create_from_file(fileNames.at(i).toStdString());
    if (newSpectrum != nullptr)
      add_spectrum(newSpectrum);
    else
      PL_INFO << "Spectrum construction did not succeed. Aborting";
  }

  update_spectra();

  emit toggleIO(true);

  this->setCursor(Qt::ArrowCursor);
}

void FormEfficiencyCalibration::update_spectra() {
  QVector<SelectorItem> items;

  for (auto &q : spectra_.spectra(1, -1)) {
    Qpx::Spectrum::Metadata md;
    if (q != nullptr)
      md = q->metadata();

    SelectorItem new_spectrum;
    new_spectrum.text = QString::fromStdString(md.name);
    new_spectrum.color = QColor::fromRgba(md.appearance);
    new_spectrum.visible = md.visible;
    items.push_back(new_spectrum);
  }

  ui->spectrumSelector->setItems(items);

  ui->pushShowAll->setEnabled(ui->spectrumSelector->items().size());
  ui->pushHideAll->setEnabled(ui->spectrumSelector->items().size());

  setSpectrum(ui->spectrumSelector->selected().text);
}

void FormEfficiencyCalibration::spectrumLooksChanged(SelectorItem item) {
  replot_calib();
}

void FormEfficiencyCalibration::spectrumDetails(SelectorItem item)
{
  setSpectrum(item.text);

  QString id = ui->spectrumSelector->selected().text;
  Qpx::Spectrum::Spectrum* someSpectrum = spectra_.by_name(id.toStdString());

  Qpx::Spectrum::Metadata md;
  if (someSpectrum)
    md = someSpectrum->metadata();

  if (id.isEmpty() || (someSpectrum == nullptr)) {
    ui->pushRemove->setEnabled(false);
    ui->pushFullInfo->setEnabled(false);
    fit_data_ = Gamma::Fitter();
    ui->plotSpectrum->new_spectrum();
    ui->doubleScaleFactor->setEnabled(false);
    return;
  }

  ui->pushRemove->setEnabled(true);
  ui->pushFullInfo->setEnabled(true);
}

void FormEfficiencyCalibration::on_pushRemove_clicked()
{
  ui->doubleScaleFactor->setEnabled(false);
  std::string name = ui->spectrumSelector->selected().text.toStdString();

  spectra_.delete_spectrum(name);
  peak_sets_.erase(name);

  if (spectra_.empty()) {
    spectrumDetails(SelectorItem());
    peak_sets_.clear();
    new_calibration_ = Gamma::Calibration();
    current_detector_.clear();
    ui->labelCalibForDet->setText("Efficiency calibration for ...");
  }

  update_spectra();
}

void FormEfficiencyCalibration::on_pushShowAll_clicked()
{
  ui->spectrumSelector->show_all();
  replot_calib();
}

void FormEfficiencyCalibration::on_pushHideAll_clicked()
{
  ui->spectrumSelector->hide_all();
  replot_calib();
}


void FormEfficiencyCalibration::add_peak_to_table(const Gamma::Peak &p, int row, QColor bckg) {
  QBrush background(bckg);

  QTableWidgetItem *chn = new QTableWidgetItem( QString::number(p.center) );
  chn->setData(Qt::EditRole, QVariant::fromValue(p.center));
  chn->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, chn);

  QTableWidgetItem *nrg = new QTableWidgetItem( QString::number(p.energy) );
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);

  QTableWidgetItem *cps = new QTableWidgetItem(QString::number(p.cps_best_));
  cps->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, cps);

  QTableWidgetItem *norm = new QTableWidgetItem(QString::number(p.efficiency_relative_));
  norm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, norm);
}




void FormEfficiencyCalibration::replot_calib() {
  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();
  std::set<double> chosen_peaks_chan;

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

      for (auto &q : fit.second.peaks_) {
        if (q.second.intensity_theoretical_ == 0)
          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_;

        if (q.second.selected)
          chosen_peaks_chan.insert(q.second.center);

        xx.push_back(x);
        yy.push_back(y * fit.second.activity_scale_factor_);

        if (x < xmin)
          xmin = x;
        if (x > xmax)
          xmax = x;
      }

      style_pts.default_pen = QPen(color, 7);
      ui->PlotCalib->addPoints(xx, yy, style_pts);
    }
  }

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (have_data) {
    ui->PlotCalib->set_selected_pts(chosen_peaks_chan);
    if (new_calibration_.units_ != "channels") {
      xx.clear(); yy.clear();
      double step = (xmax-xmin) / 50.0;

      for (double i=xmin; i < xmax; i+=step) {
        xx.push_back(i);
        yy.push_back(new_calibration_.transform(i));
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
    ui->tablePeaks->setRowCount(fit_data_.peaks_.size());

    bool gray = false;
    QColor background_col;
    int i=0;
    for (auto &q : fit_data_.peaks_) {
      if (q.second.flagged && gray)
        background_col = Qt::darkGreen;
      else if (gray)
        background_col = Qt::lightGray;
      else if (q.second.flagged)
        background_col = Qt::green;
      else
        background_col = Qt::white;
      add_peak_to_table(q.second, i, background_col);
      ++i;
      if (!q.second.intersects_R)
        gray = !gray;
    }
  }

  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks_) {
    if (q.second.selected) {
      PL_DBG << "peak = " << q.second.center << " selected on row " << i;
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}


void FormEfficiencyCalibration::selection_changed_in_calib_plot() {
  std::set<double> chosen_peaks_chan = ui->PlotCalib->get_selected_pts();
  for (auto &q : fit_data_.peaks_)
    q.second.selected = (chosen_peaks_chan.count(q.second.energy) > 0);
  rebuild_table(false);
  ui->plotSpectrum->update_fit(false);
  toggle_push();
}

void FormEfficiencyCalibration::selection_changed_in_table() {
  for (auto &q : fit_data_.peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows()) {
    fit_data_.peaks_[ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
  replot_calib();
  ui->plotSpectrum->update_fit(false);
  toggle_push();
}

void FormEfficiencyCalibration::toggle_push() {
  int sel = 0; int unflagged = 0;

  for (auto &q : fit_data_.peaks_) {
    if (q.second.selected)
      sel++;
    if (!q.second.flagged)
      unflagged++;
  }

  ui->pushMarkerRemove->setEnabled(sel > 0);
  ui->pushCullPeaks->setEnabled(unflagged > 0);

  int points_for_calib = 0;
  QVector<SelectorItem> items = ui->spectrumSelector->items();
  for (auto &fit : peak_sets_) {
    bool visible = false;

    for (auto &i : items)
      if (i.visible && (fit.second.metadata_.name == i.text.toStdString()))
        visible = true;

    if (visible) {
      for (auto &q : fit.second.peaks_)
        if (q.second.flagged)
          points_for_calib++;
    }
  }

  if (points_for_calib > 1) {
    ui->pushFit->setEnabled(true);
    ui->spinTerms->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  if (new_calibration_ != Gamma::Calibration())
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
      for (auto &q : fit.second.peaks_) {
        if (!q.second.flagged)
          continue;

        double x = q.second.energy;
        double y = q.second.efficiency_relative_ * fit.second.activity_scale_factor_;

        xx.push_back(x);
        yy.push_back(y);
      }

    }
  }

  Polynomial p = Polynomial(xx, yy, ui->spinTerms->value());

  if (p.coeffs_.size()) {
    new_calibration_.type_ = "Efficiency";
    new_calibration_.bits_ = fit_data_.metadata_.bits;
    new_calibration_.coefficients_ = p.coeffs_;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "ratio";
    new_calibration_.model_ = Gamma::CalibrationModel::polynomial;
    PL_DBG << "<Efficiency calibration> new calibration fit " << new_calibration_.to_string();
  }
  else
    PL_INFO << "<Efficiency calibration> Gamma::Calibration failed";

  replot_calib();
  toggle_push();
}

void FormEfficiencyCalibration::isotope_chosen() {
  update_peaks(true);
}

void FormEfficiencyCalibration::on_pushApplyCalib_clicked()
{
  update_detector_calibs();
}

void FormEfficiencyCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, data_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormEfficiencyCalibration::on_pushMarkerRemove_clicked()
{
  std::set<double> chosen_peaks;
  double last_sel = -1;
  for (auto &q : fit_data_.peaks_)
    if (q.second.selected) {
      chosen_peaks.insert(q.second.center);
      last_sel = q.second.center;
    }

  fit_data_.remove_peaks(chosen_peaks);

  for (auto &q : fit_data_.peaks_)
    if (q.second.center > last_sel) {
      PL_DBG << "reselected " << q.second.center << " > " << last_sel;
      q.second.selected = true;
      break;
    }

  update_peaks(true);
}


void FormEfficiencyCalibration::on_pushCullPeaks_clicked()
{
  for (auto &q : fit_data_.peaks_)
    if (!q.second.flagged)
      q.second.selected = true;
  on_pushMarkerRemove_clicked();
}

void FormEfficiencyCalibration::on_doubleEpsilonE_editingFinished()
{
  update_peaks(true);
}

void FormEfficiencyCalibration::on_doubleScaleFactor_editingFinished()
{
  fit_data_.activity_scale_factor_ = ui->doubleScaleFactor->value();
  if (!fit_data_.metadata_.name.empty()) //should be ==Gamma::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;
  replot_calib();
}

void FormEfficiencyCalibration::on_doubleScaleFactor_valueChanged(double arg1)
{
  fit_data_.activity_scale_factor_ = ui->doubleScaleFactor->value();
  if (!fit_data_.metadata_.name.empty()) //should be ==Gamma::Setting()
    peak_sets_[fit_data_.metadata_.name] = fit_data_;
  replot_calib();
}

void FormEfficiencyCalibration::add_spectrum(Qpx::Spectrum::Spectrum* spectrum) {
  bool confirm = false;

  Qpx::Spectrum::Metadata md = spectrum->metadata();

  bool det_valid = ((!md.detectors.empty()) &&
                    (md.detectors.front() != Gamma::Detector()) &&
                    (!md.detectors.front().energy_calibrations_.empty()));

  std::string det_name;
  if (det_valid && (md.detectors.front().name_ != "unknown"))
    det_name = md.detectors.front().name_;

  if (!det_valid) {
    QMessageBox::warning(this, "No valid detector",
                         "No valid detector with energy calibration in "
                         + QString::fromStdString(md.name)
                         + ". Discarding spectrum.", QMessageBox::Ok);
  } else if (current_detector_.empty()) {
    if (det_name.empty()) {
      bool ok;
      QString text = QInputDialog::getText(this, "New Detector",
                                           "Invalid detector name in " + QString::fromStdString(md.name)
                                           + ". New detector name:", QLineEdit::Normal,
                                           QString::fromStdString(fit_data_.detector_.name_),
                                           &ok);
      if (ok && !text.isEmpty()) {
        current_detector_ = text.toStdString();
        confirm = true;
      }
    } else {
      current_detector_ = det_name;
      confirm = true;
    }
  } else {
    if (det_name != current_detector_) {
      std::string msg_text("Loaded spectrum '");
      msg_text += md.name + "'' has profile for detector '"+ det_name  + "'.";

      std::string question_text("Do you want to save this spectrum for calibrating '");
      question_text += current_detector_ + "'?";

      QMessageBox msgBox;
      msgBox.setText(QString::fromStdString(msg_text));
      msgBox.setInformativeText(QString::fromStdString(question_text));
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::No);
      msgBox.setIcon(QMessageBox::Question);
      int ret = msgBox.exec();

      if (ret == QMessageBox::Yes)
        confirm = true;

    } else {
      confirm = true;
    }
  }

  if (confirm) {
    ui->labelCalibForDet->setText("Efficiency calibration for " + QString::fromStdString(current_detector_));
    spectrum->set_appearance(generateColor().rgba());
    spectra_.add_spectrum(spectrum);
  } else
    delete spectrum;
}

void FormEfficiencyCalibration::on_pushFullInfo_clicked()
{
  Qpx::Spectrum::Spectrum* someSpectrum = spectra_.by_name(ui->spectrumSelector->selected().text.toStdString());
  if (someSpectrum == nullptr)
    return;

  dialog_spectrum* newSpecDia = new dialog_spectrum(*someSpectrum, this);
  connect(newSpecDia, SIGNAL(finished(bool)), this, SLOT(spectrumDetailsClosed(bool)));
  connect(newSpecDia, SIGNAL(delete_spectrum()), this, SLOT(on_pushRemove_clicked()));
  newSpecDia->exec();
}

void FormEfficiencyCalibration::spectrumDetailsClosed(bool looks_changed) {
  if (looks_changed) {
    update_spectra();
    //what if energy calibration changes?
  }
}
