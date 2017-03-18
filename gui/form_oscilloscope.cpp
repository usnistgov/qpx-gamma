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
#include "histogram.h"
#include <QSettings>


FormOscilloscope::FormOscilloscope(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOscilloscope)
{
  ui->setupUi(this);

  ui->widgetPlot->setScaleType("Linear");
  ui->widgetPlot->setPlotStyle("Step center");
  ui->widgetPlot->setShowMarkerLabels(false);
  ui->widgetPlot->setVisibleOptions(QPlot::zoom |
                                    QPlot::title |
                                    QPlot::save);
  ui->widgetPlot->setTitle("");
  ui->widgetPlot->setAxisLabels("time (ticks)", "energy/channel");

  connect(ui->selectorChannels, SIGNAL(itemSelected(SelectorItem)), this, SLOT(channelDetails(SelectorItem)));
  connect(ui->selectorChannels, SIGNAL(itemToggled(SelectorItem)), this, SLOT(channelToggled(SelectorItem)));
}

FormOscilloscope::~FormOscilloscope()
{
  delete ui;
}

void FormOscilloscope::closeEvent(QCloseEvent *event) {
  event->accept();
}


void FormOscilloscope::updateMenu(std::vector<Qpx::Detector> dets) {
  channels_ = dets;

  QVector<SelectorItem> my_channels = ui->selectorChannels->items();

  bool changed = false;

  if (static_cast<int>(dets.size()) < my_channels.size())
  {
    my_channels.resize(dets.size());
    changed = true;
  }

  ui->pushShowAll->setEnabled(!dets.empty());
  ui->pushHideAll->setEnabled(!dets.empty());

  QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};

  for (size_t i=0; i < dets.size(); ++i) {
    if (static_cast<int>(i) >= my_channels.size()) {
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

void FormOscilloscope::channelToggled(SelectorItem) {
  replot();
}

void FormOscilloscope::channelDetails(SelectorItem item) {
  int i = item.data.toInt();
  QString text;
  if ((i > -1) && (i < static_cast<int32_t>(traces_.size()))) {
    Qpx::Detector det = channels_.at(i);
    text += QString::fromStdString(det.name_);
    text += " (" + QString::fromStdString(det.type_) + ")";
  }
  ui->widgetPlot->setTitle(text);
}

void FormOscilloscope::toggle_push(bool enable, Qpx::ProducerStatus status) {
  bool online = (status & Qpx::ProducerStatus::can_oscil);
  ui->pushOscilRefresh->setEnabled(enable && online);
}

void FormOscilloscope::on_pushOscilRefresh_clicked()
{
  emit refresh_oscil();
}

void FormOscilloscope::oscil_complete(std::vector<Qpx::Hit> traces) {
  if (!this->isVisible())
    return;

  traces_ = traces;

//  updateMenu(dets);

  replot();
}

void FormOscilloscope::replot() {
  ui->widgetPlot->clearAll();

  QString unit = "n/a";

  if (!traces_.empty()) {

    QVector<SelectorItem> my_channels = ui->selectorChannels->items();

    for (size_t i=0; i < traces_.size(); i++) {
      Qpx::Hit trace = traces_.at(i);

      if (!trace.trace().size())
        continue;

      HistMap1D hist;
      for (size_t j=0; j < trace.trace().size(); ++j)
        hist[trace.timestamp().to_nanosec(j) * 0.001] = trace.trace().at(j);

      if ((static_cast<int>(i) < my_channels.size()) && (my_channels[i].visible))
        ui->widgetPlot->addGraph(hist, QPen(my_channels[i].color, 1));

    }
  }

  ui->widgetPlot->setAxisLabels("time (\u03BCs)", "energy (" + unit + ")");
  ui->widgetPlot->replotAll();
  ui->widgetPlot->tightenX();
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
