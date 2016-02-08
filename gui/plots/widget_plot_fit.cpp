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
 *      WidgetPlotFit -
 *
 ******************************************************************************/

#include "widget_plot_fit.h"
#include "ui_widget_plot_fit.h"
#include "custom_logger.h"
#include "qt_util.h"


WidgetPlotFit::WidgetPlotFit(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WidgetPlotFit),
  fit_data_(nullptr)
{
  ui->setupUi(this);

  ui->mcaPlot->setInteraction(QCP::iSelectItems, true);
  ui->mcaPlot->setInteraction(QCP::iRangeDrag, true);
  ui->mcaPlot->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, true);
  ui->mcaPlot->setInteraction(QCP::iMultiSelect, true);

  ui->mcaPlot->yAxis->setPadding(28);
  ui->mcaPlot->setNoAntialiasingOnDrag(true);
  ui->mcaPlot->xAxis->grid()->setVisible(true);
  ui->mcaPlot->yAxis->grid()->setVisible(true);
  ui->mcaPlot->xAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->yAxis->grid()->setSubGridVisible(true);

//  ui->mcaPlot->legend->setVisible(true);
//  ui->mcaPlot->legend->setBrush(QBrush(QColor(255,255,255,150)));
//  ui->mcaPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignTop);

  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->mcaPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->mcaPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));

  minx_zoom = 0; maxx_zoom = 0;
  minx = std::numeric_limits<double>::max();
  maxx = - std::numeric_limits<double>::max();
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  force_rezoom_ = false;
  mouse_pressed_ = false;

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;

  scale_type_ = "Logarithmic";
  ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));

  ui->mcaPlot->xAxis->grid()->setVisible(true);
  ui->mcaPlot->yAxis->grid()->setVisible(true);
  ui->mcaPlot->xAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->yAxis->grid()->setSubGridVisible(true);

  menuExportFormat.addAction("png");
  menuExportFormat.addAction("jpg");
  menuExportFormat.addAction("pdf");
  menuExportFormat.addAction("bmp");
  connect(&menuExportFormat, SIGNAL(triggered(QAction*)), this, SLOT(exportRequested(QAction*)));

  connect(&menuOptions, SIGNAL(triggered(QAction*)), this, SLOT(optionsChanged(QAction*)));

  menuROI.addAction("Rollback...");
  menuROI.addAction("Refit");
  connect(&menuROI, SIGNAL(triggered(QAction*)), this, SLOT(changeROI(QAction*)));


  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  connect(shortcut, SIGNAL(activated()), this, SLOT(zoom_out()));


  build_menu();

  plotButtons();
  //  redraw();
}

WidgetPlotFit::~WidgetPlotFit()
{
  delete ui;
}

void WidgetPlotFit::build_menu() {
  menuOptions.clear();
  menuOptions.addSeparator();
  menuOptions.addAction("Linear");
  menuOptions.addAction("Logarithmic");

  for (auto &q : menuOptions.actions()) {
    q->setCheckable(true);
    q->setChecked(q->text() == scale_type_);
  }
}


void WidgetPlotFit::clearGraphs()
{
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearItems();
  minima_.clear();
  maxima_.clear();
}

void WidgetPlotFit::clearSelection()
{
  my_range_.visible = false;
  selected_peaks_.clear();
  plotRange();
  calc_visible();
}

void WidgetPlotFit::rescale() {
  force_rezoom_ = true;
  plot_rezoom();
}

void WidgetPlotFit::redraw() {
  ui->mcaPlot->replot();
}

void WidgetPlotFit::reset_scales()
{
  minx = std::numeric_limits<double>::max();
  maxx = - std::numeric_limits<double>::max();
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();
  ui->mcaPlot->rescaleAxes();
}

void WidgetPlotFit::setTitle(QString title) {
  title_text_ = title;
  std::list<QCPAbstractItem*> to_remove;
  for (int i=0; i < ui->mcaPlot->itemCount(); ++i)
    if (ui->mcaPlot->item(i)->property("floating text").isValid())
      to_remove.push_back(ui->mcaPlot->item(i));
  for (auto &q : to_remove)
    ui->mcaPlot->removeItem(q);

  if (!title_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->mcaPlot);
    ui->mcaPlot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(title_text_);
    floatingText->setProperty("floating text", true);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }
}

void WidgetPlotFit::set_range(Range rng) {
  //  PL_DBG << "<WidgetPlotFit> set range";
  my_range_ = rng;
  plotRange();
  if (my_range_.visible)
    follow_selection();
  calc_visible();
}

void WidgetPlotFit::select_peaks(const std::set<double> &peaks) {
  selected_peaks_ = peaks;
  if (!selected_peaks_.empty())
    follow_selection();
  calc_visible();
}

std::set<double> WidgetPlotFit::get_selected_peaks() {
  return selected_peaks_;
}


void WidgetPlotFit::addGraph(const QVector<double>& x, const QVector<double>& y,
                             QPen appearance, int fittable, int32_t bits,
                             QString name) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  ui->mcaPlot->graph(g)->setPen(appearance);
  ui->mcaPlot->graph(g)->setProperty("fittable", QVariant::fromValue(fittable));
  ui->mcaPlot->graph(g)->setProperty("bits", QVariant::fromValue(bits));
  ui->mcaPlot->graph(g)->setName(name);
  ui->mcaPlot->graph(g)->setProperty("start", QVariant::fromValue(x.front()));

  if (fittable > 0) {
    ui->mcaPlot->graph(g)->setBrush(QBrush());
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsStepCenter);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  } else {
    ui->mcaPlot->graph(g)->setBrush(QBrush());
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  }

  if (x[0] < minx) {
    minx = x[0];
    //PL_DBG << "new minx " << minx;
    ui->mcaPlot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
    ui->mcaPlot->xAxis->rescale();
  }
}

void WidgetPlotFit::plot_rezoom() {
  if (mouse_pressed_)
    return;

  if (minima_.empty() || maxima_.empty()) {
    ui->mcaPlot->yAxis->rescale();
    return;
  }

  double upperc = ui->mcaPlot->xAxis->range().upper;
  double lowerc = ui->mcaPlot->xAxis->range().lower;

  if (!force_rezoom_ && (lowerc == minx_zoom) && (upperc == maxx_zoom))
    return;

  minx_zoom = lowerc;
  maxx_zoom = upperc;
  calc_visible();

  force_rezoom_ = false;

  calc_y_bounds(lowerc, upperc);

  //PL_DBG << "Rezoom";

  if (miny <= 0)
    ui->mcaPlot->yAxis->rescale();
  else
    ui->mcaPlot->yAxis->setRangeLower(miny);
  ui->mcaPlot->yAxis->setRangeUpper(maxy);

}

void WidgetPlotFit::tight_x() {
  //PL_DBG << "tightning x to " << minx << " " << maxx;
  ui->mcaPlot->xAxis->setRangeLower(minx);
  ui->mcaPlot->xAxis->setRangeUpper(maxx);
}

void WidgetPlotFit::calc_y_bounds(double lower, double upper) {
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::min();

  for (std::map<double, double>::const_iterator it = minima_.lower_bound(lower); it != minima_.upper_bound(upper); ++it)
    if (it->second < miny)
      miny = it->second;

  for (std::map<double, double>::const_iterator it = maxima_.lower_bound(lower); it != maxima_.upper_bound(upper); ++it) {
    if (it->second > maxy)
      maxy = it->second;
  }

  if (my_range_.visible && (edge_trc1 != nullptr) && (edge_trc2 != nullptr)) {
    if (edge_trc1->position->value() > maxy)
      maxy = edge_trc1->position->value();
    if (edge_trc1->position->value() < miny)
      miny = edge_trc1->position->value();
    if (edge_trc2->position->value() > maxy)
      maxy = edge_trc2->position->value();
    if (edge_trc2->position->value() < miny)
      miny = edge_trc2->position->value();
  }

  double miny2 = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(miny) + 20);
  maxy = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(maxy) - 100);

  if (miny2 > 1.0)
    miny = miny2;
}

void WidgetPlotFit::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
  ui->mcaPlot->xAxis->setBasePen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setBasePen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setTickPen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setTickPen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setSubTickPen(QPen(fore, 1));
  ui->mcaPlot->yAxis->setSubTickPen(QPen(fore, 1));
  ui->mcaPlot->xAxis->setTickLabelColor(fore);
  ui->mcaPlot->yAxis->setTickLabelColor(fore);
  ui->mcaPlot->xAxis->setLabelColor(fore);
  ui->mcaPlot->yAxis->setLabelColor(fore);
  ui->mcaPlot->xAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->mcaPlot->yAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->mcaPlot->xAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->mcaPlot->yAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->mcaPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->setBackground(QBrush(back));
}

void WidgetPlotFit::plotRange() {
  std::list<QCPAbstractItem*> to_remove;
  for (int i=0; i < ui->mcaPlot->itemCount(); ++i)
    if (ui->mcaPlot->item(i)->property("tracer").isValid())
      to_remove.push_back(ui->mcaPlot->item(i));
  for (auto &q : to_remove)
    ui->mcaPlot->removeItem(q);

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;
  int fit_level = 0;

  if (my_range_.visible) {

    double pos_l = 0, pos_c = 0, pos_r = 0;
    pos_l = my_range_.l.energy();
    pos_c = my_range_.center.energy();
    pos_r = my_range_.r.energy();

    if (pos_l < pos_r) {

      int top_idx = -1;
      for (int i=0; i < ui->mcaPlot->graphCount(); i++) {

        int level = ui->mcaPlot->graph(i)->property("fittable").toInt();

        if (level < fit_level)
          continue;

        if ((ui->mcaPlot->graph(i)->data()->firstKey() > pos_l)
            || (pos_r > ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        fit_level = level;
        top_idx = i;
      }

      if (top_idx >= 0) {
        edge_trc1 = new QCPItemTracer(ui->mcaPlot);
        edge_trc1->setStyle(QCPItemTracer::tsNone);
        edge_trc1->setGraph(ui->mcaPlot->graph(top_idx));
        edge_trc1->setGraphKey(pos_l);
        edge_trc1->setInterpolating(true);
        edge_trc1->setProperty("tracer", QVariant::fromValue(0));
        ui->mcaPlot->addItem(edge_trc1);
        edge_trc1->updatePosition();

        edge_trc2 = new QCPItemTracer(ui->mcaPlot);
        edge_trc2->setStyle(QCPItemTracer::tsNone);
        edge_trc2->setGraph(ui->mcaPlot->graph(top_idx));
        edge_trc2->setGraphKey(pos_r);
        edge_trc2->setInterpolating(true);
        edge_trc2->setProperty("tracer", QVariant::fromValue(0));
        ui->mcaPlot->addItem(edge_trc2);
        edge_trc2->updatePosition();
      } else
        fit_level = 0;
    } else {
      //      PL_DBG << "<WidgetPlotFit> bad range";
      my_range_.visible = false;
      //emit range_moved();
    }
  }

  if (fit_level > 0) {
      DraggableTracer *ar1 = new DraggableTracer(ui->mcaPlot, edge_trc1, 10);
      ar1->setPen(QPen(Qt::darkGreen, 1));
      ar1->setProperty("tracer", QVariant::fromValue(1));
      ar1->setSelectable(true);
      ar1->set_limits(minx, maxx);
      ui->mcaPlot->addItem(ar1);

      DraggableTracer *ar2 = new DraggableTracer(ui->mcaPlot, edge_trc2, 10);
      ar2->setPen(QPen(Qt::darkGreen, 1));
      ar2->setProperty("tracer", QVariant::fromValue(1));
      ar2->setSelectable(true);
      ar2->set_limits(minx, maxx);
      ui->mcaPlot->addItem(ar2);

      QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
      line->setSelectable(false);
      line->setProperty("tracer", QVariant::fromValue(2));
      line->start->setParentAnchor(edge_trc1->position);
      line->start->setCoords(0, 0);
      line->end->setParentAnchor(edge_trc2->position);
      line->end->setCoords(0, 0);
      line->setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
      ui->mcaPlot->addItem(line);
  }
}

void WidgetPlotFit::plotEnergyLabels() {
  if (fit_data_ == nullptr)
    return;

  for (auto &r : fit_data_->regions_) {
    for (auto &q : r.peaks_) {
      QCPItemTracer *top_crs = nullptr;

      double max = std::numeric_limits<double>::lowest();
      for (int i=0; i < ui->mcaPlot->graphCount(); i++) {

        if (!ui->mcaPlot->graph(i)->property("fittable").toBool())
          continue;

        if ((ui->mcaPlot->graph(i)->data()->firstKey() >= q.second.energy)
            || (q.second.energy >= ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
        crs->setStyle(QCPItemTracer::tsNone); //tsCirlce?
        crs->setProperty("chan_value", q.second.center);

        crs->setGraph(ui->mcaPlot->graph(i));
        crs->setInterpolating(true);
        crs->setGraphKey(q.second.energy);
        ui->mcaPlot->addItem(crs);

        crs->updatePosition();
        double val = crs->positions().first()->value();
        if (val > max) {
          max = val;
          top_crs = crs;
        }
      }


      if (top_crs != nullptr) {
        QPen pen = QPen(Qt::darkGray, 1);
        QPen selected_pen = QPen(Qt::black, 2);

        QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
        line->start->setParentAnchor(top_crs->position);
        line->start->setCoords(0, -35);
        line->end->setParentAnchor(top_crs->position);
        line->end->setCoords(0, -5);
        line->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 7, 18));
        line->setPen(pen);
        line->setSelectedPen(selected_pen);
        line->setProperty("chan_value", top_crs->property("chan_value"));
        line->setSelectable(true);
        line->setSelected(selected_peaks_.count(q.second.center));
        ui->mcaPlot->addItem(line);

        QCPItemText *markerText = new QCPItemText(ui->mcaPlot);
        markerText->setProperty("chan_value", top_crs->property("chan_value"));

        markerText->position->setParentAnchor(top_crs->position);
        markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
        markerText->position->setCoords(0, -35);
        markerText->setText(QString::number(q.second.energy));
        markerText->setTextAlignment(Qt::AlignLeft);
        markerText->setFont(QFont("Helvetica", 9));
        markerText->setPen(pen);
        markerText->setColor(pen.color());
        markerText->setSelectedColor(selected_pen.color());
        markerText->setSelectedPen(selected_pen);
        markerText->setPadding(QMargins(1, 1, 1, 1));
        markerText->setSelectable(true);
        markerText->setSelected(selected_peaks_.count(q.second.center));
        ui->mcaPlot->addItem(markerText);

      }

    }
  }
}

void WidgetPlotFit::plotROI_options() {
  for (auto &r : fit_data_->regions_) {
    double x = r.hr_x_nrg.front();
    double y = r.hr_fullfit.front();
    QCPItemPixmap *overlayButton;

    overlayButton = new QCPItemPixmap(ui->mcaPlot);
//    overlayButton->setClipToAxisRect(false);
    overlayButton->setPixmap(QPixmap(":/icons/oxy/32/package_utilities.png"));
    overlayButton->topLeft->setType(QCPItemPosition::ptPlotCoords);
    overlayButton->topLeft->setCoords(x, y);
    overlayButton->bottomRight->setParentAnchor(overlayButton->topLeft);
    overlayButton->bottomRight->setCoords(16, 16);
    overlayButton->setScaled(true);
    overlayButton->setSelectable(false);
    overlayButton->setProperty("button_name", QString("ROI options"));
    overlayButton->setProperty("ROI", QVariant::fromValue(x));
    overlayButton->setProperty("tooltip", QString("Do ROI stuff"));
    ui->mcaPlot->addItem(overlayButton);

  }
}


void WidgetPlotFit::follow_selection() {
  double min_marker = std::numeric_limits<double>::max();
  double max_marker = - std::numeric_limits<double>::max();

  for (auto &q : selected_peaks_) {
    double nrg = fit_data_->nrg_cali_.transform(q);
    if (nrg > max_marker)
      max_marker = nrg;
    if (nrg < min_marker)
      min_marker = nrg;
  }

  if (my_range_.visible) {
    if (my_range_.l.energy() < min_marker)
      min_marker = my_range_.l.energy();
    if (my_range_.r.energy() < max_marker)
      max_marker = my_range_.r.energy();
  }

  bool xaxis_changed = false;
  double dif_lower = min_marker - ui->mcaPlot->xAxis->range().lower;
  double dif_upper = max_marker - ui->mcaPlot->xAxis->range().upper;
  if (dif_upper > 0) {
    ui->mcaPlot->xAxis->setRangeUpper(max_marker + 20);
    if (dif_lower > (dif_upper + 20))
      ui->mcaPlot->xAxis->setRangeLower(ui->mcaPlot->xAxis->range().lower + dif_upper + 20);
    xaxis_changed = true;
  }

  if (dif_lower < 0) {
    ui->mcaPlot->xAxis->setRangeLower(min_marker - 20);
    if (dif_upper < (dif_lower - 20))
      ui->mcaPlot->xAxis->setRangeUpper(ui->mcaPlot->xAxis->range().upper + dif_lower - 20);
    xaxis_changed = true;
  }

  if (xaxis_changed) {
    ui->mcaPlot->replot();
    plot_rezoom();
  }

//  miny = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(miny) + 20);
//  maxy = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(maxy) - 100);


}

void WidgetPlotFit::plotButtons() {
  QCPItemPixmap *overlayButton;

  overlayButton = new QCPItemPixmap(ui->mcaPlot);
  overlayButton->setClipToAxisRect(false);
  overlayButton->setPixmap(QPixmap(":/icons/oxy/16/view_fullscreen.png"));
  overlayButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  overlayButton->topLeft->setCoords(5, 5);
  overlayButton->bottomRight->setParentAnchor(overlayButton->topLeft);
  overlayButton->bottomRight->setCoords(16, 16);
  overlayButton->setScaled(true);
  overlayButton->setSelectable(false);
  overlayButton->setProperty("button_name", QString("reset_scales"));
  overlayButton->setProperty("tooltip", QString("Reset plot scales"));
  ui->mcaPlot->addItem(overlayButton);

  if (!menuOptions.isEmpty()) {
    QCPItemPixmap *newButton = new QCPItemPixmap(ui->mcaPlot);
    newButton->setClipToAxisRect(false);
    newButton->setPixmap(QPixmap(":/icons/oxy/16/view_statistics.png"));
    newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    newButton->bottomRight->setParentAnchor(newButton->topLeft);
    newButton->bottomRight->setCoords(16, 16);
    newButton->setScaled(false);
    newButton->setSelectable(false);
    newButton->setProperty("button_name", QString("options"));
    newButton->setProperty("tooltip", QString("Style options"));
    ui->mcaPlot->addItem(newButton);
    overlayButton = newButton;
  }

  if (true) {
    QCPItemPixmap *newButton = new QCPItemPixmap(ui->mcaPlot);
    newButton->setClipToAxisRect(false);
    newButton->setPixmap(QPixmap(":/icons/oxy/16/document_save.png"));
    newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    newButton->bottomRight->setParentAnchor(newButton->topLeft);
    newButton->bottomRight->setCoords(16, 16);
    newButton->setScaled(true);
    newButton->setSelectable(false);
    newButton->setProperty("button_name", QString("export"));
    newButton->setProperty("tooltip", QString("Export plot"));
    ui->mcaPlot->addItem(newButton);
    overlayButton = newButton;
  }
}


void WidgetPlotFit::plot_mouse_clicked(double x, double y, QMouseEvent* event, bool on_item) {
  if (event->button() == Qt::RightButton) {
    emit clickedRight(x);
  } else if (!on_item) { //tricky
    emit clickedLeft(x);
  }
}


void WidgetPlotFit::selection_changed() {
  selected_peaks_.clear();
  for (auto &q : ui->mcaPlot->selectedItems())
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      if (txt->property("chan_value").isValid())
        selected_peaks_.insert(txt->property("chan_value").toDouble());
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("chan_value").isValid())
        selected_peaks_.insert(line->property("chan_value").toDouble());
    }
  my_range_.visible = false;
  plotRange();
  calc_visible();

  emit markers_selected();
}

void WidgetPlotFit::clicked_plottable(QCPAbstractPlottable *plt) {
  //  PL_INFO << "<WidgetPlotFit> clickedplottable";
}

void WidgetPlotFit::clicked_item(QCPAbstractItem* itm) {
  if (QCPItemPixmap *pix = qobject_cast<QCPItemPixmap*>(itm)) {
    //    QPoint p = this->mapFromGlobal(QCursor::pos());
    QString name = pix->property("button_name").toString();
    if (name == "options") {
      menuOptions.exec(QCursor::pos());
    } else if (name == "export") {
      menuExportFormat.exec(QCursor::pos());
    } else if (name == "reset_scales") {
      zoom_out();
    } else if (name == "ROI options") {
      menuROI.setProperty("ROI", pix->property("ROI"));
      menuROI.exec(QCursor::pos());
    }
  }
}

void WidgetPlotFit::zoom_out() {
  ui->mcaPlot->rescaleAxes();
  force_rezoom_ = true;
  plot_rezoom();
  ui->mcaPlot->replot();

}

void WidgetPlotFit::plot_mouse_press(QMouseEvent*) {
  disconnect(ui->mcaPlot, 0, this, 0);
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->mcaPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  force_rezoom_ = false;
  mouse_pressed_ = true;
}

void WidgetPlotFit::plot_mouse_release(QMouseEvent*) {
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));
  force_rezoom_ = true;
  mouse_pressed_ = false;
  plot_rezoom();
  ui->mcaPlot->replot();

  if ((edge_trc1 != nullptr) || (edge_trc2 != nullptr))
    emit range_moved(edge_trc1->graphKey(), edge_trc2->graphKey());
}

void WidgetPlotFit::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (choice == "Linear") {
    scale_type_ = choice;
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "Logarithmic") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    scale_type_ = choice;
  }

  build_menu();
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void WidgetPlotFit::changeROI(QAction* action) {
  QString choice = action->text();
  double ROI_nrg = menuROI.property("ROI").toDouble();
  if (choice == "Rollback...") {
    emit rollback_ROI(ROI_nrg);
  } else if (choice == "Refit") {
    emit refit_ROI(ROI_nrg);
  }
}

QString WidgetPlotFit::scale_type() {
  return scale_type_;
}

void WidgetPlotFit::set_scale_type(QString sct) {
  this->setCursor(Qt::WaitCursor);
  scale_type_ = sct;
  if (scale_type_ == "Linear")
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type() == "Logarithmic")
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->mcaPlot->replot();
  build_menu();
  this->setCursor(Qt::ArrowCursor);
}

void WidgetPlotFit::exportRequested(QAction* choice) {
  QString filter = choice->text() + "(*." + choice->text() + ")";
  QString fileName = CustomSaveFileDialog(this, "Export plot",
                                          QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                          filter);
  if (validateFile(this, fileName, true)) {
    QFileInfo file(fileName);
    if (file.suffix() == "png") {
      //      PL_INFO << "Exporting plot to png " << fileName.toStdString();
      ui->mcaPlot->savePng(fileName,0,0,1,100);
    } else if (file.suffix() == "jpg") {
      //      PL_INFO << "Exporting plot to jpg " << fileName.toStdString();
      ui->mcaPlot->saveJpg(fileName,0,0,1,100);
    } else if (file.suffix() == "bmp") {
      //      PL_INFO << "Exporting plot to bmp " << fileName.toStdString();
      ui->mcaPlot->saveBmp(fileName);
    } else if (file.suffix() == "pdf") {
      //      PL_INFO << "Exporting plot to pdf " << fileName.toStdString();
      ui->mcaPlot->savePdf(fileName, true);
    }
  }
}


void WidgetPlotFit::setData(Gamma::Fitter* fit) {
  fit_data_ = fit;
  updateData();
}

void WidgetPlotFit::add_bounds(const QVector<double>& x, const QVector<double>& y) {
  if (x.size() != y.size())
    return;

  for (int i=0; i < x.size(); ++i) {
    if (!minima_.count(x[i]) || (minima_[x[i]] > y[i]))
      minima_[x[i]] = y[i];
    if (!maxima_.count(x[i]) || (maxima_[x[i]] < y[i]))
      maxima_[x[i]] = y[i];
  }
}

void WidgetPlotFit::updateData() {
  if (fit_data_ == nullptr)
    return;

  QColor plotcolor;
  plotcolor.setHsv(QColor::fromRgba(fit_data_->metadata_.appearance).hsvHue(), 48, 160);
  QPen main_graph = QPen(plotcolor, 1);

  QPen back_poly = QPen(Qt::darkGray, 1);
  QPen back_with_steps = QPen(Qt::darkBlue, 1);
  QPen full_fit = QPen(Qt::red, 2);

  QPen peak  = QPen(Qt::darkCyan, 2);
  peak.setStyle(Qt::DotLine);

  QPen resid = QPen( Qt::darkGreen, 1);
  resid.setStyle(Qt::DashLine);

//  QColor flagged_color;
//  flagged_color.setHsv(QColor(Qt::green).hsvHue(), QColor(Qt::green).hsvSaturation(), 192);
//  QPen flagged =  QPen(flagged_color, 1);
//  flagged.setStyle(Qt::DotLine);

  QPen sum4edge = QPen(Qt::darkYellow, 3);
  QPen sum4back = QPen(Qt::darkYellow, 1);

  this->clearGraphs();

  minima_.clear(); maxima_.clear();

  ui->mcaPlot->xAxis->setLabel(QString::fromStdString(fit_data_->nrg_cali_.units_));
  ui->mcaPlot->yAxis->setLabel("count");

  QVector<double> xx, yy;

  xx = QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(fit_data_->finder_.x_, fit_data_->metadata_.bits));
  yy = QVector<double>::fromStdVector(fit_data_->finder_.y_);

  addGraph(xx, yy, main_graph, 1, fit_data_->metadata_.bits, "Data");
  add_bounds(xx, yy);

  for (auto &q : fit_data_->regions_) {
    xx = QVector<double>::fromStdVector(q.hr_x_nrg);

    yy = QVector<double>::fromStdVector(q.hr_fullfit);
    trim_log_lower(yy);
    addGraph(xx, yy, full_fit, false, q.bits_, "Region fit");
    add_bounds(xx, yy);

    for (auto & p : q.peaks_) {
      yy = QVector<double>::fromStdVector(p.second.hr_fullfit_);
      trim_log_lower(yy);
//      QPen pen = p.second.flagged ? flagged : peak;
      addGraph(xx, yy, peak, false, q.bits_, "Individual peak");

      if (p.second.sum4_.peak_width > 0) {
        addEdge(p.second.sum4_.LB(), sum4edge);

        if (!p.second.sum4_.bx.empty()) {
          std::vector<double> x_sum4;
          for (auto &i : p.second.sum4_.bx)
            x_sum4.push_back(fit_data_->finder_.x_[i]);
          addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(x_sum4, fit_data_->metadata_.bits)),
                               QVector<double>::fromStdVector(p.second.sum4_.by),
                               sum4back, false, fit_data_->metadata_.bits, "SUM4 background");
        }

        addEdge(p.second.sum4_.RB(), sum4edge);
      }
    }

    if (!q.peaks_.empty()) {

      yy = QVector<double>::fromStdVector(q.hr_background);
      trim_log_lower(yy);
      addGraph(xx, yy, back_poly, false, q.bits_, "Background poly");
      add_bounds(xx, yy);

      yy = QVector<double>::fromStdVector(q.hr_back_steps);
      trim_log_lower(yy);
      addGraph(xx, yy, back_with_steps, false, q.bits_, "Background steps");

      xx = QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.finder_.x_, fit_data_->metadata_.bits));
      yy = QVector<double>::fromStdVector(q.finder_.y_resid_on_background_);
//      trim_log_lower(yy); //maybe not?
      addGraph(xx, yy, resid, 2, q.bits_, "Residuals");
//      add_bounds(xx, yy);
    }

  }

  ui->mcaPlot->clearItems();

  setTitle(title_text_);

  plotEnergyLabels();
  plotRange();
  plotButtons();
  plotROI_options();

  calc_visible();
  rescale();
}

void WidgetPlotFit::addEdge(Gamma::SUM4Edge edge, QPen pen) {
  std::vector<double> x_edge, y_edge;
  for (int i = edge.start(); i <= edge.end(); ++i) {
    x_edge.push_back(fit_data_->finder_.x_[i]);
    y_edge.push_back(edge.average());
  }
//  if (!x_edge.empty()) {
//    x_edge[0] -= 0.5;
//    x_edge[x_edge.size()-1] += 0.5;
//  }

  addGraph(QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(x_edge, fit_data_->metadata_.bits)),
                       QVector<double>::fromStdVector(y_edge),
                       pen, false, fit_data_->metadata_.bits, "SUM4 edge");
}


void WidgetPlotFit::trim_log_lower(QVector<double> &array) {
  for (auto &r : array)
    if (r < 1)
      r = std::floor(r * 10 + 0.5)/10;
}

void WidgetPlotFit::calc_visible() {
  uint16_t visible_roi_ = 0;
  uint16_t visible_peaks_ = 0;
  std::set<double> good_starts;
//  ui->mcaPlot->legend->clearItems();

  if (fit_data_ == nullptr)
    return;

  for (auto &q : fit_data_->regions_) {
    if (q.hr_x_nrg.empty() ||
        (q.hr_x_nrg.front() > maxx_zoom) ||
        (q.hr_x_nrg.back() < minx_zoom))
      continue;
    visible_roi_++;
    visible_peaks_ += q.peaks_.size();
    good_starts.insert(q.hr_x_nrg.front()); //for all parts of fit
    good_starts.insert(q.cal_nrg_.transform(q.finder_.x_.front(), q.bits_)); //for residues
    for (auto &p : q.peaks_) {
      if ((selected_peaks_.count(p.second.center) > 0) && (p.second.sum4_.peak_width > 0)) {
        good_starts.insert(fit_data_->nrg_cali_.transform(fit_data_->finder_.x_[p.second.sum4_.LB().start()], fit_data_->metadata_.bits));
        good_starts.insert(fit_data_->nrg_cali_.transform(fit_data_->finder_.x_[p.second.sum4_.RB().start()], fit_data_->metadata_.bits));
        if (!p.second.sum4_.bx.empty())
          good_starts.insert(fit_data_->nrg_cali_.transform(fit_data_->finder_.x_[p.second.sum4_.bx.front()], fit_data_->metadata_.bits));
      }
    }
  }

  bool plot_back_poly = (visible_roi_ < 4);
  bool plot_resid_on_back = plot_back_poly;
  bool plot_peak      = ((visible_roi_ < 4) || (visible_peaks_ < 10));
  bool plot_back_steps = ((visible_roi_ < 10) || plot_peak);
  bool plot_fullfit    = ((visible_roi_ < 15) || plot_peak);

  this->blockSignals(true);
  for (int i=0; i < ui->mcaPlot->itemCount(); ++i) {
    QCPAbstractItem *q =  ui->mcaPlot->item(i);
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      if (txt->property("chan_value").isValid()) {
        txt->setSelected(selected_peaks_.count(txt->property("chan_value").toDouble()));
      }
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("chan_value").isValid()) {
        line->setSelected(selected_peaks_.count(line->property("chan_value").toDouble()));
      }
    } else if (QCPItemPixmap *map = qobject_cast<QCPItemPixmap*>(q)) {
      if (map->property("ROI").isValid()) {
        map->setVisible(plot_peak && good_starts.count(map->property("ROI").toDouble()));
      }
    }
  }
  this->blockSignals(false);

  int total = ui->mcaPlot->graphCount();
  for (int i=0; i < total; i++) {
    if (ui->mcaPlot->graph(i)->name() == "Region fit")
      ui->mcaPlot->graph(i)->setVisible(plot_fullfit &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "Background steps")
      ui->mcaPlot->graph(i)->setVisible(plot_back_steps &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "Background poly")
      ui->mcaPlot->graph(i)->setVisible(plot_back_poly &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "Individual peak")
      ui->mcaPlot->graph(i)->setVisible(plot_peak &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "Residuals")
      ui->mcaPlot->graph(i)->setVisible(plot_resid_on_back &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "SUM4 edge")
      ui->mcaPlot->graph(i)->setVisible(plot_peak &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
    else if (ui->mcaPlot->graph(i)->name() == "SUM4 background")
      ui->mcaPlot->graph(i)->setVisible(plot_peak &&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0));
  }

}
