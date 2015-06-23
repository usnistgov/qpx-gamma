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

FormOscilloscope::FormOscilloscope(ThreadRunner& thread, QSettings& settings, QWidget *parent) :
  QWidget(parent),
  runner_thread_(thread),
  settings_(settings),
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

  loadSettings();

  updateMenu();
  ui->toolSelectChans->setMenu(&menuDetsSelected);
  ui->toolSelectChans->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolSelectChans, SIGNAL(triggered(QAction*)), this, SLOT(chansChosen(QAction*)));

  connect(&runner_thread_, SIGNAL(oscilReadOut(QList<QVector<double>>*, QString)), this, SLOT(oscil_complete(QList<QVector<double>>*, QString)));
}

FormOscilloscope::~FormOscilloscope()
{
  delete ui;
}

void FormOscilloscope::closeEvent(QCloseEvent *event) {
  saveSettings();
  event->accept();
}


void FormOscilloscope::loadSettings() {
  settings_.beginGroup("Oscilloscope");
  ui->doubleSpinXDT->setValue(settings_.value("xdt", 0.053333).toDouble());

  det_on_.resize(Pixie::kNumChans);
  det_on_[0] = settings_.value("chan_0", true).toBool();
  det_on_[1] = settings_.value("chan_1", true).toBool();
  det_on_[2] = settings_.value("chan_2", true).toBool();
  det_on_[3] = settings_.value("chan_3", true).toBool();
  settings_.endGroup();
}

void FormOscilloscope::saveSettings() {
  settings_.beginGroup("Oscilloscope");
  settings_.setValue("xdt", ui->doubleSpinXDT->value());
  settings_.setValue("chan_0", QVariant::fromValue(det_on_[0]));
  settings_.setValue("chan_1", QVariant::fromValue(det_on_[1]));
  settings_.setValue("chan_2", QVariant::fromValue(det_on_[2]));
  settings_.setValue("chan_3", QVariant::fromValue(det_on_[3]));
  settings_.endGroup();
}

void FormOscilloscope::updateMenu() {
  menuDetsSelected.clear();
  std::vector<Pixie::Detector> dets = Pixie::Wrapper::getInstance().settings().get_detectors();
  if (dets.size() != det_on_.size())
    det_on_.resize(dets.size());
  for (int i=0; i < dets.size(); ++i)
    menuDetsSelected.addAction(QString::fromStdString(dets[i].name_) + " (chan " + QString::number(i) + ")");
  int i = 0;
  for (auto &q : menuDetsSelected.actions()) {
    q->setCheckable(true);
    q->setChecked(det_on_[i]);
    q->setData(QVariant::fromValue(i));
    ++i;
  }
}

double FormOscilloscope::xdt() {
  return ui->doubleSpinXDT->value();
}


void FormOscilloscope::chansChosen(QAction* act) {
  int choice = act->data().toInt();
  det_on_[choice] = !det_on_[choice];
  act->setChecked(det_on_[choice]);
  on_pushOscilRefresh_clicked();
}

void FormOscilloscope::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (Pixie::LiveStatus::online == live);
  ui->pushOscilRefresh->setEnabled(enable && online);
  ui->pushOscilBaselines->setEnabled(enable && online);
  ui->pushOscilOffsets->setEnabled(enable && online);
  ui->doubleSpinXDT->setEnabled(enable && online);
  ui->toolSelectChans->setEnabled(enable && online);
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

  std::map<double, double> minima, maxima;

  QVector<QColor> palette {Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::darkCyan};
  QVector<double> &xx = *(plot_data->begin());

  int i=0;
  for (QList<QVector<double>>::iterator it = ++(plot_data->begin()); it != plot_data->end(); ++it) {
    if (det_on_[i]) {
      QVector<double> &yy = *it;
      ui->widgetPlot->addGraph(xx, yy, palette[i % palette.size()], 1);

      for (int i=0; i < xx.size(); i++) {
        if (!minima.count(xx[i]) || (minima[xx[i]] > yy[i]))
          minima[xx[i]] = yy[i];
        if (!maxima.count(xx[i]) || (maxima[xx[i]] < yy[i]))
          maxima[xx[i]] = yy[i];
      }
    }
    i++;
  }

  ui->widgetPlot->setLabels("time (\u03BCs)", "energy (" + unit + ")");
  ui->widgetPlot->setYBounds(minima, maxima);

  ui->doubleSpinXDT->setValue((*plot_data->begin())[1]);


  ui->widgetPlot->rescale();
  ui->widgetPlot->redraw();

  delete plot_data;
  updateMenu();

  emit toggleIO(true);
}

void FormOscilloscope::on_doubleSpinXDT_editingFinished()
{
  on_pushOscilRefresh_clicked();
}
