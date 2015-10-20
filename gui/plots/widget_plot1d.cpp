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
 *      WidgetPlot1D -
 *
 ******************************************************************************/

#include "widget_plot1d.h"
#include "ui_widget_plot1d.h"
#include "custom_logger.h"
#include "qt_util.h"

WidgetPlot1D::WidgetPlot1D(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WidgetPlot1D)
{
  ui->setupUi(this);

  visible_options_  =
      (ShowOptions::style | ShowOptions::scale | ShowOptions::labels | ShowOptions::themes | ShowOptions::grid | ShowOptions::save);


  ui->mcaPlot->setInteraction(QCP::iSelectItems, true);
  ui->mcaPlot->setInteraction(QCP::iRangeDrag, true);
  ui->mcaPlot->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, true);
  ui->mcaPlot->setInteraction(QCP::iMultiSelect, true);
  ui->mcaPlot->yAxis->setPadding(28);

  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->mcaPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->mcaPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));

  minx = 0; maxx = 0;
  minx_zoom = 0; maxx_zoom = 0;
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  force_rezoom_ = false;
  mouse_pressed_ = false;
  use_calibrated_ = false;
  markers_selectable_ = false;

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;

  plot_style_ = "Lines";
  scale_type_ = "Logarithmic";
  color_theme_ = "light";
  grid_style_ = "Grid + subgrid";
  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));

  ui->mcaPlot->xAxis->grid()->setVisible(true);
  ui->mcaPlot->yAxis->grid()->setVisible(true);
  ui->mcaPlot->xAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->yAxis->grid()->setSubGridVisible(true);

  marker_labels_ = false;

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

WidgetPlot1D::~WidgetPlot1D()
{
  delete ui;
}

void WidgetPlot1D::set_visible_options(ShowOptions options) {
  visible_options_ = options;

  ui->mcaPlot->setInteraction(QCP::iRangeDrag, options & ShowOptions::zoom);
  ui->mcaPlot->setInteraction(QCP::iRangeZoom, options & ShowOptions::zoom);

  build_menu();
  replot_markers();
}

void WidgetPlot1D::build_menu() {
  menuOptions.clear();

  if (visible_options_ & ShowOptions::style) {
    menuOptions.addAction("Step");
    menuOptions.addAction("Lines");
    menuOptions.addAction("Scatter");
    menuOptions.addAction("Fill");
  }

  if (visible_options_ & ShowOptions::scale) {
    menuOptions.addSeparator();
    menuOptions.addAction("Linear");
    menuOptions.addAction("Logarithmic");
  }

  if (visible_options_ & ShowOptions::labels) {
    menuOptions.addSeparator();
    menuOptions.addAction("Energy labels");
  }

  if (visible_options_ & ShowOptions::themes) {
    menuOptions.addSeparator();
    menuOptions.addAction("dark");
    menuOptions.addAction("light");
  }

  if (visible_options_ & ShowOptions::grid) {
    menuOptions.addSeparator();
    menuOptions.addAction("No grid");
    menuOptions.addAction("Grid");
    menuOptions.addAction("Grid + subgrid");
  }

  for (auto &q : menuOptions.actions()) {
    q->setCheckable(true);
    q->setChecked((q->text() == scale_type_) ||
                  (q->text() == plot_style_) ||
                  (q->text() == color_theme_) ||
                  (q->text() == grid_style_) ||
                  ((q->text() == "Energy labels") && marker_labels_));
  }
}


void WidgetPlot1D::clearGraphs()
{
  ui->mcaPlot->clearGraphs();
  minima_.clear();
  maxima_.clear();
  use_calibrated_ = false;
}

void WidgetPlot1D::clearExtras()
{
  my_markers_.clear();
  my_cursors_.clear();
  my_range_.visible = false;
  rect.clear();
}

void WidgetPlot1D::rescale() {
  force_rezoom_ = true;
  plot_rezoom();
}

void WidgetPlot1D::redraw() {
  ui->mcaPlot->replot();
}

void WidgetPlot1D::reset_scales()
{
  minx = maxx = 0;
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();
  ui->mcaPlot->rescaleAxes();
}

void WidgetPlot1D::setTitle(QString title) {
  title_text_ = title;
  replot_markers();
}

void WidgetPlot1D::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

void WidgetPlot1D::use_calibrated(bool uc) {
  use_calibrated_ = uc;
  replot_markers();
}

void WidgetPlot1D::set_markers(const std::list<Marker>& markers) {
  my_markers_ = markers;
}

void WidgetPlot1D::set_block(Marker a, Marker b) {
  rect.resize(2);
  rect[0] = a;
  rect[1] = b;
}

void WidgetPlot1D::set_cursors(const std::list<Marker>& cursors) {
  my_cursors_ = cursors;
}

void WidgetPlot1D::set_range(Range rng) {
  my_range_ = rng;
}

Range WidgetPlot1D::get_range() {
  return my_range_;
}

std::set<double> WidgetPlot1D::get_selected_markers() {
  std::set<double> selection;
  for (auto &q : ui->mcaPlot->selectedItems())
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      selection.insert(txt->property("chan_value").toDouble());
      //PL_DBG << "found selected " << txt->property("true_value").toDouble() << " chan=" << txt->property("chan_value").toDouble();
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("chan_value").isValid())
        selection.insert(line->property("chan_value").toDouble());
    }

  return selection;
}


void WidgetPlot1D::setYBounds(const std::map<double, double> &minima, const std::map<double, double> &maxima) {
  minima_ = minima;
  maxima_ = maxima;
  rescale();
}


void WidgetPlot1D::addGraph(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, bool fittable) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  ui->mcaPlot->graph(g)->setPen(appearance.get_pen(color_theme_));
  ui->mcaPlot->graph(g)->setProperty("fittable", fittable);
  set_graph_style(ui->mcaPlot->graph(g), plot_style_);

  if (x[0] < minx) {
    minx = x[0];
    ui->mcaPlot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
    ui->mcaPlot->xAxis->rescale();
  }
}

void WidgetPlot1D::addPoints(const QVector<double>& x, const QVector<double>& y, AppearanceProfile appearance, QCPScatterStyle::ScatterShape shape) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->mcaPlot->addGraph();
  int g = ui->mcaPlot->graphCount() - 1;
  ui->mcaPlot->graph(g)->addData(x, y);
  ui->mcaPlot->graph(g)->setPen(appearance.get_pen((color_theme_)));
  ui->mcaPlot->graph(g)->setBrush(QBrush());
  ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(shape, appearance.get_pen(color_theme_).color(), appearance.get_pen(color_theme_).color(), appearance.get_pen(color_theme_).width()));
  ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);

  if (x[0] < minx) {
    minx = x[0];
    ui->mcaPlot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
    ui->mcaPlot->xAxis->rescale();
  }
}


void WidgetPlot1D::plot_rezoom() {
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
  force_rezoom_ = false;

  calc_y_bounds(lowerc, upperc);

  //PL_DBG << "Rezoom";

  if (miny <= 0)
    ui->mcaPlot->yAxis->rescale();
  else
    ui->mcaPlot->yAxis->setRangeLower(miny);
  ui->mcaPlot->yAxis->setRangeUpper(maxy);
}

void WidgetPlot1D::calc_y_bounds(double lower, double upper) {
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

void WidgetPlot1D::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
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

void WidgetPlot1D::replot_markers() {
  ui->mcaPlot->clearItems();
  edge_trc1 = nullptr;
  edge_trc2 = nullptr;
  double min_marker = std::numeric_limits<double>::max();
  double max_marker = - std::numeric_limits<double>::max();
  int total_markers = 0;

  for (auto &q : my_markers_) {
    QCPItemTracer *top_crs = nullptr;
    if (q.visible) {
      double pos = 0;
      if (use_calibrated_)
        pos = q.energy;
      else
        pos = q.channel;

      double max = std::numeric_limits<double>::lowest();
      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {
        if (ui->mcaPlot->graph(i)->scatterStyle().shape() != QCPScatterStyle::ssNone)
          continue;

        if ((ui->mcaPlot->graph(i)->data()->firstKey() >= pos)
            || (pos >= ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
        crs->setStyle(QCPItemTracer::tsNone); //tsCirlce?
        crs->setProperty("chan_value", q.channel);

        crs->setSize(4);
        crs->setGraph(ui->mcaPlot->graph(i));
        crs->setGraphKey(pos);
        crs->setInterpolating(true);
        crs->setPen(q.appearance.get_pen(color_theme_));
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
      QPen pen = q.appearance.get_pen(color_theme_);
      QPen selected_pen = q.selected_appearance.get_pen(color_theme_);

      if (q.selected) {
        total_markers++;
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
      line->setSelectable(markers_selectable_);
      if (markers_selectable_)
        line->setSelected(q.selected);
      ui->mcaPlot->addItem(line);

      if (marker_labels_) {
        QCPItemText *markerText = new QCPItemText(ui->mcaPlot);
        markerText->setProperty("true_value", top_crs->graphKey());
        markerText->setProperty("chan_value", top_crs->property("chan_value"));

        markerText->position->setParentAnchor(top_crs->position);
        markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
        markerText->position->setCoords(0, -30);
        markerText->setText(QString::number(q.energy));
        markerText->setTextAlignment(Qt::AlignLeft);
        markerText->setFont(QFont("Helvetica", 9));
        markerText->setPen(pen);
        markerText->setColor(pen.color());
        markerText->setSelectedColor(selected_pen.color());
        markerText->setSelectedPen(selected_pen);
        markerText->setPadding(QMargins(1, 1, 1, 1));
        markerText->setSelectable(markers_selectable_);
        if (markers_selectable_)
          markerText->setSelected(q.selected);
        ui->mcaPlot->addItem(markerText);
      }
    }

    //ui->mcaPlot->xAxis->setRangeLower(min_marker);
    //ui->mcaPlot->xAxis->setRangeUpper(max_marker);
    //ui->mcaPlot->xAxis->setRange(min, max);
    //ui->mcaPlot->xAxis2->setRange(min, max);

  }

  for (auto &q : my_cursors_) {
    double pos = 0;
    if (use_calibrated_)
      pos = q.energy;
    else
      pos = q.channel;

    if (!q.visible)
      continue;
    int total = ui->mcaPlot->graphCount();
    for (int i=0; i < total; i++) {
      QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
      crs->setPen(q.appearance.get_pen(color_theme_));
      crs->setStyle(QCPItemTracer::tsCircle);
      crs->setSize(4);
      crs->setGraph(ui->mcaPlot->graph(i));
      crs->setGraphKey(pos);
      crs->setInterpolating(true);
      crs->setSelectable(false);
      ui->mcaPlot->addItem(crs);
    }
  }

  if (my_range_.visible) {

    double pos_l = 0, pos_c = 0, pos_r = 0;
    if (use_calibrated_) {
      pos_l = my_range_.l.energy;
      pos_c = my_range_.center.energy;
      pos_r = my_range_.r.energy;
    } else {
      pos_l = my_range_.l.channel;
      pos_c = my_range_.center.channel;
      pos_r = my_range_.r.channel;
    }

    if ((pos_l < pos_c) && (pos_c < pos_r)) {

      int total = ui->mcaPlot->graphCount();
      for (int i=0; i < total; i++) {
        if (!ui->mcaPlot->graph(i)->property("fittable").toBool())
          continue;

        if (ui->mcaPlot->graph(i)->scatterStyle().shape() != QCPScatterStyle::ssNone)
          continue;

        if ((ui->mcaPlot->graph(i)->data()->firstKey() > pos_l)
            || (pos_r > ui->mcaPlot->graph(i)->data()->lastKey()))
          continue;

        QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
        crs->setStyle(QCPItemTracer::tsNone);
        crs->setGraph(ui->mcaPlot->graph(i));
        crs->setGraphKey(pos_c);
        crs->setInterpolating(true);
        crs->setSelectable(false);
        ui->mcaPlot->addItem(crs);
        crs->updatePosition();
        double center_val = crs->positions().first()->value();

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

        QPen pen_l = my_range_.l.appearance.get_pen(color_theme_);
        DraggableTracer *ar1 = new DraggableTracer(ui->mcaPlot, edge_trc1, pen_l.width());
        pen_l.setWidth(1);
        ar1->setPen(pen_l);
        ar1->setSelectable(true);
        ar1->set_limits(minx - 1, pos_r + 1); //exclusive limits
        ui->mcaPlot->addItem(ar1);

        QPen pen_r = my_range_.r.appearance.get_pen(color_theme_);
        DraggableTracer *ar2 = new DraggableTracer(ui->mcaPlot, edge_trc2, pen_r.width());
        pen_r.setWidth(1);
        ar2->setPen(pen_r);
        ar2->setSelectable(true);
        ar2->set_limits(pos_l - 1, maxx + 1); //exclusive limits
        ui->mcaPlot->addItem(ar2);

        if ((my_range_.l.visible) && (my_range_.r.visible)) {
          QCPItemLine *line = new QCPItemLine(ui->mcaPlot);
          line->start->setParentAnchor(edge_trc1->position);
          line->start->setCoords(0, 0);
          line->end->setParentAnchor(edge_trc2->position);
          line->end->setCoords(0, 0);
          line->setPen(my_range_.base.get_pen(color_theme_));
          ui->mcaPlot->addItem(line);

          if (edge_trc1->graphKey() < min_marker)
            min_marker = edge_trc1->graphKey();
          if (edge_trc2->graphKey() > max_marker)
            max_marker = edge_trc2->graphKey();
        }

        if (my_range_.center.visible) {
          crs->setPen(my_range_.center.appearance.get_pen(color_theme_));
          crs->setStyle(QCPItemTracer::tsCircle);

          /*
          if (my_range_.l.visible) {
            QCPItemCurve *ln = new QCPItemCurve(ui->mcaPlot);
            ln->start->setParentAnchor(crs->position);
            ln->start->setCoords(0, 0);
            ln->end->setParentAnchor(edge_trc1->position);
            ln->end->setCoords(0, 0);
            ln->setPen(my_range_.top.get_pen(color_theme_));
            ui->mcaPlot->addItem(ln);
          }

          if (my_range_.r.visible) {
            QCPItemCurve *ln = new QCPItemCurve(ui->mcaPlot);
            ln->start->setParentAnchor(crs->position);
            ln->start->setCoords(0, 0);
            ln->end->setParentAnchor(edge_trc2->position);
            ln->end->setCoords(0, 0);
            ln->setPen(my_range_.top.get_pen(color_theme_));
            ui->mcaPlot->addItem(ln);
          } */
        }
      }

    } else {
      my_range_.visible = false;
      emit range_moved();
    }
  }


  if ((rect.size() == 2) && (rect[0].visible) && !maxima_.empty() && !minima_.empty()){
    double upperc = maxima_.rbegin()->first;
    double lowerc = maxima_.begin()->first;

    calc_y_bounds(lowerc, upperc);

    double pos1 = 0, pos2 = 0;
    if (use_calibrated_) {
      pos1 = rect[0].energy;
      pos2 = rect[1].energy;
    } else {
      pos1 = rect[0].channel;
      pos2 = rect[1].channel;
    }


    QCPItemRect *cprect = new QCPItemRect(ui->mcaPlot);
    double x1 = pos1;
    double y1 = maxy;
    double x2 = pos2;
    double y2 = miny;

    cprect->topLeft->setCoords(x1, y1);
    cprect->bottomRight->setCoords(x2, y2);
    cprect->setPen(rect[0].appearance.get_pen(color_theme_));
    cprect->setBrush(QBrush(rect[1].appearance.get_pen(color_theme_).color()));
    cprect->setSelectable(false);
    ui->mcaPlot->addItem(cprect);
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
    if (color_theme_ == "light")
      floatingText->setColor(Qt::black);
    else
      floatingText->setColor(Qt::white);
  }

  QCPItemPixmap *overlayButton;

  overlayButton = new QCPItemPixmap(ui->mcaPlot);
  overlayButton->setClipToAxisRect(false);
  overlayButton->setPixmap(QPixmap(":/new/icons/oxy/view_fullscreen.png"));
  overlayButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  overlayButton->topLeft->setCoords(5, 5);
  overlayButton->bottomRight->setParentAnchor(overlayButton->topLeft);
  overlayButton->bottomRight->setCoords(18, 18);
  overlayButton->setScaled(true);
  overlayButton->setSelectable(false);
  overlayButton->setProperty("button_name", QString("reset_scales"));
  overlayButton->setProperty("tooltip", QString("Reset plot scales"));
  ui->mcaPlot->addItem(overlayButton);

  if (!menuOptions.isEmpty()) {
    QCPItemPixmap *newButton = new QCPItemPixmap(ui->mcaPlot);
    newButton->setClipToAxisRect(false);
    newButton->setPixmap(QPixmap(":/new/icons/oxy/view_statistics.png"));
    newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    newButton->bottomRight->setParentAnchor(newButton->topLeft);
    newButton->bottomRight->setCoords(18, 18);
    newButton->setScaled(false);
    newButton->setSelectable(false);
    newButton->setProperty("button_name", QString("options"));
    newButton->setProperty("tooltip", QString("Style options"));
    ui->mcaPlot->addItem(newButton);
    overlayButton = newButton;
  }

  if (visible_options_ & ShowOptions::save) {
    QCPItemPixmap *newButton = new QCPItemPixmap(ui->mcaPlot);
    newButton->setClipToAxisRect(false);
    newButton->setPixmap(QPixmap(":/new/icons/oxy/document_save.png"));
    newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    newButton->bottomRight->setParentAnchor(newButton->topLeft);
    newButton->bottomRight->setCoords(18, 18);
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


void WidgetPlot1D::plot_mouse_clicked(double x, double y, QMouseEvent* event, bool on_item) {
    if (event->button() == Qt::RightButton) {
      emit clickedRight(x);
    } else if (!on_item) { //tricky
      emit clickedLeft(x);
    }
}


void WidgetPlot1D::selection_changed() {
  emit markers_selected();
}

void WidgetPlot1D::clicked_plottable(QCPAbstractPlottable *plt) {
//  PL_INFO << "<WidgetPlot1D> clickedplottable";
}

void WidgetPlot1D::clicked_item(QCPAbstractItem* itm) {
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

void WidgetPlot1D::zoom_out() {
  ui->mcaPlot->xAxis->rescale();
  force_rezoom_ = true;
  plot_rezoom();
  ui->mcaPlot->replot();

}

void WidgetPlot1D::plot_mouse_press(QMouseEvent*) {
  disconnect(ui->mcaPlot, 0, this, 0);
  connect(ui->mcaPlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->mcaPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->mcaPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  force_rezoom_ = false;
  mouse_pressed_ = true;
}

void WidgetPlot1D::plot_mouse_release(QMouseEvent*) {
  connect(ui->mcaPlot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->mcaPlot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->mcaPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));
  force_rezoom_ = true;
  mouse_pressed_ = false;
  plot_rezoom();
  ui->mcaPlot->replot();

  //channels only???
  if (edge_trc1 != nullptr)
    if (use_calibrated_) {
      my_range_.l.energy = edge_trc1->graphKey();
      my_range_.l.chan_valid = false;
    } else {
      my_range_.l.channel = edge_trc1->graphKey();
      my_range_.l.energy_valid = false;
    }
  if (edge_trc2 != nullptr)
    if (use_calibrated_) {
      my_range_.r.energy = edge_trc2->graphKey();
      my_range_.r.chan_valid = false;
    } else {
      my_range_.r.channel = edge_trc2->graphKey();
      my_range_.r.energy_valid = false;
    }

  if ((edge_trc1 != nullptr) || (edge_trc2 != nullptr))
    emit range_moved();
}

void WidgetPlot1D::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (choice == "Linear") {
    scale_type_ = choice;
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "Logarithmic") {
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    scale_type_ = choice;
  } else if ((choice == "Scatter") || (choice == "Lines") || (choice == "Fill") || (choice == "Step")) {
    plot_style_ = choice;
    int total = ui->mcaPlot->graphCount();
    for (int i=0; i < total; i++)
      if ((ui->mcaPlot->graph(i)->scatterStyle().shape() == QCPScatterStyle::ssNone) || (ui->mcaPlot->graph(i)->scatterStyle().shape() == QCPScatterStyle::ssDisc))
        set_graph_style(ui->mcaPlot->graph(i), choice);
  } else if (choice == "Energy labels") {
    marker_labels_ = !marker_labels_;
    replot_markers();
  } else if (choice == "light") {
    setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));
    color_theme_ = choice;
    replot_markers();
  } else if (choice == "dark") {
    setColorScheme(Qt::white, Qt::black, QColor(144, 144, 144), QColor(80, 80, 80));
    color_theme_ = choice;
    replot_markers();
  } else if ((choice == "No grid") || (choice == "Grid") || (choice == "Grid + subgrid")) {
    grid_style_ = choice;
    ui->mcaPlot->xAxis->grid()->setVisible(grid_style_ != "No grid");
    ui->mcaPlot->yAxis->grid()->setVisible(grid_style_ != "No grid");
    ui->mcaPlot->xAxis->grid()->setSubGridVisible(grid_style_ == "Grid + subgrid");
    ui->mcaPlot->yAxis->grid()->setSubGridVisible(grid_style_ == "Grid + subgrid");
  }

  build_menu();
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

QString WidgetPlot1D::scale_type() {
  return scale_type_;
}

QString WidgetPlot1D::plot_style() {
  return plot_style_;
}

void WidgetPlot1D::set_graph_style(QCPGraph* graph, QString style) {
  if (graph->pen().width() != 1) {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Fill") {
    graph->setBrush(QBrush(graph->pen().color()));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Lines") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Step") {
    graph->setBrush(QBrush());
    graph->setLineStyle(QCPGraph::lsStepCenter);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
  } else if (style == "Scatter") {
    graph->setBrush(QBrush());
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 1));
    graph->setLineStyle(QCPGraph::lsNone);
  }
}

void WidgetPlot1D::set_scale_type(QString sct) {
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

void WidgetPlot1D::set_plot_style(QString stl) {
  this->setCursor(Qt::WaitCursor);
  plot_style_ = stl;
  int total = ui->mcaPlot->graphCount();
  for (int i=0; i < total; i++) {
    if ((ui->mcaPlot->graph(i)->scatterStyle().shape() == QCPScatterStyle::ssNone) || (ui->mcaPlot->graph(i)->scatterStyle().shape() == QCPScatterStyle::ssDisc))
      set_graph_style(ui->mcaPlot->graph(i), stl);
  }
  build_menu();
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void WidgetPlot1D::set_marker_labels(bool sl)
{
  marker_labels_ = sl;
  replot_markers();
  build_menu();
  ui->mcaPlot->replot();
}

bool WidgetPlot1D::marker_labels() {
  return marker_labels_;
}

void WidgetPlot1D::set_markers_selectable(bool s) {
  markers_selectable_ = s;
}

void WidgetPlot1D::exportRequested(QAction* choice) {
  QString filter = choice->text() + "(*." + choice->text() + ")";
  QString fileName = CustomSaveFileDialog(this, "Export plot",
                                          QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                          filter);
  if (validateFile(this, fileName, true)) {
    QFileInfo file(fileName);
    if (file.suffix() == "png") {
      PL_INFO << "Exporting plot to png " << fileName.toStdString();
      ui->mcaPlot->savePng(fileName,0,0,1,100);
    } else if (file.suffix() == "jpg") {
      PL_INFO << "Exporting plot to jpg " << fileName.toStdString();
      ui->mcaPlot->saveJpg(fileName,0,0,1,100);
    } else if (file.suffix() == "bmp") {
      PL_INFO << "Exporting plot to bmp " << fileName.toStdString();
      ui->mcaPlot->saveBmp(fileName);
    } else if (file.suffix() == "pdf") {
      PL_INFO << "Exporting plot to pdf " << fileName.toStdString();
      ui->mcaPlot->savePdf(fileName, true);
    }
  }
}
