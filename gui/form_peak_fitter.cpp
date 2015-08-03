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

FormPeakFitter::FormPeakFitter(QSettings &settings, Gamma::Fitter &fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormPeakFitter),
  fit_data_(fit),
  settings_(settings)
{
  ui->setupUi(this);

  loadSettings();

  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(7);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "energy", "fwhm", "A(net)", "A(gaussian)", "cps(net)", "cps(gaussian)"});
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
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);

  toggle_push();
}

void FormPeakFitter::update_peaks(bool contents_changed) {
  if (contents_changed) {
    ui->tablePeaks->clearContents();
    ui->tablePeaks->setRowCount(fit_data_.peaks_.size());
    bool gray = false;
    int i=0;
    for (auto &q : fit_data_.peaks_) {
      add_peak_to_table(q.second, i, gray);
      ++i;
      if (!q.second.intersects_R)
        gray = !gray;
    }
  }

  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();
  int i = 0;
  for (auto &q : fit_data_.peaks_) {
    if (q.second.selected) {
      ui->tablePeaks->selectRow(i);
    }
    ++i;
  }
  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
  
  toggle_push();
}

void FormPeakFitter::add_peak_to_table(const Gamma::Peak &p, int row, bool gray) {
  QBrush background(gray ? Qt::lightGray : Qt::white);

  QTableWidgetItem *center = new QTableWidgetItem(QString::number(p.center));
  center->setData(Qt::EditRole, QVariant::fromValue(p.center));
  center->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 0, center);

  QTableWidgetItem *nrg = new QTableWidgetItem(QString::number(p.energy));
  nrg->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 1, nrg);


  QTableWidgetItem *fwhm = new QTableWidgetItem(QString::number(p.fwhm_gaussian));
  fwhm->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 2, fwhm);

//  ui->tablePeaks->setItem(row, 3, new QTableWidgetItem( QString::number(p.fwhm_gaussian / p.fwhm_theoretical * 100) + "%" ));
  //   ui->tablePeaks->setItem(row, 4, new QTableWidgetItem( QString::number(p.x_[0]) ));
  //   ui->tablePeaks->setItem(row, 5, new QTableWidgetItem( QString::number(p.x_[p.x_.size() - 1]) ));    
//   ui->tablePeaks->setItem(row, 4, new QTableWidgetItem( QString::number(p.hwhm_R - p.hwhm_L) ));
 /* QString nbrs = "";
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
  //ui->tablePeaks->setItem(row, 3, new QTableWidgetItem( nbrs ));
  //ui->tablePeaks->setItem(row, 6, new QTableWidgetItem( QString::number(p.area_gross_) ));
  //ui->tablePeaks->setItem(row, 7, new QTableWidgetItem( QString::number(p.area_bckg_) ));

  QTableWidgetItem *area_net = new QTableWidgetItem(QString::number(p.area_net_));
  area_net->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 3, area_net);

  QTableWidgetItem *area_gauss = new QTableWidgetItem(QString::number(p.area_gauss_));
  area_gauss->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 4, area_gauss);

  QTableWidgetItem *cps_net = new QTableWidgetItem(QString::number(p.cts_per_sec_net_));
  cps_net->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 5, cps_net);

  QTableWidgetItem *cps_gauss = new QTableWidgetItem(QString::number(p.cts_per_sec_gauss_));
  cps_gauss->setData(Qt::BackgroundRole, background);
  ui->tablePeaks->setItem(row, 6, cps_gauss);
}

void FormPeakFitter::update_peak_selection(std::set<double> pks) {
  for (auto &q : fit_data_.peaks_)
    q.second.selected = (pks.count(q.second.center) > 0);
  toggle_push();
  if (isVisible())
    emit peaks_changed(false);
}

void FormPeakFitter::selection_changed_in_table() {
  for (auto &q : fit_data_.peaks_)
    q.second.selected = false;
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows()) {
    fit_data_.peaks_[ui->tablePeaks->item(i.row(), 0)->data(Qt::EditRole).toDouble()].selected = true;
  }
  toggle_push();
  if (isVisible())
    emit peaks_changed(false);
}

void FormPeakFitter::toggle_push() {
}
