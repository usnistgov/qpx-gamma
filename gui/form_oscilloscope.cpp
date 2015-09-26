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

#include "form_oscilloscope.h"
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
  ui->widgetPlot->setTitle("Oscilloscope");
  ui->widgetPlot->setLabels("time (ticks)", "energy/channel");

  loadSettings();

  ui->toolSelectChans->setMenu(&menuDetsSelected);
  ui->toolSelectChans->setPopupMode(QToolButton::InstantPopup);
  connect(ui->toolSelectChans, SIGNAL(triggered(QAction*)), this, SLOT(chansChosen(QAction*)));

  connect(&runner_thread_, SIGNAL(oscilReadOut(std::vector<Qpx::Trace>)), this, SLOT(oscil_complete(std::vector<Qpx::Trace>)));
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
  settings_.endGroup();
}

void FormOscilloscope::saveSettings() {
  settings_.beginGroup("Oscilloscope");
  settings_.setValue("xdt", ui->doubleSpinXDT->value());
  settings_.endGroup();
}

void FormOscilloscope::updateMenu(std::vector<Gamma::Detector> dets) {
  menuDetsSelected.clear();
  if (dets.size() < det_on_.size())
    det_on_.resize(dets.size());
  for (int i=0; i < dets.size(); ++i) {
    menuDetsSelected.addAction(QString::fromStdString(dets[i].name_) + " (chan " + QString::number(i) + ")");
    if (i >= det_on_.size())
      det_on_.push_back(1);
  }
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

void FormOscilloscope::toggle_push(bool enable, Qpx::LiveStatus live) {
  bool online = (Qpx::LiveStatus::online == live);
  ui->pushOscilRefresh->setEnabled(enable && online);
  ui->doubleSpinXDT->setEnabled(enable && online);
  ui->toolSelectChans->setEnabled(enable && online);
}

void FormOscilloscope::on_pushOscilRefresh_clicked()
{
  emit statusText("Getting traces...");
  emit toggleIO(false);
  runner_thread_.do_oscil(ui->doubleSpinXDT->value());
}

void FormOscilloscope::oscil_complete(std::vector<Qpx::Trace> traces) {
  ui->widgetPlot->clearGraphs();
  ui->widgetPlot->reset_scales();

  double xdt = 0.5;
  QString unit = "n/a";
  std::vector<Gamma::Detector> dets;

  if (!traces.empty()) {

    std::map<double, double> minima, maxima;
    QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::cyan, Qt::blue};

    for (int i=0; i < traces.size(); i++) {
      if (!traces[i].data.size())
        continue;

      uint32_t trace_length = traces[i].data.size();
      dets.push_back(traces[i].detector);
      double xinterval = traces[i].detector.settings_.branches.get(Gamma::Setting("QpxSettings/Pixie-4/System/module/channel/XDT", 17, Gamma::SettingType::floating, i)).value;

      xdt = xinterval;

      QVector<double> xx;
      for (int i=0; i < trace_length; ++i)
        xx.push_back(i*xinterval);

      Gamma::Calibration calib = traces[i].detector.highest_res_calib();
      unit = QString::fromStdString(calib.units_);

      QVector<double> yy;
      for (auto it : traces[i].data) {
        if (calib.units_ != "channels")
          yy.push_back(calib.transform(it, 16));
        else
          yy.push_back(it);
      }

      AppearanceProfile profile;
      profile.default_pen = QPen(palette[i % palette.size()], 1);
      if ((i >= det_on_.size()) || (det_on_[i]))
        ui->widgetPlot->addGraph(xx, yy, profile);

      for (int i=0; i < xx.size(); i++) {
        if (!minima.count(xx[i]) || (minima[xx[i]] > yy[i]))
          minima[xx[i]] = yy[i];
        if (!maxima.count(xx[i]) || (maxima[xx[i]] < yy[i]))
          maxima[xx[i]] = yy[i];
      }

    }

    ui->widgetPlot->setYBounds(minima, maxima);
  }

  ui->widgetPlot->setLabels("time (\u03BCs)", "energy (" + unit + ")");
  ui->doubleSpinXDT->setValue(xdt);

  ui->widgetPlot->rescale();
  ui->widgetPlot->redraw();

  updateMenu(dets);
  emit toggleIO(true);
}

void FormOscilloscope::on_doubleSpinXDT_editingFinished()
{
  on_pushOscilRefresh_clicked();
}

