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
 *
 ******************************************************************************/

#include "form_gates_plot2d.h"
#include "ui_form_gates_plot2d.h"
#include "dialog_spectrum.h"
#include "custom_timer.h"

using namespace Qpx;

FormGatesPlot2D::FormGatesPlot2D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGatesPlot2D)
{
  ui->setupUi(this);

  connect(ui->coincPlot, SIGNAL(clickedPlot(double,double,Qt::MouseButton)),
          this, SLOT(plotClicked(double,double,Qt::MouseButton)));

  connect(ui->coincPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
}

FormGatesPlot2D::~FormGatesPlot2D()
{
  delete ui;
}

void FormGatesPlot2D::loadSettings(QSettings& settings)
{
  settings.beginGroup("AnalysisMatrix");
  ui->coincPlot->setGradient(settings.value("gradient", "Hot").toString());
  ui->coincPlot->setScaleType(settings.value("scale_type", "Logarithmic").toString());
  ui->coincPlot->setShowGradientLegend(settings.value("show_legend", false).toBool());
  settings.endGroup();
}

void FormGatesPlot2D::saveSettings(QSettings& settings)
{
  settings.beginGroup("AnalysisMatrix");
  settings.setValue("gradient", ui->coincPlot->gradient());
  settings.setValue("scale_type", ui->coincPlot->scaleType());
  settings.setValue("show_legend", ui->coincPlot->showGradientLegend());
  settings.endGroup();
}

void FormGatesPlot2D::selection_changed()
{
  emit stuff_selected();
}

void FormGatesPlot2D::setSpectra(ProjectPtr new_set, int64_t idx)
{
  project_ = new_set;
  current_spectrum_ = idx;

  update_plot();
}

std::set<int64_t> FormGatesPlot2D::get_selected_boxes()
{
  std::set<int64_t> ret;
  for (auto b : ui->coincPlot->selectedBoxes())
    ret.insert(b.id);
  for (auto b : ui->coincPlot->selectedLabels())
    ret.insert(b.id);
  return ret;
}

void FormGatesPlot2D::set_boxes(std::list<Bounds2D> boxes)
{
  boxes_ = boxes;
  replot_markers();
  if (!ui->coincPlot->inRange(xmin_, xmax_, ymin_, ymax_))
    refocus();
}

void FormGatesPlot2D::refocus()
{
  double range = std::max(xmax_ - xmin_, ymax_ - ymin_) * 6;
  double xcenter = (xmax_ + xmin_) / 2.0;
  double ycenter = (ymax_ + ymin_) / 2.0;
  double xmin = xcenter - range / 2.0;
  double xmax = xcenter + range / 2.0;
  double ymin = ycenter - range / 2.0;
  double ymax = ycenter + range / 2.0;

  ui->coincPlot->zoomOut(xmin, xmax, ymin, ymax);
  ui->coincPlot->replot();
}


void FormGatesPlot2D::reset_content()
{
  current_spectrum_ = 0;
  boxes_.clear();
  replot_markers();
}

void FormGatesPlot2D::replot_markers()
{
  ui->coincPlot->clearExtras();
  //DBG << "replot markers";

  std::list<QPlot::MarkerBox2D> boxes;
  std::list<QPlot::Label2D> labels;

  QPlot::Label2D label;
  QPlot::MarkerBox2D box;

  xmin_ = std::numeric_limits<double>::max();
  xmax_ = std::numeric_limits<double>::min();
  ymin_ = std::numeric_limits<double>::max();
  ymax_ = std::numeric_limits<double>::min();

  for (Bounds2D &q : boxes_)
  {
    box.id = q.id;
    label.id = q.id;

    box.xc = calib_x_.transform(q.x.centroid(), bits);
    box.x1 = calib_x_.transform(q.x.lower() - 0.5, bits);
    box.x2 = calib_x_.transform(q.x.upper() + 0.5, bits);

    box.yc = calib_y_.transform(q.y.centroid(), bits);
    box.y1 = calib_y_.transform(q.y.lower() - 0.5, bits);
    box.y2 = calib_y_.transform(q.y.upper() + 0.5, bits);

    if (q.selected)
    {
      xmin_ = std::min(xmin_, box.x1);
      xmax_ = std::max(xmax_, box.x2);
      ymin_ = std::min(ymin_, box.y1);
      ymax_ = std::max(ymax_, box.y2);
    }

    box.border = q.color;
    box.fill = box.border;
    box.fill.setAlpha(100);

    box.selectable = q.selectable;
    box.selected = q.selectable && q.selected;

    label.selectable = box.selectable;
    label.selected = box.selected;

    box.mark_center = (q.horizontal && q.vertical);

    if (q.horizontal && q.vertical)
    {
      label.x = box.x2;
      label.y = box.y2;
      label.vertical = false;
      label.text = QString::number(box.yc);
      labels.push_back(label);

      label.x = box.x2;
      label.y = box.y2;
      label.vertical = true;
      label.text = QString::number(box.xc);
      labels.push_back(label);
    }
    else if (q.horizontal)
    {
      label.x = box.xc;
      label.y = box.y2;
      label.vertical = false;
      label.vfloat = false;
      label.hfloat = q.labelfloat;
      label.text = QString::number(box.yc);
      labels.push_back(label);
    }
    else if (q.vertical)
    {
      label.x = box.x2;
      label.y = box.yc;
      label.vertical = true;
      label.vfloat = q.labelfloat;
      label.hfloat = false;
      label.text = QString::number(box.xc);
      labels.push_back(label);
    }

    boxes.push_back(box);
  }

  ui->coincPlot->setBoxes(boxes);
  ui->coincPlot->setLabels(labels);
  ui->coincPlot->replotExtras();
  ui->coincPlot->replot();
}

void FormGatesPlot2D::update_plot()
{
  this->setCursor(Qt::WaitCursor);
//  CustomTimer guiside(true);

  SinkPtr some_spectrum = project_->get_sink(current_spectrum_);

  Metadata md;
  if (some_spectrum)
    md = some_spectrum->metadata();

  uint16_t newbits = md.get_attribute("resolution").value_int;

  uint32_t adjrange{0};
  if ((md.dimensions() == 2) && (adjrange = pow(2,newbits)) )
  {
    bits = newbits;

    Detector detector_x_;
    Detector detector_y_;
    if (md.detectors.size() > 1)
    {
      detector_x_ = md.detectors[0];
      detector_y_ = md.detectors[1];
    }

    calib_x_ = detector_x_.best_calib(bits);
    calib_y_ = detector_y_.best_calib(bits);

    std::shared_ptr<EntryList> spectrum_data =
        std::move(some_spectrum->data_range({{0, adjrange}, {0, adjrange}}));

    HistList2D hist;
    if (spectrum_data)
    {
      for (auto p : *spectrum_data)
        hist.push_back(p2d(p.first.at(0), p.first.at(1), p.second.convert_to<double>()));
    }
//    DBG << md.get_attribute("name").value_text << " hist size " << hist.size();
    ui->coincPlot->updatePlot(adjrange + 1, adjrange + 1, hist);


    ui->coincPlot->setAxes(
          QString::fromStdString(calib_x_.units_),
          calib_x_.transform(0, bits), calib_x_.transform(adjrange, bits),
          QString::fromStdString(calib_y_.units_),
          calib_y_.transform(0, bits), calib_y_.transform(adjrange, bits),
          "Event count");

  }
  else
    ui->coincPlot->clearAll();

  replot_markers();

//  DBG << "<Plot2D> plotting took " << guiside.ms() << " ms";
  this->setCursor(Qt::ArrowCursor);
}

void FormGatesPlot2D::plotClicked(double x, double y, Qt::MouseButton button)
{
  if (button == Qt::LeftButton)
    emit clickedPlot(x, y);
  else
    emit clearSelection();
}
