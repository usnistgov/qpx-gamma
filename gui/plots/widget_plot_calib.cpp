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

  ui->plot->setInteraction(QCP::iSelectItems, true);
  ui->plot->setInteraction(QCP::iMultiSelect, true);

  connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(update_selection()));
  connect(ui->plot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));

  scale_type_x_ = "Linear";
  scale_type_y_ = "Linear";
  ui->plot->yAxis->setScaleType(QCPAxis::stLinear);
  ui->plot->yAxis2->setScaleType(QCPAxis::stLinear);
  ui->plot->xAxis->setScaleType(QCPAxis::stLinear);
  ui->plot->xAxis2->setScaleType(QCPAxis::stLinear);

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
    ui->plot->xAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type_x_ == "Logarithmic")
    ui->plot->xAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->plot->replot();
  build_menu();
}

void WidgetPlotCalib::set_scale_type_y(QString sct) {
  scale_type_y_ = sct;
  if (scale_type_y_ == "Linear")
    ui->plot->yAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type_y_ == "Logarithmic")
    ui->plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->plot->replot();
  build_menu();
}

void WidgetPlotCalib::clear_data() {
  points_.clear();
  x_fit.clear();
  y_fit.clear();

  selection_.clear();
  floating_text_.clear();
}

void WidgetPlotCalib::clearPoints()
{
  points_.clear();
  selection_.clear();
}

void WidgetPlotCalib::clearGraphs()
{
  x_fit.clear();
  y_fit.clear();
  points_.clear();
  selection_.clear();
  redraw();
}

void WidgetPlotCalib::redraw() {
  ui->plot->clearGraphs();
  ui->plot->clearItems();
  ui->plot->rescaleAxes();

  double xmin, xmax;
  double ymin, ymax;
  xmin = std::numeric_limits<double>::max();
  xmax = - std::numeric_limits<double>::max();
  ymin = std::numeric_limits<double>::max();
  ymax = - std::numeric_limits<double>::max();

  for (const PointSet &p : points_) {
    if ((p.y.size() != p.x.size()) || p.x.isEmpty())
      continue;

    ui->plot->addGraph();
    int g = ui->plot->graphCount() - 1;
    QPen pen(p.appearance.themes["selected"].color(), 0);

    ui->plot->graph(g)->setPen(pen);
    ui->plot->graph(g)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(g)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 0));
    ui->plot->graph(g)->setErrorPen(pen);
    if ((p.y_sigma.size() == p.y.size()) && (p.x_sigma.size() == p.x.size()))
    {
      ui->plot->graph(g)->setErrorType(QCPGraph::etBoth);
      ui->plot->graph(g)->setDataBothError(p.x, p.y, p.x_sigma, p.y_sigma);
    }
    else if (p.y_sigma.size() == p.y.size())
    {
      ui->plot->graph(g)->setErrorType(QCPGraph::etValue);
      ui->plot->graph(g)->setDataValueError(p.x, p.y, p.y_sigma);
    }
    else if (p.x_sigma.size() == p.x.size())
    {
      ui->plot->graph(g)->setErrorType(QCPGraph::etKey);
      ui->plot->graph(g)->setDataKeyError(p.x, p.y, p.x_sigma);
    }
    else
    {
      ui->plot->graph(g)->setErrorType(QCPGraph::etNone);
      ui->plot->graph(g)->setData(p.x, p.y);
    }

    for (int i = 0; i < p.x.size(); ++i) {
      QCPItemTracer *crs = new QCPItemTracer(ui->plot);
      crs->setStyle(QCPItemTracer::tsCircle); //tsCirlce?
      crs->setSize(p.appearance.default_pen.width());
      crs->setGraph(ui->plot->graph(g));
      crs->setGraphKey(p.x[i]);
      crs->setInterpolating(true);
      crs->setPen(QPen(p.appearance.themes["selected"].color(), 0));
      crs->setBrush(p.appearance.default_pen.color());
      crs->setSelectedPen(QPen(p.appearance.themes["selected"].color(), 0));
      crs->setSelectedBrush(p.appearance.themes["selected"].color());
      crs->setSelectable(true);
      crs->setProperty("true_value", p.x[i]);
      if (selection_.count(p.x[i]) > 0)
        crs->setSelected(true);
      ui->plot->addItem(crs);
      crs->updatePosition();
    }

    for (auto &q : p.x) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }
    for (auto &q : p.y) {
      if (q < ymin)
        ymin = q;
      if (q > ymax)
        ymax = q;
    }
  }

  ui->plot->rescaleAxes();

  xmax = ui->plot->xAxis->pixelToCoord(ui->plot->xAxis->coordToPixel(xmax) + 15);
  xmin = ui->plot->xAxis->pixelToCoord(ui->plot->xAxis->coordToPixel(xmin) - 15);

  if (!x_fit.empty()) {
    ui->plot->addGraph();
    int g = ui->plot->graphCount() - 1;
    ui->plot->graph(g)->addData(x_fit, y_fit);
    ui->plot->graph(g)->setPen(style_fit.default_pen);
    ui->plot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);

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

  ui->plot->rescaleAxes();
  
  ymax = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(ymax) - 60);
  ymin = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(ymin) + 15);

  if (!floating_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->plot);
    ui->plot->addItem(floatingText);
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(0.5, 0); // place position at center/top of axis rect
    floatingText->setText(floating_text_);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }

  QCPItemPixmap *overlayButton;

  overlayButton = new QCPItemPixmap(ui->plot);
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
  ui->plot->addItem(overlayButton);


  if ((scale_type_x_ == "Logarithmic") && (xmin < 1))
    xmin = 1;

  if ((scale_type_y_ == "Logarithmic") && (ymin < 0))
    ymin = 0;

  ui->plot->xAxis->setRange(xmin, xmax);
  ui->plot->xAxis2->setRange(xmin, xmax);
  ui->plot->yAxis->setRange(ymin, ymax);
  ui->plot->yAxis2->setRange(ymin, ymax);

  ui->plot->replot();
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
  ui->plot->xAxis->setLabel(x);
  ui->plot->yAxis->setLabel(y);
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
}

void WidgetPlotCalib::addPoints(AppearanceProfile style,
                                const QVector<double>& x, const QVector<double>& y,
                                const QVector<double>& x_sigma, const QVector<double>& y_sigma) {
  if (!x.empty() && (x.size() == y.size())) {
    PointSet ps;
    ps.appearance = style;
    ps.x = x;
    ps.y = y;
    ps.x_sigma = x_sigma;
    ps.y_sigma = y_sigma;
    points_.push_back(ps);
  }
}

void WidgetPlotCalib::set_selected_pts(std::set<double> sel) {
  selection_ = sel;
}

void WidgetPlotCalib::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
  ui->plot->xAxis->setBasePen(QPen(fore, 1));
  ui->plot->yAxis->setBasePen(QPen(fore, 1));
  ui->plot->xAxis->setTickPen(QPen(fore, 1));
  ui->plot->yAxis->setTickPen(QPen(fore, 1));
  ui->plot->xAxis->setSubTickPen(QPen(fore, 1));
  ui->plot->yAxis->setSubTickPen(QPen(fore, 1));
  ui->plot->xAxis->setTickLabelColor(fore);
  ui->plot->yAxis->setTickLabelColor(fore);
  ui->plot->xAxis->setLabelColor(fore);
  ui->plot->yAxis->setLabelColor(fore);
  ui->plot->xAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->plot->yAxis->grid()->setPen(QPen(grid1, 1, Qt::DotLine));
  ui->plot->xAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->plot->yAxis->grid()->setSubGridPen(QPen(grid2, 1, Qt::DotLine));
  ui->plot->xAxis->grid()->setSubGridVisible(true);
  ui->plot->yAxis->grid()->setSubGridVisible(true);
  ui->plot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->plot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->plot->setBackground(QBrush(back));
}

void WidgetPlotCalib::update_selection() {
  selection_.clear();
  for (auto &q : ui->plot->selectedItems())
    if (QCPItemTracer *txt = qobject_cast<QCPItemTracer*>(q))
      selection_.insert(txt->property("true_value").toDouble());

  emit selection_changed();
}

void WidgetPlotCalib::setFloatingText(QString txt) {
  floating_text_ = txt;
}

void WidgetPlotCalib::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (choice == "X linear") {
    scale_type_x_ = "Linear";
    ui->plot->xAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "X logarithmic") {
    scale_type_x_ = "Logarithmic";
    ui->plot->xAxis->setScaleType(QCPAxis::stLogarithmic);
  } else if (choice == "Y linear") {
    scale_type_y_ = "Linear";
    ui->plot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "Y logarithmic") {
    scale_type_y_ = "Logarithmic";
    ui->plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  } else {
    QString filter = action->text() + "(*." + action->text() + ")";
    QString fileName = CustomSaveFileDialog(this, "Export plot",
                                            QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                            filter);
    if (validateFile(this, fileName, true)) {
      QFileInfo file(fileName);
      if (file.suffix() == "png") {
        ui->plot->savePng(fileName,0,0,1,100);
      } else if (file.suffix() == "jpg") {
        ui->plot->saveJpg(fileName,0,0,1,100);
      } else if (file.suffix() == "bmp") {
        ui->plot->saveBmp(fileName);
      } else if (file.suffix() == "pdf") {
        ui->plot->savePdf(fileName, true);
      }
    }

  }

  build_menu();
  ui->plot->replot();
  this->setCursor(Qt::ArrowCursor);
}
