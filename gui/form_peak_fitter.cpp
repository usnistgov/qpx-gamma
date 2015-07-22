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
 *      FormPeakFitter - 
 *
 ******************************************************************************/

#include "form_peak_fitter.h"
#include "widget_detectors.h"
#include "ui_form_peak_fitter.h"
#include "gamma_fitter.h"
#include "qt_util.h"

FormPeakFitter::FormPeakFitter(QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormPeakFitter),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(6);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "fwhm", "fw/theor", "balance", "flags"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();

  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));
}

FormPeakFitter::~FormPeakFitter()
{
  delete ui;
}

bool FormPeakFitter::save_close() {
  saveSettings();
  return true;
}

void FormPeakFitter::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();

}

void FormPeakFitter::saveSettings() {

  settings_.beginGroup("peak_fitter");
  settings_.endGroup();
}

void FormPeakFitter::clear() {
  peaks_.clear();
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);

  toggle_push();
}

void FormPeakFitter::update_peaks(std::vector<Gamma::Peak> pks) {
  peaks_ = pks;

  std::sort(peaks_.begin(), peaks_.end(), Gamma::Peak::by_center);

  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(peaks_.size());
  for (int i=0; i < peaks_.size(); ++i)
    add_peak_to_table(peaks_[i], i);
  
  toggle_push();
  replot_markers();
}

void FormPeakFitter::add_peak_to_table(Gamma::Peak p, int row) {
  ui->tablePeaks->setItem(row, 0, new QTableWidgetItem( QString::number(p.center) ));
  ui->tablePeaks->setItem(row, 1, new QTableWidgetItem( QString::number(p.energy) ));
  ui->tablePeaks->setItem(row, 2, new QTableWidgetItem( QString::number(p.fwhm_gaussian) ));
  ui->tablePeaks->setItem(row, 3, new QTableWidgetItem( QString::number(p.fwhm_gaussian / p.fwhm_theoretical * 100) + "%" ));
  //   ui->tablePeaks->setItem(row, 4, new QTableWidgetItem( QString::number(p.x_[0]) ));
  //   ui->tablePeaks->setItem(row, 5, new QTableWidgetItem( QString::number(p.x_[p.x_.size() - 1]) ));    
   ui->tablePeaks->setItem(row, 4, new QTableWidgetItem( QString::number(p.hwhm_R - p.hwhm_L) ));
  QString nbrs = "";
  if (p.subpeak)
    nbrs += "M ";
  if (p.intersects_L)
    nbrs += "<";
  if (p.intersects_R)
    nbrs += " >";
  /*  if (peaks_[i].hwhm_L > 0.5 * peaks_[i].fwhm_theoretical)
      nbrs += " [";
      if (peaks_[i].hwhm_R > 0.5 * peaks_[i].fwhm_theoretical)
      nbrs += " ]";*/
  ui->tablePeaks->setItem(row, 5, new QTableWidgetItem( nbrs ));  
}

void FormPeakFitter::replot_markers() {
  std::set<double> chosen_peaks_nrg;

  ui->tablePeaks->blockSignals(true);
  ui->tablePeaks->clearSelection();
  for (int i=0; i < peaks_.size(); ++i) {
    if (peaks_[i].selected) {
      ui->tablePeaks->selectRow(i);
      chosen_peaks_nrg.insert(peaks_[i].energy);
    }
  }
  ui->tablePeaks->blockSignals(false);
}

void FormPeakFitter::update_peak_selection(std::set<double> pks) {
  for (int i=0; i < peaks_.size(); ++i)
    peaks_[i].selected = (pks.count(peaks_[i].center) > 0);
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(peaks_, false);
}

void FormPeakFitter::selection_changed_in_table() {
  for (auto &q : peaks_)
    q.selected = false;
  foreach (QModelIndex idx, ui->tablePeaks->selectionModel()->selectedRows())
    peaks_[idx.row()].selected = true;
  toggle_push();
  replot_markers();
  if (isVisible())
    emit peaks_changed(peaks_, false);
}

void FormPeakFitter::toggle_push() {
}
