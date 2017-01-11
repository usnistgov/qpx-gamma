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
 *      FormFitter -
 *
 ******************************************************************************/

#include "form_fitter.h"
#include "widget_detectors.h"
#include "ui_form_fitter.h"
#include "fitter.h"
#include "qt_util.h"
#include "qp_button.h"
#include "form_fitter_settings.h"
#include "form_peak_info.h"
#include "rollback_dialog.h"

FormFitter::FormFitter(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormFitter)
{
  ui->setupUi(this);
  player = new QMediaPlayer(this);

  connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
  connect(ui->plot, SIGNAL(range_selection_changed(double,double)),
          this, SLOT(update_range_selection(double,double)));

  connect(ui->plot, SIGNAL(add_peak(double, double)), this, SLOT(add_peak(double, double)));
  connect(ui->plot, SIGNAL(delete_selected_peaks()), this, SLOT(delete_selected_peaks()));
  connect(ui->plot, SIGNAL(adjust_sum4(double, double, double)),
          this, SLOT(adjust_sum4(double, double, double)));
  connect(ui->plot, SIGNAL(adjust_background_L(double,double,double)),
          this, SLOT(adjust_background_L(double,double,double)));
  connect(ui->plot, SIGNAL(adjust_background_R(double,double,double)),
          this, SLOT(adjust_background_R(double,double,double)));
  connect(ui->plot, SIGNAL(peak_info(double)), this, SLOT(peak_info(double)));

  connect(ui->plot, SIGNAL(rollback_ROI(double)), this, SLOT(rollback_ROI(double)));
  connect(ui->plot, SIGNAL(roi_settings(double)), this, SLOT(roi_settings(double)));
  connect(ui->plot, SIGNAL(refit_ROI(double)), this, SLOT(refit_ROI(double)));
  connect(ui->plot, SIGNAL(delete_ROI(double)), this, SLOT(delete_ROI(double)));


  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->plot);
  connect(shortcut, SIGNAL(activated()), ui->plot, SLOT(zoomOut()));

//  QShortcut *shortcut2 = new QShortcut(QKeySequence(Qt::Key_Insert), ui->plot);
//  connect(shortcut2, SIGNAL(activated()), this, SLOT(add_peak()));

  QShortcut* shortcut3 = new QShortcut(QKeySequence(QKeySequence::Delete), ui->plot);
  connect(shortcut3, SIGNAL(activated()), this, SLOT(delete_selected_peaks()));

  //  QShortcut* shortcut4 = new QShortcut(QKeySequence(QKeySequence(Qt::Key_L)), ui->plot);
  //  connect(shortcut4, SIGNAL(activated()), this, SLOT(switch_scale_type()));

  connect(&thread_fitter_, SIGNAL(fit_updated(Qpx::Fitter)), this, SLOT(fit_updated(Qpx::Fitter)));
  connect(&thread_fitter_, SIGNAL(fitting_done()), this, SLOT(fitting_complete()));

  QMovie *movie = new QMovie(":/loader.gif");
  ui->labelMovie->setMovie(movie);
  ui->labelMovie->show();
  movie->start();
  ui->labelMovie->setVisible(false);

  thread_fitter_.start();
}

FormFitter::~FormFitter()
{
  thread_fitter_.stop_work();
  thread_fitter_.terminate(); //in thread itself?

  delete ui;
}

void FormFitter::setFit(Qpx::Fitter* fit) {
  fit_data_ = fit;
  ui->plot->setFit(fit);
  update_spectrum();
  updateData();
}

void FormFitter::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  //  scale_log_ = settings_.value("scale_log", true).toBool();
  settings_.endGroup();
}

void FormFitter::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  //  settings_.setValue("scale_log", scale_log_);
  settings_.endGroup();
}

void FormFitter::clear()
{
  if (!fit_data_ || busy_)
    return;

  //DBG << "FormFitter::clear()";

  ui->plot->clearAll();

  clearSelection();
  ui->plot->replot();
  toggle_push(busy_);
}

void FormFitter::update_spectrum()
{
  ui->plot->update_spectrum();
  toggle_push(busy_);
}

void FormFitter::refit_ROI(double ROI_bin)
{
  if (!fit_data_ || busy_)
    return;

  toggle_push(true);

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.refit(ROI_bin);
}

void FormFitter::rollback_ROI(double ROI_bin)
{
  if (!fit_data_ || busy_)
    return;

  if (fit_data_->contains_region(ROI_bin))
  {
    RollbackDialog *editor = new RollbackDialog(fit_data_->region(ROI_bin), qobject_cast<QWidget *> (parent()));
    int ret = editor->exec();
    if (ret == QDialog::Accepted) {
      fit_data_->rollback_ROI(ROI_bin, editor->get_choice());
      toggle_push(false);
      updateData();

      emit data_changed();
      emit fitting_done();
    }
  }
}

void FormFitter::delete_ROI(double ROI_bin)
{
  if (!fit_data_ || busy_)
    return;

  fit_data_->delete_ROI(ROI_bin);
  toggle_push(false);
  updateData();

  emit data_changed();
  emit fitting_done();
}

void FormFitter::make_range(double energy)
{
  if (!fit_data_ || busy_)
    return;

  ui->plot->make_range_selection(energy);
  ui->plot->follow_selection();
}

void FormFitter::set_selected_peaks(std::set<double> selected_peaks)
{
  ui->plot->set_selected_peaks(selected_peaks);
  toggle_push(busy_);
}

std::set<double> FormFitter::get_selected_peaks()
{
  return ui->plot->get_selected_peaks();
}

void FormFitter::on_pushFindPeaks_clicked()
{
  perform_fit();
}

void FormFitter::perform_fit()
{
  if (!fit_data_ || busy_)
    return;

  fit_data_->find_regions();
  //  DBG << "number of peaks found " << fit_data_->peaks_.size();

  toggle_push(true);

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.fit_peaks();

}

void FormFitter::add_peak(double l, double r)
{
  if (!fit_data_ || busy_)
    return;

  ui->plot->clear_range_selection();

  toggle_push(true);

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.add_peak(fit_data_->settings().nrg_to_bin(l),
                          fit_data_->settings().nrg_to_bin(r));
}

void FormFitter::adjust_sum4(double peak_id, double l, double r)
{
  if (!fit_data_ || busy_)
    return;

  if (fit_data_->adjust_sum4(peak_id,
                             fit_data_->settings().nrg_to_bin(l),
                             fit_data_->settings().nrg_to_bin(r)))
  {
    updateData();
    std::set<double> selected_peaks;
    selected_peaks.insert(peak_id);
    ui->plot->set_selected_peaks(selected_peaks);

    emit data_changed();
    emit fitting_done();
  }
  else
    ui->plot->clear_range_selection();
}

void FormFitter::adjust_background_L(double ROI_id, double l, double r)
{
  if (!fit_data_ || busy_)
    return;

  ui->plot->clear_range_selection();

  if (!fit_data_->contains_region(ROI_id))
    return;

  std::set<double> rois = fit_data_->relevant_regions(
        fit_data_->settings().nrg_to_bin(l),
        fit_data_->region(ROI_id).right_bin());

  if (!rois.count(ROI_id))
  {
    QMessageBox::information(this, "Out of bounds", "Background sample bad. Very bad...");
    return;
  }

  bool merge = ((rois.size() > 1) &&
                (QMessageBox::question(this, "Merge?", "Regions overlap. Merge them?") == QMessageBox::Yes));

  thread_fitter_.set_data(*fit_data_);

  toggle_push(true);

  if (merge)
    thread_fitter_.merge_regions(
          fit_data_->settings().nrg_to_bin(l),
          fit_data_->region(ROI_id).right_bin());
  else
    thread_fitter_.adjust_LB(ROI_id,
                             fit_data_->settings().nrg_to_bin(l),
                             fit_data_->settings().nrg_to_bin(r));

}


void FormFitter::adjust_background_R(double ROI_id, double l, double r)
{
  if (!fit_data_ || busy_)
    return;

  ui->plot->clear_range_selection();

  if (!fit_data_->contains_region(ROI_id))
    return;

  std::set<double> rois = fit_data_->relevant_regions(
        fit_data_->region(ROI_id).left_bin(),
        fit_data_->settings().nrg_to_bin(r));

  if (!rois.count(ROI_id))
  {
    QMessageBox::information(this, "Out of bounds", "Background sample bad. Very bad...");
    return;
  }

  bool merge = ((rois.size() > 1) &&
                (QMessageBox::question(this, "Merge?", "Regions overlap. Merge them?") == QMessageBox::Yes));

  thread_fitter_.set_data(*fit_data_);

  toggle_push(true);

  if (merge)
    thread_fitter_.merge_regions(fit_data_->region(ROI_id).left_bin(),
                                 fit_data_->settings().nrg_to_bin(r));
  else
    thread_fitter_.adjust_RB(ROI_id,
                             fit_data_->settings().nrg_to_bin(l),
                             fit_data_->settings().nrg_to_bin(r));
}


void FormFitter::delete_selected_peaks()
{
  if (!fit_data_ || busy_)
    return;

  std::set<double> chosen_peaks = this->get_selected_peaks();
  if (chosen_peaks.empty())
    return;

  clearSelection();

  toggle_push(true);

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.remove_peaks(chosen_peaks);
}


void FormFitter::fit_updated(Qpx::Fitter fitter)
{
  //  while (player->state() == QMediaPlayer::PlayingState)
  //    player->stop();

  //  if (player->state() != QMediaPlayer::PlayingState) {
  //    player->setMedia(QUrl("qrc:/sounds/laser6.wav"));
  //    player->setVolume(100);
  //    player->setPosition(0);
  //    player->play();
  ////    while (player->state() == QMediaPlayer::PlayingState) {}
  //  }

  *fit_data_ = fitter;
  toggle_push(busy_);
  updateData();;
  emit data_changed();
}

void FormFitter::fitting_complete() {
  //  while (player->state() == QMediaPlayer::PlayingState)
  //    player->stop();

  //  if (player->state() != QMediaPlayer::PlayingState) {
  //    player->setMedia(QUrl("qrc:/sounds/laser12.wav"));
  //    player->setVolume(100);
  //    player->setPosition(0);
  //    player->play();
  ////    while (player->state() == QMediaPlayer::PlayingState) {}
  //  }

  //  busy_= false;
  //  calc_visible();
  //  ui->plot->replot();
  toggle_push(false);
  emit data_changed();
  emit fitting_done();
}

void FormFitter::toggle_push(bool busy)
{
  busy_ = busy;
  //  if (fit_data_ == nullptr)
  //    return;

  ui->pushStopFitter->setEnabled(busy_);
  ui->pushFindPeaks->setEnabled(!busy_);
  ui->pushSettings->setEnabled(!busy_);
  ui->labelMovie->setVisible(busy_);
  ui->plot->set_busy(busy);

  emit fitter_busy(busy_);
}

void FormFitter::on_pushStopFitter_clicked()
{
  ui->pushStopFitter->setEnabled(false);
  thread_fitter_.stop_work();
}

void FormFitter::clearSelection()
{
  ui->plot->clearSelection();
}


void FormFitter::selection_changed()
{
  toggle_push(busy_);
  emit peak_selection_changed(ui->plot->get_selected_peaks());
}

void FormFitter::update_range_selection(double l, double r)
{
  emit range_selection_changed(l, r);
}

void FormFitter::updateData()
{
  ui->plot->updateData();
}

void FormFitter::on_pushSettings_clicked()
{
  if (!fit_data_)
    return;

  FitSettings fs = fit_data_->settings();
  FormFitterSettings *FitterSettings = new FormFitterSettings(fs, this);
  FitterSettings->setWindowTitle("Fitter settings");
  int ret = FitterSettings->exec();

  if (ret == QDialog::Accepted) {
    fit_data_->apply_settings(fs);
    updateData();
  }
}

void FormFitter::roi_settings(double roi)
{
  if (!fit_data_ || !fit_data_->contains_region(roi))
    return;

  FitSettings fs = fit_data_->region(roi).fit_settings();

  FormFitterSettings *FitterSettings = new FormFitterSettings(fs, this);
  FitterSettings->setWindowTitle("Region settings");
  int ret = FitterSettings->exec();

  if (ret == QDialog::Accepted)
  {
    ui->plot->clear_range_selection();

    toggle_push(true);
    thread_fitter_.set_data(*fit_data_);
    thread_fitter_.override_ROI_settings(roi, fs);
    emit peak_selection_changed(ui->plot->get_selected_peaks());
  }

}

void FormFitter::peak_info(double bin)
{
  if (!fit_data_)
    return;

  if (!fit_data_->contains_peak(bin))
    return;

  Hypermet hm = fit_data_->peaks().at(bin).hypermet();

  FormPeakInfo *peakInfo = new FormPeakInfo(hm, this);
  peakInfo->setWindowTitle("Parameters for peak at " + QString::number(fit_data_->peaks().at(bin).energy().value()));
  int ret = peakInfo->exec();

  if ((ret == QDialog::Accepted) && fit_data_->replace_hypermet(bin, hm))
  {
    updateData();
    std::set<double> selected_peaks;
    selected_peaks.insert(bin);
    ui->plot->set_selected_peaks(selected_peaks);

    emit data_changed();
    emit fitting_done();
  }
}
