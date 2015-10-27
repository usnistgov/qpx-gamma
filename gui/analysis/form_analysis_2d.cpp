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
#include "ui_form_analysis_2d.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>


FormAnalysis2D::FormAnalysis2D(QSettings &settings, XMLableDB<Gamma::Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis2D),
  detectors_(newDetDB),
  settings_(settings),
  my_gates_(nullptr)
{
  ui->setupUi(this);

  initialized = false;


  my_gates_ = new FormMultiGates(settings_, this);
  ui->tabs->addTab(my_gates_, "Gates");
  //my_gates_->hide();
  //my_gates_->blockSignals(true);
  connect(my_gates_, SIGNAL(gate_selected()), this, SLOT(remake_gate()));
  connect(my_gates_, SIGNAL(boxes_made()), this, SLOT(take_boxes()));
  //connect(my_gates_, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));
  connect(my_gates_, SIGNAL(range_changed(MarkerBox2D)), this, SLOT(update_range(MarkerBox2D)));

  connect(ui->plotMatrix, SIGNAL(markers_set(Marker,Marker)), this, SLOT(update_gates(Marker,Marker)));
  connect(ui->plotMatrix, SIGNAL(stuff_selected()), this, SLOT(matrix_selection()));

  loadSettings();


  res = 0;
  xmin_ = 0;
  xmax_ = 0;
  ymin_ = 0;
  ymax_ = 0;
  xc_ = -1;
  yc_ = -1;

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

}

void FormAnalysis2D::take_boxes() {
  coincidences_ = my_gates_->boxes();
  show_all_coincidences();
}

void FormAnalysis2D::show_all_coincidences() {
  ui->plotMatrix->set_boxes(coincidences_);
  ui->plotMatrix->replot_markers();
}


void FormAnalysis2D::update_range(MarkerBox2D range) {
  range2d = range;
  update_peaks(false);
}


void FormAnalysis2D::matrix_selection() {
  PL_DBG << "User selected peaks in matrix";

  std::list<MarkerBox2D> chosen_peaks = ui->plotMatrix->get_selected_boxes();

  for (auto &q : chosen_peaks)
    for (auto &p : coincidences_)
      p.selected = (p == q);
  my_gates_->choose_peaks(chosen_peaks);
  update_peaks(false);
}


void FormAnalysis2D::remake_gate() {
  if (!my_gates_)
    return;

  Gamma::Gate gate = my_gates_->current_gate();
  PL_DBG << "remake gate c=" << gate.centroid_chan << " w=" << gate.width_chan;

  yc_ = xc_ = gate.centroid_chan;
  double xmin = 0;
  double xmax = res - 1;
  double ymin = 0;
  double ymax = res - 1;

  Marker xx, yy;
  double width = gate.width_chan;

  if (gate.centroid_chan == -1) {
    xx.visible = false;
    yy.visible = false;

    //ui->plotMatrix->

  } else {
    xmin = 0;
    xmax = xmax = res - 1;
    ymin = yc_ - (width / 2); if (ymin < 0) ymin = 0;
    ymax = yc_ + (width / 2); if (ymax >= res) ymax = res - 1;

    xx.pos.set_bin(res / 2, gate.fit_data_.metadata_.bits, gate.fit_data_.nrg_cali_);
    yy.pos.set_bin(yc_, gate.fit_data_.metadata_.bits, gate.fit_data_.nrg_cali_);

    xx.visible = false;
    yy.visible = true;
  }


  //ui->plotMatrix->set_boxes(boxes);

  xmin_ = xmin; xmax_ = xmax;
  ymin_ = ymin; ymax_ = ymax;
  ui->plotMatrix->set_gate_width(static_cast<uint16_t>(gate.width_chan));
  ui->plotMatrix->set_markers(xx, yy);
  update_peaks(true);
}

void FormAnalysis2D::configure_UI() {
  //while (ui->tabs->count())
//    ui->tabs->removeTab(0);

//  ui->plotSpectrum->setVisible(visible1);
//  ui->plotSpectrum->blockSignals(!visible1);

  ui->plotMatrix->set_gates_movable(false);

  ui->plotMatrix->set_gates_visible(false, coincidences_.empty(), false);

  ui->plotMatrix->set_show_boxes(true);

  if (!coincidences_.empty()) {
    ui->plotMatrix->set_markers(Marker(), Marker());
    show_all_coincidences();
  }
}

void FormAnalysis2D::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
  my_gates_->setSpectrum(newset, name);
}

void FormAnalysis2D::reset() {
  initialized = false;
//  while (ui->tabs->count())
//    ui->tabs->removeTab(0);
}

void FormAnalysis2D::initialize() {
  if (initialized)
    return;

  if (spectra_) {

    PL_DBG << "<Analysis2D> initializing to " << current_spectrum_.toStdString();
    Qpx::Spectrum::Spectrum *spectrum = spectra_->by_name(current_spectrum_.toStdString());

    if (spectrum && spectrum->resolution()) {
      Qpx::Spectrum::Metadata md = spectrum->metadata();
      res = md.resolution;

      xc_ = -1;
      yc_ = -1;

      xmin_ = 0;
      xmax_ = res - 1;
      ymin_ = 0;
      ymax_ = res - 1;

      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_, current_spectrum_);
      ui->plotMatrix->update_plot();

      //need better criterion
      bool symmetrized = (md.attributes.get(Gamma::Setting("symmetrized")).value_int != 0);
    }
  }

  initialized = true;
  configure_UI();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {

    ui->plotMatrix->update_plot();
    //ui->plotSpectrum->update_spectrum();
    //ui->plotSpectrum2->update_spectrum();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks(bool content_changed) {
  //update sums

  if (my_gates_) {

    //if (yc_ >= 0) {
    ui->plotMatrix->set_boxes(my_gates_->current_peaks());
    ui->plotMatrix->set_range_x(range2d);
    ui->plotMatrix->replot_markers();
    //}

    //if (yc_ >= 0)
//      my_gates_->change_width(ui->plotMatrix->gate_width());
//    my_gates_->update_current_gate(gate);
  }
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  if (coincidences_.empty()) {
    if (ui->plotMatrix->gate_width() != my_gates_->current_gate().width_chan)
      my_gates_->change_width(ui->plotMatrix->gate_width());
    else
      my_gates_->make_range(xx);
  } else {
    PL_DBG << "make 2D range, unimplemented";
  }
}

void FormAnalysis2D::closeEvent(QCloseEvent *event) {
//  ui->plotSpectrum->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormAnalysis2D::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

//  ui->plotSpectrum->loadSettings(settings_);

  settings_.beginGroup("AnalysisMatrix");
  ui->plotMatrix->set_gradient(settings_.value("gradient", "hot").toString());
  ui->plotMatrix->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plotMatrix->set_show_legend(settings_.value("show_legend", false).toBool());
  ui->plotMatrix->set_gate_width(settings_.value("gate_width", 10).toInt());
  settings_.endGroup();

}

void FormAnalysis2D::saveSettings() {
//  ui->plotSpectrum->saveSettings(settings_);

  if (my_gates_)
    my_gates_->saveSettings();

  settings_.beginGroup("AnalysisMatrix");
  settings_.setValue("gradient", ui->plotMatrix->gradient());
  settings_.setValue("scale_type", ui->plotMatrix->scale_type());
  settings_.setValue("show_legend", ui->plotMatrix->show_legend());
  settings_.setValue("gate_width", ui->plotMatrix->gate_width());
  settings_.endGroup();
}

void FormAnalysis2D::clear() {
  res = 0;
  xmin_ = ymin_ = 0;
  xmax_ = ymax_ = 0;

  xc_ = -1;
  yc_ = -1;

  ui->plotMatrix->reset_content();

  current_spectrum_.clear();

  if (my_gates_->isVisible())
    my_gates_->clear();
}

FormAnalysis2D::~FormAnalysis2D()
{
  delete ui;
}
