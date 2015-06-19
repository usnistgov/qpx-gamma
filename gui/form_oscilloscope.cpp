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
 *      FormOsciloscope
 *
 ******************************************************************************/

#include "gui/form_oscilloscope.h"
#include "ui_form_oscilloscope.h"

FormOscilloscope::FormOscilloscope(ThreadRunner& thread, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  ui(new Ui::FormOscilloscope)
{
  ui->setupUi(this);

  ui->widgetPlot->set_scale_type("Linear");
  ui->widgetPlot->set_plot_style("Step");
  ui->widgetPlot->set_marker_labels(false);
  ui->widgetPlot->showButtonColorThemes(false);
  ui->widgetPlot->showButtonMarkerLabels(false);
  ui->widgetPlot->showButtonPlotStyle(false);
  ui->widgetPlot->showButtonScaleType(false);
  ui->widgetPlot->setZoomable(true);
  ui->widgetPlot->setTitle("Osciloscope");
  ui->widgetPlot->setLabels("time (ticks)", "energy/channel");

  connect(&runner_thread_, SIGNAL(oscilReadOut(QList<QVector<double>>*, QString)), this, SLOT(oscil_complete(QList<QVector<double>>*, QString)));
}

FormOscilloscope::~FormOscilloscope()
{
  delete ui;
}

void FormOscilloscope::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (Pixie::LiveStatus::online == live);
  ui->pushOscilRefresh->setEnabled(enable && online);
  ui->pushOscilBaselines->setEnabled(enable && online);
  ui->pushOscilOffsets->setEnabled(enable && online);
  ui->doubleSpinXDT->setEnabled(enable && online);
}

void FormOscilloscope::on_pushOscilRefresh_clicked()
{
  emit statusText("Getting traces...");
  emit toggleIO(false);
  runner_thread_.do_oscil(ui->doubleSpinXDT->value());
}

void FormOscilloscope::on_pushOscilBaselines_clicked()
{
  emit statusText("Measuring baselines...");
  emit toggleIO(false);
  runner_thread_.do_baselines();
}


void FormOscilloscope::on_pushOscilOffsets_clicked()
{
  emit statusText("Calculating offsets...");
  emit toggleIO(false);
  runner_thread_.do_offsets();
}


void FormOscilloscope::oscil_complete(QList<QVector<double>>* plot_data, QString unit) {
  ui->widgetPlot->clearGraphs();
  ui->widgetPlot->reset_scales();


  QVector<QColor> palette {Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow};

  QVector<QColor>::iterator color_it = palette.begin();
  int i=0;
  for (QList<QVector<double>>::iterator it = ++(plot_data->begin()); it != plot_data->end(); ++it) {
    ui->widgetPlot->addGraph(*(plot_data->begin()), *it, *color_it++, 1);
    i++;
  }

  ui->widgetPlot->setLabels("time (\u03BCs)", "energy (" + unit + ")");
  ui->doubleSpinXDT->setValue((*plot_data->begin())[1]);


  ui->widgetPlot->rescale();
  ui->widgetPlot->redraw();

  delete plot_data;
  emit toggleIO(true);
}

void FormOscilloscope::on_doubleSpinXDT_editingFinished()
{
  on_pushOscilRefresh_clicked();
}
