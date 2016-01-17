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
  my_gates_(nullptr),
  form_integration_(nullptr)
{
  ui->setupUi(this);

  initialized = false;

  my_gates_ = new FormMultiGates(settings_);
  connect(my_gates_, SIGNAL(gate_selected()), this, SLOT(update_peaks()));
  connect(my_gates_, SIGNAL(boxes_made()), this, SLOT(take_boxes()));
  connect(my_gates_, SIGNAL(range_changed(MarkerBox2D)), this, SLOT(update_range(MarkerBox2D)));

  form_integration_ = new FormIntegration2D(settings_);
  connect(form_integration_, SIGNAL(peak_selected()), this, SLOT(update_peaks()));
  connect(form_integration_, SIGNAL(range_changed(MarkerBox2D)), this, SLOT(update_range(MarkerBox2D)));

  connect(ui->plotMatrix, SIGNAL(markers_set(Marker,Marker)), this, SLOT(update_gates(Marker,Marker)));
  connect(ui->plotMatrix, SIGNAL(stuff_selected()), this, SLOT(matrix_selection()));

  ui->tabs->addTab(my_gates_, "Gates");
  ui->tabs->addTab(form_integration_, "Integration");
  ui->tabs->setCurrentWidget(my_gates_);

  loadSettings();


  res = 0;

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

}

void FormAnalysis2D::take_boxes() {
  form_integration_->setPeaks(my_gates_->boxes());
}


void FormAnalysis2D::update_range(MarkerBox2D range) {
  range2d = range;
  update_peaks();
}


void FormAnalysis2D::matrix_selection() {
  //PL_DBG << "User selected peaks in matrix";

  std::list<MarkerBox2D> chosen_peaks = ui->plotMatrix->get_selected_boxes();

  if (my_gates_->isVisible())
    my_gates_->choose_peaks(chosen_peaks);
  else if (form_integration_->isVisible())
    form_integration_->choose_peaks(chosen_peaks);

  update_peaks();
}


void FormAnalysis2D::configure_UI() {
  //while (ui->tabs->count())
//    ui->tabs->removeTab(0);

//  ui->plotSpectrum->setVisible(visible1);
//  ui->plotSpectrum->blockSignals(!visible1);

  ui->plotMatrix->set_gates_movable(false);

  ui->plotMatrix->set_gates_visible(false, true, false);

  ui->plotMatrix->set_show_boxes(true);


}

void FormAnalysis2D::setSpectrum(Qpx::SpectraSet *newset, QString name) {
  clear();
  spectra_ = newset;
  current_spectrum_ = name;
  my_gates_->setSpectrum(newset, name);
  form_integration_->setSpectrum(newset, name);
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

      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_, current_spectrum_);
      ui->plotMatrix->update_plot();

      bool symmetrized = (md.attributes.get(Gamma::Setting("symmetrized")).value_int != 0);
    }
  }

  initialized = true;
  configure_UI();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    ui->plotMatrix->update_plot();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks() {
  Marker xx, yy;
  xx.visible = false;
  yy.visible = false;

  if (my_gates_->isVisible()) {
    Gamma::Gate gate = my_gates_->current_gate();
    //PL_DBG << "remake gate c=" << gate.centroid_chan << " w=" << gate.width_chan;

    if (gate.centroid_chan != -1) {
        xx.pos.set_bin(res / 2, gate.fit_data_.metadata_.bits, gate.fit_data_.nrg_cali_);
      yy.pos.set_bin(gate.centroid_chan, gate.fit_data_.metadata_.bits, gate.fit_data_.nrg_cali_);
      yy.visible = true;
    }

    ui->plotMatrix->set_gate_width(static_cast<uint16_t>(gate.width_chan));
    ui->plotMatrix->set_boxes(my_gates_->current_peaks());
  } else if (form_integration_->isVisible()) {
    ui->plotMatrix->set_boxes(form_integration_->peaks());
//    range2d = MarkerBox2D();
  }

  ui->plotMatrix->set_range_x(range2d);
  ui->plotMatrix->set_markers(xx, yy);
  ui->plotMatrix->replot_markers();
}

void FormAnalysis2D::update_gates(Marker xx, Marker yy) {
  //PL_DBG << "FormAnalysis2D::update_gates";
  if (my_gates_->isVisible()) {
    if (ui->plotMatrix->gate_width() != my_gates_->current_gate().width_chan)
      my_gates_->change_width(ui->plotMatrix->gate_width());
    else
      my_gates_->make_range(xx);
  } else if (form_integration_->isVisible()) {
    if (ui->plotMatrix->gate_width() != my_gates_->current_gate().width_chan)
      PL_DBG << "spin change on peaks, unimplemented";
    else
      form_integration_->make_range(xx, yy);
  }
}

void FormAnalysis2D::closeEvent(QCloseEvent *event) {
//  ui->plotSpectrum->saveSettings(settings_);

  saveSettings();
  event->accept();
}

void FormAnalysis2D::loadSettings() {
  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  settings_.beginGroup("AnalysisMatrix");
  ui->plotMatrix->set_gradient(settings_.value("gradient", "Hot").toString());
  ui->plotMatrix->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plotMatrix->set_show_legend(settings_.value("show_legend", false).toBool());
  ui->plotMatrix->set_gate_width(settings_.value("gate_width", 10).toInt());
  settings_.endGroup();

  if (my_gates_)
    my_gates_->loadSettings();
  if (form_integration_)
    form_integration_->loadSettings();
}

void FormAnalysis2D::saveSettings() {
  if (my_gates_)
    my_gates_->saveSettings();
  if (form_integration_)
    form_integration_->saveSettings();

  settings_.beginGroup("AnalysisMatrix");
  settings_.setValue("gradient", ui->plotMatrix->gradient());
  settings_.setValue("scale_type", ui->plotMatrix->scale_type());
  settings_.setValue("show_legend", ui->plotMatrix->show_legend());
  settings_.setValue("gate_width", ui->plotMatrix->gate_width());
  settings_.endGroup();
}

void FormAnalysis2D::clear() {
  res = 0;

  ui->plotMatrix->reset_content();

  current_spectrum_.clear();

  if (my_gates_)
    my_gates_->clear();
  if (form_integration_)
    form_integration_->clear();
}

FormAnalysis2D::~FormAnalysis2D()
{
  delete ui;
}

void FormAnalysis2D::on_tabs_currentChanged(int index)
{
  update_peaks();
}
