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
#include "fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>
#include <QSettings>


using namespace Qpx;

FormAnalysis2D::FormAnalysis2D(QWidget *parent)
  : QWidget(parent)
  , ui(new Ui::FormAnalysis2D)
{
  ui->setupUi(this);

  my_gates_ = new FormMultiGates();
  connect(my_gates_, SIGNAL(gate_selected()), this, SLOT(update_peaks()));
  connect(my_gates_, SIGNAL(boxes_made()), this, SLOT(take_boxes()));
  connect(my_gates_, SIGNAL(projectChanged()), this, SLOT(changedProject()));

  form_integration_ = new FormIntegration2D();
  connect(form_integration_, SIGNAL(peak_selected()), this, SLOT(update_peaks()));

  connect(ui->plotMatrix, SIGNAL(clickedPlot(double,double)),
          this, SLOT(clickedPlot2d(double,double)));
  connect(ui->plotMatrix, SIGNAL(stuff_selected()), this, SLOT(matrix_selection()));
  connect(ui->plotMatrix, SIGNAL(clearSelection()), this, SLOT(clearSelection()));

  ui->tabs->addTab(my_gates_, "Gates");
  ui->tabs->addTab(form_integration_, "Integration");
  ui->tabs->setCurrentWidget(my_gates_);

  loadSettings();
}

FormAnalysis2D::~FormAnalysis2D()
{
  delete ui;
}

void FormAnalysis2D::closeEvent(QCloseEvent *event)
{
  saveSettings();
  event->accept();
}

void FormAnalysis2D::loadSettings()
{
  QSettings settings;
  ui->plotMatrix->loadSettings(settings);

  my_gates_->loadSettings();
  form_integration_->loadSettings();
}

void FormAnalysis2D::saveSettings()
{
  QSettings settings;
  ui->plotMatrix->saveSettings(settings);

  my_gates_->saveSettings();
  form_integration_->saveSettings();
}

void FormAnalysis2D::reset()
{
  initialized = false;
}


void FormAnalysis2D::clear()
{
  ui->plotMatrix->reset_content();

  current_spectrum_ = 0;

  my_gates_->clear();
  form_integration_->clear();
  update_peaks();
}

void FormAnalysis2D::setSpectrum(ProjectPtr newset, int64_t idx)
{
  clear();
  project_ = newset;
  current_spectrum_ = idx;
  my_gates_->setSpectrum(newset, idx);
  form_integration_->setSpectrum(newset, idx);
}

void FormAnalysis2D::initialize()
{
  if (initialized || !project_)
    return;

  SinkPtr spectrum = project_->get_sink(current_spectrum_);

  if (spectrum)
  {
    ui->plotMatrix->reset_content();
    ui->plotMatrix->setSpectra(project_, current_spectrum_);
    ui->plotMatrix->update_plot();
  }
  initialized = true;
}

void FormAnalysis2D::take_boxes()
{
  form_integration_->setPeaks(my_gates_->get_all_peaks());
}

void FormAnalysis2D::matrix_selection()
{
  auto chosen_peaks = ui->plotMatrix->get_selected_boxes();

  if (my_gates_->isVisible())
    my_gates_->choose_peaks(chosen_peaks);
  else if (form_integration_->isVisible())
    form_integration_->choose_peaks(chosen_peaks);

  update_peaks();
}

void FormAnalysis2D::changedProject()
{
  emit projectChanged();
}

void FormAnalysis2D::update_spectrum()
{
  if (this->isVisible()) {
    ui->plotMatrix->update_plot();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event )
{
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks()
{
  if (my_gates_->isVisible())
    ui->plotMatrix->set_boxes(my_gates_->current_peaks());
  else if (form_integration_->isVisible())
    ui->plotMatrix->set_boxes(form_integration_->peaks());

  ui->plotMatrix->replot_markers();
}

void FormAnalysis2D::clickedPlot2d(double x, double y)
{
  if (my_gates_->isVisible())
    my_gates_->make_range(x);
  else if (form_integration_->isVisible())
    form_integration_->make_range(x, y);
}

void FormAnalysis2D::clearSelection()
{
//  ui->plotMatrix->set_range_x(Bounds2D());
//  ui->plotMatrix->replot_markers();
  if (my_gates_->isVisible())
    my_gates_->clearSelection();
  else if (form_integration_->isVisible())
    form_integration_->clearSelection();
}

void FormAnalysis2D::on_tabs_currentChanged(int index)
{
  update_peaks();
}
