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
#include "ui_form_calibration.h"
#include "poly_fit.h"
#include "qt_util.h"

FormCalibration::FormCalibration(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormCalibration),
  marker_table_(my_markers_),
  selection_model_(&marker_table_)
{
  ui->setupUi(this);

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


  moving.themes["light"] = QPen(Qt::darkRed, 2);
  moving.themes["dark"] = QPen(Qt::red, 2);

  list.themes["light"] = QPen(Qt::darkYellow, 1);
  list.themes["dark"] = QPen(Qt::yellow, 1);
  list.visible = true;

  selected.themes["light"] = QPen(Qt::darkYellow, 2);
  selected.themes["dark"] = QPen(Qt::yellow, 2);
  selected.visible = true;

  ui->tableMarkers->setModel(&marker_table_);
  ui->tableMarkers->setSelectionModel(&selection_model_);
  ui->tableMarkers->setItemDelegate(&special_delegate_);
  ui->tableMarkers->verticalHeader()->hide();
  ui->tableMarkers->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableMarkers->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableMarkers->horizontalHeader()->setStretchLastSection(true);
  ui->tableMarkers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableMarkers->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));

  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));

  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));
  connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
}

FormCalibration::~FormCalibration()
{
  delete ui;
}

void FormCalibration::closeEvent(QCloseEvent *event) {

/*  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Spectra still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  spectra_.terminate();
  plot_thread_.wait();
  saveSettings();*/
  event->accept();
}

void FormCalibration::setData(XMLableDB<Pixie::Detector>& newDetDB, QSettings &sets) {
  detectors_ = &newDetDB;

  sets.beginGroup("Program");
  data_directory_ = sets.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  sets.endGroup();

  ui->isotopes->setDir(data_directory_);
  ui->widgetDetectors->setData(*detectors_, data_directory_, load_formats_);
  toggle_radio();
}

void FormCalibration::clear() {
  my_markers_.clear();
  current_spectrum_.clear();
  new_calibration_ = Pixie::Calibration();
  old_calibration_ = Pixie::Calibration();
  detector_ = Pixie::Detector();
  x_chan.clear(); y.clear();
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  toggle_radio();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->reset_scales();
  ui->pushFromDB->setEnabled(false);
  update_plot();
  replot_markers();
}


void FormCalibration::setSpectrum(Pixie::SpectraSet *newset, QString name) {
  spectra_ = newset;
  clear();

  if (!spectra_)
    return;

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
        ui->plot1D->setTitle("Spectrum=" + current_spectrum_ + "  resolution=" + QString::number(bits) + "bits  Detector=" + QString::fromStdString(detector_.name_));
      }
    }
  }

  update_plot();
  ui->plot1D->reset_scales();
}

void FormCalibration::update_plot() {
  if (!spectra_)
    return;

  Pixie::Spectrum::Spectrum *spectrum;
  if (!(spectrum = spectra_->by_name(current_spectrum_.toStdString()))) {
    return;
  }
  if (spectrum->resolution()) {
    x_chan.resize(spectrum->resolution());
    y.resize(spectrum->resolution());
    for (std::size_t i = 0; i < spectrum->resolution(); i++) {
      x_chan[i] = i;
      y[i] = spectrum->get_count({static_cast<uint16_t>(i)});
    }
    ui->plot1D->clearGraphs();
    ui->plot1D->addGraph(x_chan, y, QColor::fromRgba(spectrum->appearance()), 0);
    ui->plot1D->setLabels("channel", "counts");
    ui->plot1D->update_plot();
  }
}

void FormCalibration::addMovingMarker(double x) {
  moving.position = x;
  moving.visible = true;
  ui->pushAdd->setEnabled(true);
  PL_INFO << "Marker at " << moving.position << " originally caliblated to " << old_calibration_.transform(moving.position)
          << ", new calibration = " << new_calibration_.transform(moving.position);
  replot_markers();
}

void FormCalibration::removeMovingMarker(double x) {
  moving.visible = false;
  ui->pushAdd->setEnabled(false);
  replot_markers();
}


void FormCalibration::replot_markers() {
  std::list<Marker> markers;

  markers.push_back(moving);

  for (auto &q : my_markers_) {
    Marker m = list;
    m.position = q.first;
    markers.push_back(m);
  }

  if (!selection_model_.selectedIndexes().empty()) {
    QModelIndex chan_ix = marker_table_.index(selection_model_.selectedRows().front().row(), 0);
    double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
    Marker sm = selected;
    sm.position = chan;
    markers.push_back(sm);
  }

  ui->plot1D->set_markers(markers);
}

void FormCalibration::on_pushAdd_clicked()
{
  my_markers_[moving.position] = old_calibration_.transform(moving.position);

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
  ui->pushPushEnergy->setEnabled(false);

  if (!selection_model_.selectedIndexes().empty()) {
    ui->pushMarkerRemove->setEnabled(true);
    if (ui->isotopes->current_gammas().size() == 1)
      ui->pushPushEnergy->setEnabled(true);
  }

  if (my_markers_.empty())
    ui->pushClear->setEnabled(false);
  else
    ui->pushClear->setEnabled(true);

  if (static_cast<int>(my_markers_.size()) >= ui->spinTerms->value())
    ui->pushFit->setEnabled(true);
  else
    ui->pushFit->setEnabled(false);

  if (detectors_->has_a(detector_) && detectors_->get(detector_).energy_calibrations_.has_a(old_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);
}

void FormCalibration::toggle_radio() {
  ui->pushApplyCalib->setEnabled(false);
  ui->radioDetSpec->setEnabled(false);
  ui->radioDetAll->setEnabled(false);
  ui->radioSpecAll->setEnabled(false);
  ui->radioDetDB->setEnabled(false);

  if (!new_calibration_.shallow_equals(Pixie::Calibration()) && !detector_.shallow_equals(Pixie::Detector())) {
    ui->pushApplyCalib->setEnabled(true);
    Pixie::Spectrum::Spectrum* spectrum;
    if (spectrum  = spectra_->by_name(current_spectrum_.toStdString())) {
      ui->radioSpecAll->setText("all detectors on " + current_spectrum_);
      ui->radioSpecAll->setEnabled(true);
      for (auto &q : spectrum->get_detectors()) {
        if (q.shallow_equals(detector_)) {
          ui->radioDetSpec->setText(QString::fromStdString(detector_.name_) + " on " + current_spectrum_);
          ui->radioDetSpec->setEnabled(true);
        }
      }
    }
    bool others_have_det = false;
    for (auto &q : spectra_->spectra()) {
      if (q && (q->name() != current_spectrum_.toStdString())) {
        for (auto &p : q->get_detectors())
          if (p.shallow_equals(detector_))
            others_have_det = true;
      }
    }
    if (others_have_det) {
      ui->radioDetAll->setText(QString::fromStdString(detector_.name_) + " on all open spectra");
      ui->radioDetAll->setEnabled(true);
    }
    if (detectors_ && detectors_->has_a(detector_)) {
      ui->radioDetDB->setText(QString::fromStdString(detector_.name_) + " in detector database");
      ui->radioDetDB->setEnabled(true);
    }
  }

}

void FormCalibration::on_pushMarkerRemove_clicked()
{
  QModelIndex chan_ix = marker_table_.index(selection_model_.selectedRows().front().row(), 0);
  double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
  my_markers_.erase(chan);
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormCalibration::on_pushFit_clicked()
{  
  std::vector<double> x, y, coefs(ui->spinTerms->value());
  x.resize(my_markers_.size());
  y.resize(my_markers_.size());
  int i = 0;
  for (auto &q : my_markers_) {
    x[i] = q.first;
    y[i] = q.second;
    i++;
  }
  if (poly_fit(x, y, coefs)) {
    new_calibration_.coefficients_ = coefs;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::local_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = 1;
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

void FormCalibration::on_pushClear_clicked()
{
  my_markers_.clear();
  selection_model_.reset();
  marker_table_.update();
  toggle_push();
  replot_markers();
}

void FormCalibration::on_pushPushEnergy_clicked()
{
  //will sum all selected
  double energy = 0.0;
  for (auto q : ui->isotopes->current_gammas())
    energy += q;
  QModelIndex chan_ix = marker_table_.index(selection_model_.selectedRows().front().row(), 0);
  double chan = marker_table_.data(chan_ix, Qt::DisplayRole).toDouble();
  if (chan)
    my_markers_[chan] = energy;
  marker_table_.update();
}

void FormCalibration::isotope_energies_chosen() {
  toggle_push();
}

void FormCalibration::on_pushApplyCalib_clicked()
{
  PL_INFO << "Apply calibration";

  if (ui->radioDetDB->isChecked() && detectors_->has_a(detector_)) {
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
      if (ui->radioDetAll->isChecked() || is_selected_spectrum) {
        std::vector<Pixie::Detector> detectors = q->get_detectors();
        for (auto &p : detectors) {
          if ((is_selected_spectrum && ui->radioSpecAll->isChecked()) ||
              (is_selected_spectrum && ui->radioDetSpec->isChecked() && p.shallow_equals(detector_)) ||
              (ui->radioDetAll->isChecked() && p.shallow_equals(detector_))) {
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
  my_markers_.clear();
  UtilXY xy(x_chan.toStdVector(), y.toStdVector());
  xy.find_peaks(ui->spinMinPeakWidth->value());
  for (auto &q : xy.peaks_)
    my_markers_[q] = old_calibration_.transform(q);
  marker_table_.update();
  toggle_push();
  replot_markers();
}
