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
 *      WidgetPlotCalib - 
 *
 ******************************************************************************/

#include "widget_plot_calib.h"
#include "ui_widget_plot_calib.h"
#include "custom_logger.h"
#include "qt_util.h"

WidgetPlotCalib::WidgetPlotCalib(QWidget *parent) :
  QWidget(parent),  ui(new Ui::WidgetPlotCalib)
{
  ui->setupUi(this);

  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));

  ui->mcaPlot->setInteraction(QCP::iSelectItems, true);
  ui->mcaPlot->setInteraction(QCP::iMultiSelect, true);

  connect(ui->mcaPlot, SIGNAL(selectionChangedByUser()), this, SLOT(update_selection()));
  connect(ui->mcaPlot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));

  scale_type_x_ = "Linear";
  scale_type_y_ = "Linear";
  ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  ui->mcaPlot->yAxis2->setScaleType(QCPAxis::stLinear);
  ui->mcaPlot->xAxis->setScaleType(QCPAxis::stLinear);
  ui->mcaPlot->xAxis2->setScaleType(QCPAxis::stLinear);

  connect(&menuOptions, SIGNAL(triggered(QAction*)), this, SLOT(optionsChanged(QAction*)));
  build_menu();

  redraw();
}

WidgetPlotCalib::~WidgetPlotCalib()
{
  delete ui;
}

void WidgetPlotCalib::build_menu() {
  menuOptions.clear();

  menuOptions.addAction("X linear");
  menuOptions.addAction("X logarithmic");
  menuOptions.addSeparator();
  menuOptions.addAction("Y linear");
  menuOptions.addAction("Y logarithmic");
  menuOptions.addSeparator();
  menuOptions.addAction("png");
  menuOptions.addAction("jpg");
  menuOptions.addAction("pdf");
  menuOptions.addAction("bmp");

  for (auto &q : menuOptions.actions()) {
    q->setCheckable(true);
    if ((q->text() == "X linear") && (scale_type_x_ == "Linear"))
      q->setChecked(true);
    if ((q->text() == "X logarithmic") && (scale_type_x_ == "Logarithmic"))
      q->setChecked(true);
    if ((q->text() == "Y linear") && (scale_type_y_ == "Linear"))
      q->setChecked(true);
    if ((q->text() == "Y logarithmic") && (scale_type_y_ == "Logarithmic"))
      q->setChecked(true);
  }

}

QString WidgetPlotCalib::scale_type_x() {
  return scale_type_x_;
}

QString WidgetPlotCalib::scale_type_y() {
  return scale_type_y_;
}

void WidgetPlotCalib::set_scale_type_x(QString sct) {
  scale_type_x_ = sct;
  if (scale_type_x_ == "Linear")
    ui->mcaPlot->xAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type_x_ == "Logarithmic")
    ui->mcaPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->mcaPlot->replot();
  build_menu();
}

void WidgetPlotCalib::set_scale_type_y(QString sct) {
  scale_type_y_ = sct;
  if (scale_type_y_ == "Linear")
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type_y_ == "Logarithmic")
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->mcaPlot->replot();
  build_menu();
}

void WidgetPlotCalib::clearPoints()
{
  x_pts.clear();
  y_pts.clear();
  style_pts.clear();
  selection_.clear();
}

void WidgetPlotCalib::clearGraphs()
{
  x_fit.clear();
  y_fit.clear();
  x_pts.clear();
  y_pts.clear();
  style_pts.clear();
  selection_.clear();
  redraw();
}

void WidgetPlotCalib::redraw() {
  ui->mcaPlot->clearGraphs();
  ui->mcaPlot->clearItems();

  double xmin, xmax;
  double ymin, ymax;
  xmin = std::numeric_limits<double>::max();
  xmax = - std::numeric_limits<double>::max();
  ymin = std::numeric_limits<double>::max();
  ymax = - std::numeric_limits<double>::max();

  for (int k=0; k < x_pts.size(); ++k) {
    ui->mcaPlot->addGraph();
    int g = ui->mcaPlot->graphCount() - 1;
    ui->mcaPlot->graph(g)->addData(x_pts[k], y_pts[k]);
    ui->mcaPlot->graph(g)->setPen(QPen(Qt::darkCyan));
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, Qt::black, Qt::black, 0));
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsNone);

    for (int i = 0; i < x_pts[k].size(); ++i) {
      QCPItemTracer *crs = new QCPItemTracer(ui->mcaPlot);
      crs->setStyle(QCPItemTracer::tsSquare); //tsCirlce?
      crs->setSize(style_pts[k].default_pen.width());
      crs->setGraph(ui->mcaPlot->graph(g));
      crs->setGraphKey(x_pts[k][i]);
      crs->setInterpolating(true);
      crs->setPen(QPen(style_pts[k].default_pen.color(), 0));
      crs->setBrush(style_pts[k].default_pen.color());
      crs->setSelectedPen(QPen(style_pts[k].themes["selected"].color(), 0));
      crs->setSelectedBrush(style_pts[k].themes["selected"].color());
      crs->setSelectable(true);
      crs->setProperty("true_value", x_pts[k][i]);
      if (selection_.count(x_pts[k][i]) > 0)
        crs->setSelected(true);
      ui->mcaPlot->addItem(crs);
      crs->updatePosition();
    }

    for (auto &q : x_pts[k]) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }
    for (auto &q : y_pts[k]) {
      if (q < ymin)
        ymin = q;
      if (q > ymax)
        ymax = q;
    }
  }

  ui->mcaPlot->rescaleAxes();

  xmax = ui->mcaPlot->xAxis->pixelToCoord(ui->mcaPlot->xAxis->coordToPixel(xmax) + 15);
  xmin = ui->mcaPlot->xAxis->pixelToCoord(ui->mcaPlot->xAxis->coordToPixel(xmin) - 15);

  if (!x_fit.empty()) {
    ui->mcaPlot->addGraph();
    int g = ui->mcaPlot->graphCount() - 1;
    ui->mcaPlot->graph(g)->addData(x_fit, y_fit);
    ui->mcaPlot->graph(g)->setPen(style_fit.default_pen);
    ui->mcaPlot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->mcaPlot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);

    for (auto &q : x_fit) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }
    for (auto &q : y_fit) {
      if (q < ymin)
        ymin = q;
      if (q > ymax)
        ymax = q;
    }
  }

  ui->mcaPlot->rescaleAxes();
  
  ymax = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(ymax) - 60);
  ymin = ui->mcaPlot->yAxis->pixelToCoord(ui->mcaPlot->yAxis->coordToPixel(ymin) + 15);

  if (!floating_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->mcaPlot);
    ui->mcaPlot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(floating_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }

  QCPItemPixmap *overlayButton;

  overlayButton = new QCPItemPixmap(ui->mcaPlot);
  overlayButton->setClipToAxisRect(false);
  overlayButton->setPixmap(QPixmap(":/icons/oxy/16/view_statistics.png"));
  overlayButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  overlayButton->topLeft->setCoords(5, 5);
  overlayButton->bottomRight->setParentAnchor(overlayButton->topLeft);
  overlayButton->bottomRight->setCoords(16, 16);
  overlayButton->setScaled(true);
  overlayButton->setSelectable(false);
  overlayButton->setProperty("button_name", QString("options"));
  overlayButton->setProperty("tooltip", QString("Style options"));
  ui->mcaPlot->addItem(overlayButton);


  if ((scale_type_x_ == "Logarithmic") && (xmin < 1))
    xmin = 1;

  if ((scale_type_y_ == "Logarithmic") && (ymin < 0))
    ymin = 0;

  if (this->isVisible()) {
    ui->mcaPlot->xAxis->setRange(xmin, xmax);
    ui->mcaPlot->xAxis2->setRange(xmin, xmax);
    ui->mcaPlot->yAxis->setRange(ymin, ymax);
    ui->mcaPlot->yAxis2->setRange(ymin, ymax);
  }

  ui->mcaPlot->replot();
}

void WidgetPlotCalib::clicked_item(QCPAbstractItem* itm) {
  if (QCPItemPixmap *pix = qobject_cast<QCPItemPixmap*>(itm)) {
//    QPoint p = this->mapFromGlobal(QCursor::pos());
    QString name = pix->property("button_name").toString();
    if (name == "options") {
      menuOptions.exec(QCursor::pos());
    }
  }
}


void WidgetPlotCalib::setLabels(QString x, QString y) {
  ui->mcaPlot->xAxis->setLabel(x);
  ui->mcaPlot->yAxis->setLabel(y);
}

std::set<double> WidgetPlotCalib::get_selected_pts() {
  return selection_;
}

void WidgetPlotCalib::addFit(const QVector<double>& x, const QVector<double>& y, AppearanceProfile style) {
  style_fit = style;
  x_fit.clear();
  y_fit.clear();

  if (!x.empty() && (x.size() == y.size())) {
    x_fit = x;
    y_fit = y;
  }

  redraw();
}

void WidgetPlotCalib::addPoints(const QVector<double>& x, const QVector<double>& y, AppearanceProfile style) {
  if (!x.empty() && (x.size() == y.size())) {
    x_pts.push_back(x);
    y_pts.push_back(y);
    style_pts.push_back(style);
  }

  redraw();
}

void WidgetPlotCalib::set_selected_pts(std::set<double> sel) {
  selection_ = sel;
  redraw();
}

void WidgetPlotCalib::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
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
  ui->mcaPlot->xAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->yAxis->grid()->setSubGridVisible(true);
  ui->mcaPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->mcaPlot->setBackground(QBrush(back));
}

void WidgetPlotCalib::update_selection() {
  selection_.clear();
  for (auto &q : ui->mcaPlot->selectedItems())
    if (QCPItemTracer *txt = qobject_cast<QCPItemTracer*>(q))
      selection_.insert(txt->property("true_value").toDouble());

  emit selection_changed();
}

void WidgetPlotCalib::setFloatingText(QString txt) {
  floating_text_ = txt;
  redraw();
}

void WidgetPlotCalib::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (choice == "X linear") {
    scale_type_x_ = "Linear";
    ui->mcaPlot->xAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "X logarithmic") {
    scale_type_x_ = "Logarithmic";
    ui->mcaPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
  } else if (choice == "Y linear") {
    scale_type_y_ = "Linear";
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "Y logarithmic") {
    scale_type_y_ = "Logarithmic";
    ui->mcaPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  } else {
    QString filter = action->text() + "(*." + action->text() + ")";
    QString fileName = CustomSaveFileDialog(this, "Export plot",
                                            QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                            filter);
    if (validateFile(this, fileName, true)) {
      QFileInfo file(fileName);
      if (file.suffix() == "png") {
        ui->mcaPlot->savePng(fileName,0,0,1,100);
      } else if (file.suffix() == "jpg") {
        ui->mcaPlot->saveJpg(fileName,0,0,1,100);
      } else if (file.suffix() == "bmp") {
        ui->mcaPlot->saveBmp(fileName);
      } else if (file.suffix() == "pdf") {
        ui->mcaPlot->savePdf(fileName, true);
      }
    }

  }

  build_menu();
  ui->mcaPlot->replot();
  this->setCursor(Qt::ArrowCursor);
}
