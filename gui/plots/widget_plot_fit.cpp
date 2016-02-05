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
  use_calibrated_ = false;

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

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->mcaPlot);
  connect(shortcut, SIGNAL(activated()), this, SLOT(zoom_out()));


  build_menu();

  replot_markers();
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
  minima_.clear();
  maxima_.clear();
  use_calibrated_ = false;
}

void WidgetPlotFit::clearExtras()
{
  //PL_DBG << "WidgetPlotFit::clearExtras()";
  my_markers_.clear();
  my_range_.visible = false;
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
  replot_markers();
}

void WidgetPlotFit::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

void WidgetPlotFit::use_calibrated(bool uc) {
  use_calibrated_ = uc;
  //replot_markers();
}

void WidgetPlotFit::set_markers(const std::list<Marker>& markers) {
  my_markers_ = markers;
}

void WidgetPlotFit::set_range(Range rng) {
  //  PL_DBG << "<WidgetPlotFit> set range";
  my_range_ = rng;
}

std::set<double> WidgetPlotFit::get_selected_markers() {
  std::set<double> selection;
  for (auto &q : ui->mcaPlot->selectedItems())
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      if (txt->property("chan_value").isValid())
        selection.insert(txt->property("chan_value").toDouble());
      //PL_DBG << "found selected " << txt->property("true_value").toDouble() << " chan=" << txt->property("chan_value").toDouble();
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("chan_value").isValid())
        selection.insert(line->property("chan_value").toDouble());
      //PL_DBG << "found selected " << line->property("true_value").toDouble() << " chan=" << line->property("chan_value").toDouble();
    }

  return selection;
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

  maxy = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(maxy) - 75);

  /*if ((maxy > 1) && (miny == 0))
    miny = 1;*/
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

void WidgetPlotFit::replot_markers() {
  ui->mcaPlot->clearItems();
  edge_trc1 = nullptr;
  edge_trc2 = nullptr;
  double min_marker = std::numeric_limits<double>::max();
  double max_marker = - std::numeric_limits<double>::max();
  //  int total_markers = 0;

  for (auto &q : my_markers_) {
    QCPItemTracer *top_crs = nullptr;
    if (q.visible) {

      double max = std::numeric_limits<double>::lowest();
      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {

        if (!ui->mcaPlot->graph(i)->property("fittable").toBool())
          continue;

        int bits = ui->mcaPlot->graph(i)->property("bits").toInt();

        double pos = 0;
        if (use_calibrated_)
          pos = q.pos.energy();
        else
          pos = q.pos.bin(bits);

        //PL_DBG << "Adding crs at " << pos << " on plot " << i;

        if ((ui->mcaPlot->graph(i)->data()->firstKey() >= pos)
            || (pos >= ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
        crs->setStyle(QCPItemTracer::tsNone); //tsCirlce?
        crs->setProperty("chan_value", q.pos.bin(bits));
        crs->setProperty("nrg_value", q.pos.energy());

        crs->setSize(4);
        crs->setGraph(ui->mcaPlot->graph(i));
        crs->setInterpolating(true);
        crs->setGraphKey(pos);
        crs->setPen(q.appearance.default_pen);
        crs->setSelectable(false);
        ui->mcaPlot->addItem(crs);

        crs->updatePosition();
        double val = crs->positions().first()->value();
        if (val > max) {
          max = val;
          top_crs = crs;
        }
      }
    }
    if (top_crs != nullptr) {
      QPen pen = q.appearance.default_pen;
      QPen selected_pen = q.selected_appearance.default_pen;

      if (q.selected) {
        //        total_markers++;
        if (top_crs->graphKey() > max_marker)
          max_marker = top_crs->graphKey();
        if (top_crs->graphKey() < min_marker)
          min_marker = top_crs->graphKey();
      }

      QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
      line->start->setParentAnchor(top_crs->position);
      line->start->setCoords(0, -30);
      line->end->setParentAnchor(top_crs->position);
      line->end->setCoords(0, -2);
      line->setHead(QCPLineEnding(QCPLineEnding::esLineArrow, 7, 7));
      line->setPen(pen);
      line->setSelectedPen(selected_pen);
      line->setProperty("true_value", top_crs->graphKey());
      line->setProperty("chan_value", top_crs->property("chan_value"));
      line->setProperty("nrg_value", top_crs->property("nrg_value"));
      line->setSelectable(true);
      line->setSelected(q.selected);
      ui->mcaPlot->addItem(line);

      QCPItemText *markerText = new QCPItemText(ui->mcaPlot);
      markerText->setProperty("true_value", top_crs->graphKey());
      markerText->setProperty("chan_value", top_crs->property("chan_value"));
      markerText->setProperty("nrg_value", top_crs->property("nrg_value"));

      markerText->position->setParentAnchor(top_crs->position);
      markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
      markerText->position->setCoords(0, -30);
      markerText->setText(QString::number(q.pos.energy()));
      markerText->setTextAlignment(Qt::AlignLeft);
      markerText->setFont(QFont("Helvetica", 9));
      markerText->setPen(pen);
      markerText->setColor(pen.color());
      markerText->setSelectedColor(selected_pen.color());
      markerText->setSelectedPen(selected_pen);
      markerText->setPadding(QMargins(1, 1, 1, 1));
      markerText->setSelectable(true);
      markerText->setSelected(q.selected);
      ui->mcaPlot->addItem(markerText);

    }

    //ui->mcaPlot->xAxis->setRangeLower(min_marker);
    //ui->mcaPlot->xAxis->setRangeUpper(max_marker);
    //ui->mcaPlot->xAxis->setRange(min, max);
    //ui->mcaPlot->xAxis2->setRange(min, max);

  }

  if (my_range_.visible) {

    double pos_l = 0, pos_c = 0, pos_r = 0;
    pos_l = my_range_.l.energy();
    pos_c = my_range_.center.energy();
    pos_r = my_range_.r.energy();

    int fit_level = 0;

    if (pos_l < pos_r) {

      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {

        int level = ui->mcaPlot->graph(i)->property("fittable").toInt();

        if (level < fit_level)
          continue;

        fit_level = level;

        int bits = ui->mcaPlot->graph(i)->property("bits").toInt();
        if (!use_calibrated_) {
          pos_l = my_range_.l.bin(bits);
          pos_c = my_range_.center.bin(bits);
          pos_r = my_range_.r.bin(bits);
        } else {
          pos_l = my_range_.l.energy();
          pos_c = my_range_.center.energy();
          pos_r = my_range_.r.energy();
        }

        if ((ui->mcaPlot->graph(i)->data()->firstKey() > pos_l)
            || (pos_r > ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        edge_trc1 = new QCPItemTracer(ui->mcaPlot);
        edge_trc1->setStyle(QCPItemTracer::tsNone);
        edge_trc1->setGraph(ui->mcaPlot->graph(i));
        edge_trc1->setGraphKey(pos_l);
        edge_trc1->setInterpolating(true);
        ui->mcaPlot->addItem(edge_trc1);
        edge_trc1->updatePosition();

        edge_trc2 = new QCPItemTracer(ui->mcaPlot);
        edge_trc2->setStyle(QCPItemTracer::tsNone);
        edge_trc2->setGraph(ui->mcaPlot->graph(i));
        edge_trc2->setGraphKey(pos_r);
        edge_trc2->setInterpolating(true);
        ui->mcaPlot->addItem(edge_trc2);
        edge_trc2->updatePosition();
      }

    } else {
      //      PL_DBG << "<WidgetPlotFit> bad range";
      my_range_.visible = false;
      //emit range_moved();
    }

    if (my_range_.visible && (fit_level > 0)) {

      QPen pen_l = my_range_.base.default_pen;
      QPen pen_r = my_range_.base.default_pen;
      DraggableTracer *ar1 = new DraggableTracer(ui->mcaPlot, edge_trc1, 10);
      pen_l.setWidth(1);
      ar1->setPen(pen_l);
      ar1->setSelectable(true);
      ar1->set_limits(minx - 1, maxx + 1); //exclusive limits
      ui->mcaPlot->addItem(ar1);

      DraggableTracer *ar2 = new DraggableTracer(ui->mcaPlot, edge_trc2, 10);
      pen_r.setWidth(1);
      ar2->setPen(pen_r);
      ar2->setSelectable(true);
      ar2->set_limits(minx - 1, maxx + 1); //exclusive limits
      ui->mcaPlot->addItem(ar2);

      if (my_range_.visible) {
        QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
        line->setSelectable(false);
        line->start->setParentAnchor(edge_trc1->position);
        line->start->setCoords(0, 0);
        line->end->setParentAnchor(edge_trc2->position);
        line->end->setCoords(0, 0);
        line->setPen(my_range_.base.default_pen);
        ui->mcaPlot->addItem(line);

        if (edge_trc1->graphKey() < min_marker)
          min_marker = edge_trc1->graphKey();
        if (edge_trc2->graphKey() > max_marker)
          max_marker = edge_trc2->graphKey();
      }
    }

  } else {
    //    PL_DBG << "<WidgetPlotFit> range invisible";
  }


  if (!title_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->mcaPlot);
    ui->mcaPlot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(title_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }

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

}


void WidgetPlotFit::plot_mouse_clicked(double x, double y, QMouseEvent* event, bool on_item) {
  if (event->button() == Qt::RightButton) {
    emit clickedRight(x);
  } else if (!on_item) { //tricky
    emit clickedLeft(x);
  }
}


void WidgetPlotFit::selection_changed() {
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
    }
  }
}

void WidgetPlotFit::zoom_out() {
  ui->mcaPlot->xAxis->rescale();
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
  calc_visible();
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

  QPen main_graph_, back_with_steps_, peak, back_poly_, sum4edge_,
       full_fit_, flagged,
       resid;

  QColor plotcolor;
  plotcolor.setHsv(QColor::fromRgba(fit_data_->metadata_.appearance).hsvHue(), 48, 160);
  main_graph_ = QPen(plotcolor, 1);

  back_poly_ = QPen(Qt::darkGray, 1);
  back_with_steps_ = QPen(Qt::darkBlue, 1);
  full_fit_ = QPen(Qt::red, 2);

  peak  = QPen(Qt::darkCyan, 2);
  peak.setStyle(Qt::DotLine);

  resid = QPen( Qt::darkGreen, 1);
  resid.setStyle(Qt::DashLine);

  QColor flagged_color;
  flagged_color.setHsv(QColor(Qt::green).hsvHue(), QColor(Qt::green).hsvSaturation(), 192);
  flagged =  QPen(flagged_color, 1);
  flagged.setStyle(Qt::DotLine);

  sum4edge_ = QPen(Qt::darkYellow, 3);

//  this->clearGraphs();
//  this->clearExtras();

  minima_.clear(); maxima_.clear();

  use_calibrated(fit_data_->nrg_cali_.valid());
  setLabels(QString::fromStdString(fit_data_->nrg_cali_.units_), "count");

  QVector<double> xx, yy;

  xx = QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(fit_data_->finder_.x_, fit_data_->metadata_.bits));
  yy = QVector<double>::fromStdVector(fit_data_->finder_.y_);

  addGraph(xx, yy, main_graph_, 1, fit_data_->metadata_.bits, "Data");
  add_bounds(xx, yy);

  for (auto &q : fit_data_->regions_) {
    xx = QVector<double>::fromStdVector(q.hr_x_nrg);

    yy = QVector<double>::fromStdVector(q.hr_fullfit);
    trim_log_lower(yy);
    addGraph(xx, yy, full_fit_, false, q.bits_, "Region fit");
    add_bounds(xx, yy);

    for (auto & p : q.peaks_) {
      yy = QVector<double>::fromStdVector(p.hr_fullfit_);
      trim_log_lower(yy);
      QPen prof = p.flagged ? flagged : peak;
      addGraph(xx, yy, prof, false, q.bits_, "Individual peak");

      if (p.sum4_.fwhm > 0) {

        std::vector<double> x_edge, y_edge;
        for (int i = p.sum4_.LBstart; i <= p.sum4_.LBend; ++i) {
          x_edge.push_back(fit_data_->finder_.x_[i]);
          y_edge.push_back(p.sum4_.Lsum / p.sum4_.Lw);
        }
        if (!x_edge.empty()) {
          x_edge[0] -= 0.5;
          x_edge[x_edge.size()-1] += 0.5;
        }

        addGraph(QVector<double>::fromStdVector(q.cal_nrg_.transform(x_edge, q.bits_)),
                             QVector<double>::fromStdVector(y_edge),
                             sum4edge_, false, q.bits_, "SUM4 edge");


        x_edge.clear(); y_edge.clear();

        for (int i = p.sum4_.RBstart; i <= p.sum4_.RBend; ++i) {
          x_edge.push_back(fit_data_->finder_.x_[i]);
          y_edge.push_back(p.sum4_.Rsum / p.sum4_.Rw);
        }
        if (!x_edge.empty()) {
          x_edge[0] -= 0.5;
          x_edge[x_edge.size()-1] += 0.5;
        }
        addGraph(QVector<double>::fromStdVector(q.cal_nrg_.transform(x_edge, q.bits_)),
                             QVector<double>::fromStdVector(y_edge),
                             sum4edge_, false, q.bits_, "SUM4 edge");

      }
    }

    if (!q.peaks_.empty()) {

      yy = QVector<double>::fromStdVector(q.hr_background);
      trim_log_lower(yy);
      addGraph(xx, yy, back_poly_, false, q.bits_, "Background poly");
      add_bounds(xx, yy);

      yy = QVector<double>::fromStdVector(q.hr_back_steps);
      trim_log_lower(yy);
      addGraph(xx, yy, back_with_steps_, false, q.bits_, "Background steps");

      xx = QVector<double>::fromStdVector(fit_data_->nrg_cali_.transform(q.finder_.x_, fit_data_->metadata_.bits));
      yy = QVector<double>::fromStdVector(q.finder_.y_resid_on_background_);
//      trim_log_lower(yy); //maybe not?
      addGraph(xx, yy, resid, 2, q.bits_, "Residuals");
//      add_bounds(xx, yy);
    }

  }

  calc_visible();
  rescale();
//  replot_markers();
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
    q.visible_ = false;
    if (q.hr_x_nrg.empty() ||
        (q.hr_x_nrg.front() > maxx_zoom) ||
        (q.hr_x_nrg.back() < minx_zoom))
      continue;
    visible_roi_++;
    visible_peaks_ += q.peaks_.size();
    good_starts.insert(q.hr_x_nrg.front());
    good_starts.insert(q.cal_nrg_.transform(q.finder_.x_.front(), q.bits_));
  }

  bool plot_back_poly = (visible_roi_ < 4);
  bool plot_resid_on_back = plot_back_poly;
  bool plot_peak      = ((visible_roi_ < 4) || (visible_peaks_ < 10));
  bool plot_back_steps = ((visible_roi_ < 10) || plot_peak);
  bool plot_fullfit    = ((visible_roi_ < 15) || plot_peak);

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
      ui->mcaPlot->graph(i)->setVisible(plot_peak /*&&
                                        (good_starts.count(ui->mcaPlot->graph(i)->property("start").toDouble()) > 0)*/);
  }

}
