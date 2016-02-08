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


RollbackDialog::RollbackDialog(Gamma::ROI roi, QWidget *parent) :
  QDialog(parent),
  roi_(roi)
{
  QLabel *label;
  QFrame* line;

  QVBoxLayout *vl_fit    = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Fit");
  vl_fit->addWidget(label);

  QVBoxLayout *vl_peaks = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("# of peaks");
  vl_peaks->addWidget(label);

  QVBoxLayout *vl_rsq = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("r-squared");
  vl_rsq->addWidget(label);

  for (int i=0; i < roi_.fits_.size(); ++i) {

    QRadioButton *radio = new QRadioButton();
    radio->setLayoutDirection(Qt::RightToLeft);
    radio->setText(QString::number(i));
    radio->setFixedHeight(25);
    bool selected = false;
    if (!roi_.fits_[i].peaks_.empty() && !roi_.peaks_.empty())
      selected = (roi_.fits_[i].peaks_.begin()->second.hypermet_.rsq_ == roi_.peaks_.begin()->second.hypermet_.rsq_);
    radio->setChecked(selected);
    radios_.push_back(radio);
    vl_fit->addWidget(radio);

    label = new QLabel();
    label->setFixedHeight(25);
    label->setText(QString::number(roi_.fits_[i].peaks_.size()));
    vl_peaks->addWidget(label);

    label = new QLabel();
    label->setFixedHeight(25);
    double rsq = 0;
    if (!roi_.fits_[i].peaks_.empty())
      rsq = roi_.fits_[i].peaks_.begin()->second.hypermet_.rsq_;
    label->setText(QString::number(rsq));
    vl_rsq->addWidget(label);

  }

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(vl_fit);
  hl->addLayout(vl_peaks);
  hl->addLayout(vl_rsq);

  label = new QLabel();
  label->setText(QString::fromStdString("<b>ROI at chan=" + std::to_string(roi_.hr_x_nrg.front()) + " rollback to</b>"));

  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setFixedHeight(3);
  line->setLineWidth(1);

  QVBoxLayout *total    = new QVBoxLayout();
  total->addWidget(label);
  total->addWidget(line);
  total->addLayout(hl);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  total->addWidget(buttonBox);

  setLayout(total);
}

int RollbackDialog::get_choice() {
  int ret = 0;
  for (int i=0; i < radios_.size(); ++i)
    if (radios_[i]->isChecked())
      ret = i;
  return ret;
}


FormPeaks::FormPeaks(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormPeaks)
{
  ui->setupUi(this);

  connect(ui->plot1D, SIGNAL(markers_selected()), this, SLOT(user_selected_peaks()));
  connect(ui->plot1D, SIGNAL(clickedLeft(double)), this, SLOT(addRange(double)));
  connect(ui->plot1D, SIGNAL(clickedRight(double)), this, SLOT(removeRange(double)));
  connect(ui->plot1D, SIGNAL(range_moved(double, double)), this, SLOT(range_moved(double, double)));

  connect(ui->plot1D, SIGNAL(refit_ROI(double)), this, SLOT(refit_ROI(double)));
  connect(ui->plot1D, SIGNAL(rollback_ROI(double)), this, SLOT(rollback_ROI(double)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Insert), ui->plot1D);
  connect(shortcut, SIGNAL(activated()), this, SLOT(on_pushAdd_clicked()));

  QShortcut* shortcut2 = new QShortcut(QKeySequence(QKeySequence::Delete), ui->plot1D);
  connect(shortcut2, SIGNAL(activated()), this, SLOT(on_pushRemovePeaks_clicked()));


  connect(&thread_fitter_, SIGNAL(fit_updated(Gamma::Fitter)), this, SLOT(fit_updated(Gamma::Fitter)));
  connect(&thread_fitter_, SIGNAL(fitting_done()), this, SLOT(fitting_complete()));

  busy_ = false;
  thread_fitter_.start();
}

FormPeaks::~FormPeaks()
{
  thread_fitter_.stop_work();
  thread_fitter_.terminate(); //in thread itself?

  delete ui;
}

void FormPeaks::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  ui->plot1D->setData(fit_data_);
  update_spectrum();
  replot_all();
}

void FormPeaks::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  ui->spinSqWidth->setValue(settings_.value("square_width", 3).toInt());
  ui->spinSum4EdgeW->setValue(settings_.value("sum4_edge_width", 3).toInt());
  ui->doubleOverlapWidth->setValue(settings_.value("overlap_width", 2.70).toDouble());
  ui->doubleThresh->setValue(settings_.value("peak_threshold", 3.0).toDouble());
  ui->plot1D->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  settings_.endGroup();
}

void FormPeaks::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("square_width", ui->spinSqWidth->value());
  settings_.setValue("sum4_edge_width", ui->spinSum4EdgeW->value());
  settings_.setValue("peak_threshold", ui->doubleThresh->value());
  settings_.setValue("overlap_width", ui->doubleOverlapWidth->value());
  settings_.setValue("scale_type", ui->plot1D->scale_type());
  settings_.endGroup();
}

void FormPeaks::clear() {
  //PL_DBG << "FormPeaks::clear()";
  toggle_push();
  ui->plot1D->setTitle("");
  ui->plot1D->clearGraphs();
  ui->plot1D->reset_scales();
  ui->plot1D->clearSelection();
  selected_peaks_.clear();
  replot_all();
}


void FormPeaks::tighten() {
  ui->plot1D->tight_x();
}

void FormPeaks::update_spectrum(QString title) {
  if (fit_data_ == nullptr)
    return;

  if (fit_data_->metadata_.resolution == 0) {
    clear();
    replot_all();
    return;
  }

  if (title.isEmpty())
    title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  Detector=" + QString::fromStdString(fit_data_->detector_.name_);
  ui->plot1D->setTitle(title);
  ui->plot1D->reset_scales();

  if (fit_data_->peaks().empty())
    fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());

  replot_all();
  toggle_push();
}


void FormPeaks::replot_all() {
  if (fit_data_ == nullptr)
    return;

  //PL_DBG << "FormPeaks::replot_all()";

  ui->plot1D->blockSignals(true);
  this->blockSignals(true);
  
  ui->plot1D->clearGraphs();
  ui->plot1D->updateData();
  ui->plot1D->redraw();

  ui->plot1D->blockSignals(false);
  this->blockSignals(false);
}

void FormPeaks::addRange(double x) {
  if (fit_data_ == nullptr)
    return;

  Coord c;

  if (fit_data_->nrg_cali_.valid())
    c.set_energy(x, fit_data_->nrg_cali_);
  else
    c.set_bin(x, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  createRange(c);
}

void FormPeaks::refit_ROI(double ROI_nrg) {
  busy_= true;
  toggle_push();

  thread_fitter_.refit(ROI_nrg);
}


void FormPeaks::rollback_ROI(double ROI_nrg) {
  for (auto &r : fit_data_->regions_) {
    if (!r.hr_x_nrg.empty() && (r.hr_x_nrg.front() == ROI_nrg)) {
      RollbackDialog *editor = new RollbackDialog(r, qobject_cast<QWidget *> (parent()));
      int ret = editor->exec();
      if (ret == QDialog::Accepted) {
        r.rollback(editor->get_choice());
        fit_data_->remap_region(r);
        toggle_push();
        replot_all();
      }
    }
  }

}

void FormPeaks::removeRange(double dummy) {
  range_.visible = false;
  ui->plot1D->set_range(range_);
  ui->plot1D->clearSelection();
  toggle_push();
  emit range_changed(range_);
  emit selection_changed(ui->plot1D->get_selected_peaks());
}

void FormPeaks::make_range(Marker marker) {
  if (marker.visible)
    createRange(marker.pos);
  else
    removeRange(0);
}

void FormPeaks::createRange(Coord c) {
  if (fit_data_ == nullptr)
    return;
  
  range_.visible = true;

  double x = c.bin(fit_data_->metadata_.bits);

  uint16_t ch = static_cast<uint16_t>(x);
  uint16_t ch_l = fit_data_->finder_.find_left(ch);
  uint16_t ch_r = fit_data_->finder_.find_right(ch);

  for (auto &q : fit_data_->regions_) {
    if (q.overlaps(x)) {
      ch_l = q.finder_.find_left(ch);
      ch_r = q.finder_.find_right(ch);
    }
  }

  range_.center.set_bin(x, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  range_.l.set_bin(ch_l, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  range_.r.set_bin(ch_r, fit_data_->metadata_.bits, fit_data_->nrg_cali_);

  ui->plot1D->clearSelection();
  ui->plot1D->set_range(range_);
  toggle_push();

  emit selection_changed(ui->plot1D->get_selected_peaks());
  emit range_changed(range_);
}

void FormPeaks::range_moved(double l, double r) {

  //PL_DBG << "<peaks> range moved";

  if (r < l) {
    double t = l;
    l = r;
    r = t;
  }

  //double c = (r + l) / 2.0;

  if (fit_data_->nrg_cali_.valid()) {
    range_.l.set_energy(l, fit_data_->nrg_cali_);
    range_.r.set_energy(r, fit_data_->nrg_cali_);
    //range_.center.set_energy(c, fit_data_->nrg_cali_);
  } else {
    range_.l.set_bin(l, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    range_.r.set_bin(r, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
    //range_.center.set_bin(c, fit_data_->metadata_.bits, fit_data_->nrg_cali_);
  }

  ui->plot1D->set_range(range_);
  toggle_push();

  emit range_changed(range_);
}

void FormPeaks::user_selected_peaks() {

  if (fit_data_ == nullptr)
    return;

  std::set<double> selected_peaks = ui->plot1D->get_selected_peaks();
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    range_.visible = false;
    ui->plot1D->set_range(range_);
    ui->plot1D->redraw();
    toggle_push();
    if (isVisible()) {
      emit selection_changed(selected_peaks_);
      emit range_changed(range_);
    }
  }
}

void FormPeaks::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    range_.visible = false;
    ui->plot1D->set_range(range_);
    ui->plot1D->select_peaks(selected_peaks);
    ui->plot1D->redraw();
  }
  toggle_push();
}

std::set<double> FormPeaks::get_selected_peaks() {
  selected_peaks_ = ui->plot1D->get_selected_peaks();
  return selected_peaks_;
}

void FormPeaks::on_pushFindPeaks_clicked()
{
  busy_= true;
  toggle_push();
  perform_fit();

//  emit peaks_changed(true);
}


void FormPeaks::perform_fit() {
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(),ui->doubleThresh->value());
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
  fit_data_->sum4edge_samples = ui->spinSum4EdgeW->value();
  fit_data_->find_regions();
  //  PL_DBG << "number of peaks found " << fit_data_->peaks_.size();

  busy_= true;
  toggle_push();
  replot_all();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.fit_peaks();

}

void FormPeaks::on_pushAdd_clicked()
{
  if (!ui->pushAdd->isEnabled())
    return;

  if (range_.l.energy() >= range_.r.energy())
    return;

  removeRange(0);
  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.add_peak(range_.l.bin(fit_data_->metadata_.bits), range_.r.bin(fit_data_->metadata_.bits));
}

void FormPeaks::on_pushRemovePeaks_clicked()
{
  if (fit_data_ == nullptr)
    return;

  std::set<double> chosen_peaks = ui->plot1D->get_selected_peaks();
  if (chosen_peaks.empty())
    return;

  ui->plot1D->clearSelection();

  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.remove_peaks(chosen_peaks);
}

void FormPeaks::replace_peaks(std::vector<Gamma::Peak> pks) {
  if (pks.empty())
    return;

  if (fit_data_ == nullptr)
    return;

  for (auto &p : pks)
    fit_data_->replace_peak(p);

  replot_all();
  emit data_changed();
}

void FormPeaks::fit_updated(Gamma::Fitter fitter) {
  *fit_data_ = fitter;
  toggle_push();
  replot_all();
  emit data_changed();
}

void FormPeaks::fitting_complete() {
  busy_= false;
  toggle_push();
  emit data_changed();
}

void FormPeaks::toggle_push() {
  if (fit_data_ == nullptr)
    return;

  ui->pushStopFitter->setEnabled(busy_);
  ui->pushFindPeaks->setEnabled(!busy_);
  ui->spinSqWidth->setEnabled(!busy_);
  ui->spinSum4EdgeW->setEnabled(!busy_);
  ui->doubleThresh->setEnabled(!busy_);
  ui->doubleOverlapWidth->setEnabled(!busy_);

  ui->pushAdd->setEnabled(false);
  ui->pushRemovePeaks->setEnabled(false);

  if (busy_)
    return;

  ui->pushAdd->setEnabled(range_.visible);
  ui->pushRemovePeaks->setEnabled(!ui->plot1D->get_selected_peaks().empty());
}

void FormPeaks::on_spinSqWidth_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());
  replot_all();
}

void FormPeaks::on_spinSum4EdgeW_editingFinished()
{
  fit_data_->sum4edge_samples = ui->spinSum4EdgeW->value();
}

void FormPeaks::on_doubleOverlapWidth_editingFinished()
{
  fit_data_->overlap_ = ui->doubleOverlapWidth->value();
}


void FormPeaks::on_doubleThresh_editingFinished()
{
  if (fit_data_ == nullptr)
    return;

  fit_data_->finder_.find_peaks(ui->spinSqWidth->value(), ui->doubleThresh->value());
  replot_all();
}

void FormPeaks::on_pushStopFitter_clicked()
{
  ui->pushStopFitter->setEnabled(false);
  thread_fitter_.stop_work();
}
