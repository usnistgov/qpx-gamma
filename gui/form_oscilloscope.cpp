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
  connect(&runner_thread_, SIGNAL(oscilReadOut(QList<QVector<double>>*)), this, SLOT(oscil_complete(QList<QVector<double>>*)));
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
}

void FormOscilloscope::on_pushOscilRefresh_clicked()
{
  emit statusText("Getting traces...");
  emit toggleIO(false);
  runner_thread_.do_oscil();
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


void FormOscilloscope::oscil_complete(QList<QVector<double>>* plot_data) {
  QVector<QColor> palette {Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow};
  ui->oscilPlot->clearGraphs();

  QVector<QColor>::iterator color_it = palette.begin();
  int i=0;
  for (QList<QVector<double>>::iterator it = ++(plot_data->begin()); it != plot_data->end(); ++it) {
    ui->oscilPlot->addGraph();
    ui->oscilPlot->graph(i)->addData(*(plot_data->begin()), *it);
    ui->oscilPlot->graph(i)->setPen(QPen(*(color_it++)));
    i++;
  }
  ui->oscilPlot->xAxis->setLabel("time (ticks)"); //can do better....
  ui->oscilPlot->yAxis->setLabel("keV");
  ui->oscilPlot->rescaleAxes();
  ui->oscilPlot->replot();

  delete plot_data;
  emit toggleIO(true);
}
