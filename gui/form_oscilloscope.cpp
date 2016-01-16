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
  ui->widgetPlot->set_plot_style("Step center");
  ui->widgetPlot->set_marker_labels(false);
  ui->widgetPlot->set_visible_options(ShowOptions::zoom | ShowOptions::title | ShowOptions::save);
  ui->widgetPlot->setTitle("");
  ui->widgetPlot->setLabels("time (ticks)", "energy/channel");

  loadSettings();

  connect(&runner_thread_, SIGNAL(oscilReadOut(std::vector<Qpx::Trace>)), this, SLOT(oscil_complete(std::vector<Qpx::Trace>)));

  connect(ui->selectorChannels, SIGNAL(itemSelected(SelectorItem)), this, SLOT(channelDetails(SelectorItem)));
  connect(ui->selectorChannels, SIGNAL(itemToggled(SelectorItem)), this, SLOT(channelToggled(SelectorItem)));

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
  ui->doubleSpinXDT->setValue(settings_.value("xdt", 0.5).toDouble());
  settings_.endGroup();
}

void FormOscilloscope::saveSettings() {
  settings_.beginGroup("Oscilloscope");
  settings_.setValue("xdt", ui->doubleSpinXDT->value());
  settings_.endGroup();
}

void FormOscilloscope::updateMenu(std::vector<Gamma::Detector> dets) {
  QVector<SelectorItem> my_channels = ui->selectorChannels->items();

  bool changed = false;

  if (dets.size() < my_channels.size()) {
    my_channels.resize(dets.size());
    changed = true;
  }

  ui->pushShowAll->setEnabled(!dets.empty());
  ui->pushHideAll->setEnabled(!dets.empty());

  QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};

  for (int i=0; i < dets.size(); ++i) {
    if (i >= my_channels.size()) {
      my_channels.push_back(SelectorItem());
      my_channels[i].visible = true;
      changed = true;
    }

    if (my_channels[i].text != QString::fromStdString(dets[i].name_)) {
      my_channels[i].data = QVariant::fromValue(i);
      my_channels[i].text = QString::fromStdString(dets[i].name_);
      my_channels[i].color = palette[i % palette.size()];
      changed = true;
    }
  }

  if (changed) {
    ui->selectorChannels->setItems(my_channels);
    replot();
  }
}

double FormOscilloscope::xdt() {
  return ui->doubleSpinXDT->value();
}


void FormOscilloscope::channelToggled(SelectorItem) {
  replot();
}

void FormOscilloscope::channelDetails(SelectorItem item) {
  int i = item.data.toInt();
  QString text;
  if ((i > -1) && (i < traces_.size())) {
    Gamma::Detector det = traces_[i].detector;
    text += QString::fromStdString(det.name_);
    text += " (" + QString::fromStdString(det.type_) + ")";
  }
  ui->widgetPlot->setTitle(text);
}

void FormOscilloscope::toggle_push(bool enable, Qpx::DeviceStatus status) {
  bool online = (status & Qpx::DeviceStatus::can_oscil);
  ui->pushOscilRefresh->setEnabled(enable && online);
  ui->doubleSpinXDT->setEnabled(enable && online);
}

void FormOscilloscope::on_pushOscilRefresh_clicked()
{
  emit statusText("Getting traces...");
  emit toggleIO(false);
  runner_thread_.do_oscil(ui->doubleSpinXDT->value());
}

void FormOscilloscope::oscil_complete(std::vector<Qpx::Trace> traces) {
  if (!this->isVisible())
    return;

  traces_ = traces;

  std::vector<Gamma::Detector> dets;
  for (auto &q : traces) {
    dets.push_back(q.detector);
  }

  updateMenu(dets);

  replot();

  emit toggleIO(true);
}

void FormOscilloscope::replot() {
  ui->widgetPlot->clearGraphs();
  ui->widgetPlot->reset_scales();

  QString unit = "n/a";
  double xdt = 0.5;

  if (!traces_.empty()) {

    QVector<SelectorItem> my_channels = ui->selectorChannels->items();

    std::map<double, double> minima, maxima;

    for (int i=0; i < traces_.size(); i++) {
      uint32_t trace_length = traces_[i].data.size();

      if (!trace_length)
        continue;

      double xinterval = traces_[i].detector.settings_.get_setting(Gamma::Setting("XDT"), Gamma::Match::name).value_dbl;

      xdt = xinterval;

      PL_DBG << "trace " << i << " length " << trace_length << " xdt " << xdt;

      QVector<double> xx;
      for (int i=0; i < trace_length; ++i)
        xx.push_back(i*xinterval);

      Gamma::Calibration calib = traces_[i].detector.highest_res_calib();
      unit = QString::fromStdString(calib.units_);

      QVector<double> yy;
      for (auto it : traces_[i].data) {
        if (calib.valid())
          yy.push_back(calib.transform(it, 16));
        else
          yy.push_back(it);
      }

      if ((i < my_channels.size()) && (my_channels[i].visible)) {
        PL_DBG << "will plot trace " << i;
        AppearanceProfile profile;
        profile.default_pen = QPen(my_channels[i].color, 1);
        ui->widgetPlot->addGraph(xx, yy, profile);

        for (int i=0; i < xx.size(); i++) {
          if (!minima.count(xx[i]) || (minima[xx[i]] > yy[i]))
            minima[xx[i]] = yy[i];
          if (!maxima.count(xx[i]) || (maxima[xx[i]] < yy[i]))
            maxima[xx[i]] = yy[i];
        }
      }

    }

    ui->widgetPlot->setYBounds(minima, maxima);
  }

  ui->widgetPlot->setLabels("time (\u03BCs)", "energy (" + unit + ")");
  ui->doubleSpinXDT->setValue(xdt);

  ui->widgetPlot->rescale();
  ui->widgetPlot->redraw();

}

void FormOscilloscope::on_doubleSpinXDT_editingFinished()
{
  on_pushOscilRefresh_clicked();
}


void FormOscilloscope::on_pushShowAll_clicked()
{
  ui->selectorChannels->show_all();
  replot();
}

void FormOscilloscope::on_pushHideAll_clicked()
{
  ui->selectorChannels->hide_all();
  replot();
}
