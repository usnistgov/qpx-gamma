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
 *      QpFitter -
 *
 ******************************************************************************/

#include "qp_fitter.h"
#include "widget_detectors.h"
#include "fitter.h"
#include "qt_util.h"
#include "qp_button.h"

QpFitter::QpFitter(QWidget *parent)
  : QPlot::Multi1D(parent)
  , range_(nullptr)
  , fit_data_(nullptr)
{
  range_ = new RangeSelector(this);

  setVisibleOptions(QPlot::ShowOptions::zoom | QPlot::ShowOptions::save |
                    QPlot::ShowOptions::scale | QPlot::ShowOptions::title);

  connect(this, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  menuROI.addAction("History...");
  menuROI.addAction("Adjust edges");
  menuROI.addAction("Region settings...");
  menuROI.addAction("Refit");
  menuROI.addAction("Delete");
  connect(&menuROI, SIGNAL(triggered(QAction*)), this, SLOT(changeROI(QAction*)));

  replotExtras();
  plotExtraButtons();
}

void QpFitter::set_busy(bool b)
{
  bool changed = (busy_ != b);
  busy_ = b;
  if (changed)
    adjustY();
}

void QpFitter::setFit(Qpx::Fitter* fit)
{
  fit_data_ = fit;
  update_spectrum();
  updateData();
}

void QpFitter::clear() {
  if (!fit_data_)
    return;

  clearAll();

  clearSelection();
  replot();
}

void QpFitter::update_spectrum()
{
  if (!fit_data_)
    return;

  if (fit_data_->settings().bits_ <= 0) {
    clear();
    updateData();
    return;
  }

  //  if (title.isEmpty())
  //    title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  Detector=" + QString::fromStdString(fit_data_->detector_.name_);

  if (fit_data_->peaks().empty())
    selected_peaks_.clear();

  updateData();
}

void QpFitter::mouseClicked(double x, double y, QMouseEvent* event)
{
  if ((event->button() == Qt::LeftButton) && fit_data_)
    make_range_selection(fit_data_->settings().cali_nrg_.inverse_transform(x));
  else
//  if (event->button() == Qt::RightButton)
    make_range_selection(-1);

  QPlot::Multi1D::mouseClicked(x, y, event);
}


void QpFitter::make_range_selection(int32_t bin)
{
  if (!fit_data_ || (bin < 0))
    range_->visible = false;
  else
  {
    range_->visible = true;
    range_->set_bounds(
          fit_data_->settings().cali_nrg_.transform(fit_data_->finder().find_left(bin)),
          fit_data_->settings().cali_nrg_.transform(fit_data_->finder().find_right(bin))
          );
  }

  make_range_selection();
}

void QpFitter::clear_range_selection()
{
  range_->visible = false;
  make_range_selection();
}

void QpFitter::make_range_selection()
{
  range_->appearance.default_pen = QPen(Qt::darkGreen);
  range_->setProperty("purpose", QString("add peak"));
  range_->setProperty("region", -1);
  range_->setProperty("peak", -1);
  range_->latch_to.clear();
  range_->latch_to.push_back("Residuals");
  range_->latch_to.push_back("Data");

//  if (range_->visible)
//  {
    selected_peaks_.clear();
    selected_roi_ = -1;
//  }
  plotRange();
  calc_visible();
  replot();

  emit range_selection_changed(range_->left(), range_->right());
  emit peak_selection_changed(selected_peaks_);
}

void QpFitter::set_selected_peaks(std::set<double> selected_peaks)
{
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (!selected_peaks_.empty())
  {
    range_->visible = false;
    plotRange();
    follow_selection();
  }
  calc_visible();
  replot();
}

std::set<double> QpFitter::get_selected_peaks()
{
  return selected_peaks_;
}

void QpFitter::clearSelection()
{
  range_->visible = false;
  selected_peaks_.clear();

  selected_roi_ = -1;
  plotRange();
  calc_visible();
  replot();
}

void QpFitter::plotRange()
{
  range_->unplot_self();

  if (!range_->visible)
    return;

  QPlot::Button *newButton = nullptr;

  QString purpose("");
  if (range_->property("purpose").isValid())
    purpose = range_->property("purpose").toString();

  if (purpose == "add peak") {
    newButton = new QPlot::Button(this,
                                  QPixmap(":/icons/oxy/22/edit_add.png"),
                                  "add peak commit", "Add peak",
                                  Qt::AlignTop | Qt::AlignLeft);
  } else if (purpose == "SUM4") {
    newButton = new QPlot::Button(this,
                                  QPixmap(":/icons/oxy/22/flag_blue.png"),
                                  "SUM4 commit", "Apply",
                                  Qt::AlignTop | Qt::AlignLeft);
  } else if (purpose == "background L") {
    newButton = new QPlot::Button(this,
                                  QPixmap(":/icons/oxy/22/flag_blue.png"),
                                  "background L commit", "Apply",
                                  Qt::AlignTop | Qt::AlignLeft);
  } else if (purpose == "background R") {
    newButton = new QPlot::Button(this,
                                  QPixmap(":/icons/oxy/22/flag_blue.png"),
                                  "background R commit", "Apply",
                                  Qt::AlignTop | Qt::AlignLeft);
  }

  range_->plot_self(newButton);
}

void QpFitter::plotEnergyLabel(double peak_id, double peak_energy, QCPItemTracer *crs) {
  QPen pen = QPen(Qt::darkGray, 1);
  QPen selected_pen = QPen(Qt::black, 2);

  QCPItemLine *line = new QCPItemLine(this);
  line->start->setParentAnchor(crs->position);
  line->start->setCoords(0, -35);
  line->end->setParentAnchor(crs->position);
  line->end->setCoords(0, -5);
  line->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 7, 18));
  line->setPen(pen);
  line->setSelectedPen(selected_pen);
  line->setSelectable(true);
  line->setSelected(selected_peaks_.count(peak_id));
  line->setProperty("label", QVariant::fromValue(1));
  line->setProperty("region", crs->property("region"));
  line->setProperty("peak", crs->property("peak"));

  QCPItemText *markerText = new QCPItemText(this);
  markerText->position->setParentAnchor(crs->position);
  markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
  markerText->position->setCoords(0, -35);
  markerText->setText(QString::number(peak_energy));
  markerText->setTextAlignment(Qt::AlignLeft);
  markerText->setFont(QFont("Monospace", 12));
  markerText->setPen(pen);
  markerText->setColor(pen.color());
  markerText->setSelectedFont(QFont("Monospace", 12));
  markerText->setSelectedColor(selected_pen.color());
  markerText->setSelectedPen(selected_pen);
  markerText->setPadding(QMargins(1, 1, 1, 1));
  markerText->setSelectable(true);
  markerText->setSelected(selected_peaks_.count(peak_id));
  markerText->setProperty("region", crs->property("region"));
  markerText->setProperty("peak", crs->property("peak"));

  //make this optional?
  QPlot::Button *newButton = new QPlot::Button(this,
                                                     QPixmap(":/icons/oxy/22/help_about.png"),
                                                     "peak_info", "Peak details",
                                                     Qt::AlignTop | Qt::AlignLeft
                                                     );
  newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  newButton->topLeft->setParentAnchor(markerText->topRight);
  newButton->topLeft->setCoords(5, 0);
  newButton->setProperty("region", crs->property("region"));
  newButton->setProperty("peak", crs->property("peak"));
}

void QpFitter::follow_selection()
{
  double min_marker = std::numeric_limits<double>::max();
  double max_marker = - std::numeric_limits<double>::max();

  for (auto &q : selected_peaks_)
  {
    double nrg = fit_data_->settings().cali_nrg_.transform(q);
    min_marker = std::min(nrg, min_marker);
    max_marker = std::max(nrg, max_marker);
  }

  if (range_->visible)
  {
    double nrg_l = range_->left();
    double nrg_r = range_->right();
    min_marker = std::min(nrg_l, min_marker);
    max_marker = std::max(nrg_r, max_marker);
  }

  bool xaxis_changed = false;
  double dif_lower = min_marker - xAxis->range().lower;
  double dif_upper = max_marker - xAxis->range().upper;
  if (dif_upper > 0) {
    xAxis->setRangeUpper(max_marker + 20);
    if (dif_lower > (dif_upper + 20))
      xAxis->setRangeLower(xAxis->range().lower + dif_upper + 20);
    xaxis_changed = true;
  }

  if (dif_lower < 0) {
    xAxis->setRangeLower(min_marker - 20);
    if (dif_upper < (dif_lower - 20))
      xAxis->setRangeUpper(xAxis->range().upper + dif_lower - 20);
    xaxis_changed = true;
  }

  if (xaxis_changed)
  {
    replot();
    adjustY();
    replot();
  }

  //  miny = yAxis->pixelToCoord(yAxis->coordToPixel(miny) + 20);
  //  maxy = yAxis->pixelToCoord(yAxis->coordToPixel(maxy) - 100);
}

void QpFitter::plotExtraButtons()
{
  addStackButton(new QPlot::Button(this,
                                   QPixmap(":/icons/oxy/22/editdelete.png"),
                                   "delete peaks", "Delete peak(s)",
                                   Qt::AlignBottom | Qt::AlignRight));
}

void QpFitter::adjustY()
{
  calc_visible();
  QPlot::Multi1D::adjustY();
}

void QpFitter::selection_changed()
{
  if (!hold_selection_)
  {
    selected_peaks_.clear();
    selected_roi_ = -1;

    for (auto &q : selectedItems())
      if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
        if (txt->property("peak").isValid())
          selected_peaks_.insert(txt->property("peak").toDouble());
      } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
        if (line->property("label").isValid() && line->property("peak").isValid())
          selected_peaks_.insert(line->property("peak").toDouble());
      }
  }

  hold_selection_ = false;
  calc_visible();

  if (!selected_peaks_.empty())
  {
    range_->visible = false;
    plotRange();
    emit range_selection_changed(range_->left(), range_->right());
  }
  replot();

  emit peak_selection_changed(selected_peaks_);

  //  void QpFitter:: user_selected_peaks(std::set<double> selected_peaks) {
  //    bool changed = (selected_peaks_ != selected_peaks);
  //    selected_peaks_ = selected_peaks;

  //    if (changed && isVisible())
  //      emit peak_selection_changed(selected_peaks_);
  //  }
}

void QpFitter::executeButton(QPlot::Button *button)
{
  if (button->name() == "SUM4 begin")
    make_SUM4_range(button->property("region").toDouble(), button->property("peak").toDouble());
  else if (button->name() == "background L begin")
    make_background_range(button->property("region").toDouble(), true);
  else if (button->name() == "background R begin")
    make_background_range(button->property("region").toDouble(), false);
  else if (button->name() == "region options")
  {
//    if (!busy_) {
      menuROI.setProperty("region", button->property("region"));
      menuROI.exec(QCursor::pos());
//    }
  }
  else if (button->name() == "add peak commit")
    emit add_peak(range_->left(), range_->right());
  else if (button->name() == "delete peaks")
    emit delete_selected_peaks();
  else if (button->name() == "SUM4 commit")
    emit adjust_sum4(range_->property("peak").toDouble(), range_->left(), range_->right());
  else if (button->name() == "background L commit")
    emit adjust_background_L(range_->property("region").toDouble(), range_->left(), range_->right());
  else if (button->name() == "background R commit")
    emit adjust_background_R(range_->property("region").toDouble(), range_->left(), range_->right());
  else if (button->name() == "peak_info")
    emit peak_info(button->property("peak").toDouble());
  else
    QPlot::Multi1D::executeButton(button);
}

void QpFitter::make_SUM4_range(double region, double peak)
{
  if (!fit_data_  || !fit_data_->contains_region(region))
    return;

  Qpx::ROI parent_region = fit_data_->region(region);

  if (!parent_region.contains(peak))
    return;

  Qpx::Peak pk = parent_region.peak(peak);
  range_->setProperty("purpose", QString("SUM4"));
  range_->latch_to.clear();
  range_->latch_to.push_back("Background sum4");
  range_->appearance.default_pen = QPen(Qt::darkCyan);
  range_->setProperty("region", QVariant::fromValue(region));
  range_->setProperty("peak", QVariant::fromValue(peak));

  range_->set_bounds(fit_data_->settings().cali_nrg_.transform(pk.sum4().left()),
                    fit_data_->settings().cali_nrg_.transform(pk.sum4().right()));
  range_->visible = true;

  //  selected_peaks_.clear();
  plotRange();
  calc_visible();
  replot();

  emit peak_selection_changed(selected_peaks_);
}


void QpFitter::make_background_range(double roi, bool left)
{
  if (!fit_data_ || !fit_data_->contains_region(roi))
    return;

  Qpx::ROI parent_region = fit_data_->region(roi);

  if (left)
  {
    range_->setProperty("purpose", "background L");
    range_->set_bounds(fit_data_->settings().cali_nrg_.transform(parent_region.LB().left()),
                      fit_data_->settings().cali_nrg_.transform(parent_region.LB().right()));
  }
  else
  {
    range_->setProperty("purpose", "background R");
    range_->set_bounds(fit_data_->settings().cali_nrg_.transform(parent_region.RB().left()),
                      fit_data_->settings().cali_nrg_.transform(parent_region.RB().right()));
  }

  range_->setProperty("region",  QVariant::fromValue(roi));
  range_->latch_to.clear();
  range_->latch_to.push_back("Data");
  range_->appearance.default_pen = QPen(Qt::darkMagenta);
  range_->visible = true;

  //  selected_peaks_.clear();
  plotRange();
  calc_visible();
  replot();

  emit peak_selection_changed(selected_peaks_);
}

void QpFitter::changeROI(QAction* action)
{
  QString choice = action->text();
  double region = menuROI.property("region").toDouble();
  if (choice == "History...") {
    emit rollback_ROI(region);
  } else if (choice == "Region settings...") {
    emit roi_settings(region);
  } else if (choice == "Adjust edges") {
    selected_roi_ = region;
    calc_visible();
    replot();
  } else if (choice == "Refit") {
    emit refit_ROI(region);
  } else if (choice == "Delete") {
    emit delete_ROI(region);
  }
}

void QpFitter::updateData()
{
  if (fit_data_ == nullptr)
    return;

  QColor color_data;
  color_data.setHsv(QColor(QString::fromStdString(fit_data_->metadata_.get_attribute("appearance").value_text)).hsvHue(), 48, 160);
  QPen pen_data = QPen(color_data, 1);

  QPen pen_resid = QPen( Qt::darkGreen, 1);
  pen_resid.setStyle(Qt::DashLine);

  clearAll();
  replotExtras();

  xAxis->setLabel(QString::fromStdString(fit_data_->settings().cali_nrg_.units_));
  yAxis->setLabel("count");


  auto xx  = fit_data_->settings().cali_nrg_.transform(fit_data_->finder().x_, fit_data_->settings().bits_);

  QCPGraph *data_graph = addGraph(xx, fit_data_->finder().y_, pen_data, false, "Data");

  addGraph(xx, fit_data_->finder().y_resid_on_background_, pen_resid, false, "Residuals");

  for (auto &r : fit_data_->regions())
    plotRegion(r.first, r.second, data_graph);

  plotRange();
  plotExtraButtons();

  calc_visible();
  adjustY();
  replot();
}

void QpFitter::plotRegion(double region_id, const Qpx::ROI &region, QCPGraph *data_graph)
{
  QPen pen_back_sum4 = QPen(Qt::darkMagenta, 1);
  QPen pen_back_poly = QPen(Qt::darkGray, 1);
  QPen pen_back_with_steps = QPen(Qt::darkBlue, 1);
  QPen pen_full_fit = QPen(Qt::red, 1);

  QPen pen_peak  = QPen(Qt::darkCyan, 1);
  pen_peak.setStyle(Qt::DotLine);

  std::vector<double> xs;

  if (!region.fit_settings().sum4_only)
  {
    xs = region.hr_x_nrg;
    //    trim_log_lower(yy);
    addGraph(xs, region.hr_fullfit, pen_full_fit, true, "Region fit", region_id);
  } else {
    xs = fit_data_->settings().cali_nrg_.transform(region.finder().x_, fit_data_->settings().bits_);
    addGraph(xs, region.finder().y_, pen_full_fit, false, "Region fit", region_id);
  }

  if (region.width()) {
    plotBackgroundEdge(region.LB(), region.finder().x_, region_id, "background L begin");
    plotBackgroundEdge(region.RB(), region.finder().x_, region_id, "background R begin");

    auto yb = region.hr_sum4_background_;
    addGraph(xs, yb, pen_back_sum4, true, "Background sum4", region_id);
  }

  if (region.peak_count() && !region.fit_settings().sum4_only)
  {
    addGraph(xs, region.hr_background, pen_back_poly, true, "Background poly", region_id);
    addGraph(xs, region.hr_back_steps, pen_back_with_steps, true, "Background steps", region_id);
  }

  if (!region.hr_x_nrg.empty()) {
    QPlot::Button *newButton = new QPlot::Button(this,
                                                       QPixmap(":/icons/oxy/22/applications_systemg.png"),
                                                       "region options", "Region options",
                                                       Qt::AlignTop | Qt::AlignLeft
                                                       );
    newButton->topLeft->setType(QCPItemPosition::ptPlotCoords);
    newButton->topLeft->setCoords(region.hr_x_nrg.front(), region.hr_fullfit.front());
    newButton->setProperty("region", QVariant::fromValue(region_id));
  }

  for (auto & p : region.peaks())
  {
    QCPGraph *peak_graph = nullptr;
    if (!region.fit_settings().sum4_only)
    {
      //      auto ys = p.second.hr_fullfit_;
      //      trim_log_lower(yy);
      peak_graph = addGraph(xs, p.second.hr_fullfit_, pen_peak, true,
                            "Individual peak", region_id, p.first);
    }

    QCPItemTracer *crs = new QCPItemTracer(this);
    crs->setStyle(QCPItemTracer::tsNone);
    crs->setProperty("region", region_id);
    crs->setProperty("peak", p.first);
    if (peak_graph)
      crs->setGraph(peak_graph);
    else
      crs->setGraph(data_graph);
    crs->setInterpolating(true);
    double energy = fit_data_->finder().settings_.cali_nrg_.transform(p.first, fit_data_->finder().settings_.bits_);
    crs->setGraphKey(energy);
    crs->updatePosition();

    plotEnergyLabel(p.first, p.second.energy().value(), crs);
    plotPeak(region_id, p.first, p.second);
  }

}

void QpFitter::plotPeak(double region_id, double peak_id, const Qpx::Peak &peak)
{
  QPen pen_sum4_peak     = QPen(Qt::darkYellow, 3);
  QPen pen_sum4_selected = QPen(Qt::darkCyan, 3);

  if (peak.sum4().peak_width() && fit_data_->contains_region(region_id)) {

    double x1 = fit_data_->settings().cali_nrg_.transform(peak.sum4().left() - 0.5,
                                                          fit_data_->settings().bits_);
    double x2 = fit_data_->settings().cali_nrg_.transform(peak.sum4().right() + 0.5,
                                                          fit_data_->settings().bits_);
    double y1 = fit_data_->region(region_id).sum4_background().eval(peak.sum4().left());
    double y2 = fit_data_->region(region_id).sum4_background().eval(peak.sum4().right());

    QCPItemLine *line = new QCPItemLine(this);
    line->setSelectable(false);
    line->setProperty("SUM4 peak", QVariant::fromValue(1));
    line->setProperty("peak", QVariant::fromValue(peak_id));
    line->setProperty("region", QVariant::fromValue(region_id));
    line->start->setType(QCPItemPosition::ptPlotCoords);
    line->start->setCoords(x1,y1);
    line->end->setType(QCPItemPosition::ptPlotCoords);
    line->end->setCoords(x2,y2);
    line->setHead(QCPLineEnding(QCPLineEnding::esBar, 10, 20));
    line->setTail(QCPLineEnding(QCPLineEnding::esBar, 10, 20));
    line->setPen(pen_sum4_peak);
    line->setSelectedPen(pen_sum4_selected);


    QPlot::Button *newButton = new QPlot::Button(this,
                                                       QPixmap(":/icons/oxy/22/system_switch_user.png"),
                                                       "SUM4 begin", "Adjust SUM4 bounds",
                                                       Qt::AlignTop | Qt::AlignLeft
                                                       );
    newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    newButton->bottomRight->setCoords(x1, y1);
    newButton->setProperty("peak", QVariant::fromValue(peak_id));
    newButton->setProperty("region", QVariant::fromValue(region_id));
  }
}

void QpFitter::plotBackgroundEdge(Qpx::SUM4Edge edge,
                                    const std::vector<double> &x,
                                    double region_id,
                                    QString button_name) {

  QPen pen_background_edge = QPen(Qt::darkMagenta, 3);


  double x1 = fit_data_->settings().cali_nrg_.transform(edge.left() - 0.5,
                                                        fit_data_->settings().bits_);
  double x2 = fit_data_->settings().cali_nrg_.transform(edge.right() + 0.5,
                                                        fit_data_->settings().bits_);
  double y = edge.average();

  QCPItemLine *line = new QCPItemLine(this);
  line->setSelectable(false);
  line->setProperty("background edge", QVariant::fromValue(1));
  line->setProperty("region", QVariant::fromValue(region_id));
  line->start->setType(QCPItemPosition::ptPlotCoords);
  line->start->setCoords(x1,y);
  line->end->setType(QCPItemPosition::ptPlotCoords);
  line->end->setCoords(x2,y);
  line->setHead(QCPLineEnding(QCPLineEnding::esBar, 10, 20));
  line->setTail(QCPLineEnding(QCPLineEnding::esBar, 10, 20));
  line->setPen(pen_background_edge);
  line->setSelectedPen(pen_background_edge);

  QPlot::Button *newButton = new QPlot::Button(this,
                                                     QPixmap(":/icons/oxy/22/system_switch_user.png"),
                                                     button_name, "Adjust background",
                                                     Qt::AlignTop | Qt::AlignLeft
                                                     );
  newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
  newButton->bottomRight->setCoords(x1, y);
  newButton->setProperty("region", QVariant::fromValue(region_id));
  newButton->setProperty("peak", QVariant::fromValue(-1));
}


void QpFitter::trim_log_lower(std::vector<double> &array) {
  for (auto &r : array)
    if (r < 1)
      r = std::floor(r * 10 + 0.5)/10;
}

void QpFitter::calc_visible()
{
  std::set<double> good_roi;
  std::set<double> good_peak;
  std::set<double> prime_roi;

  auto xmin = xAxis->range().lower;
  auto xmax = xAxis->range().upper;

  if (fit_data_ == nullptr)
    return;

  for (auto &q : fit_data_->regions()) {
    if (q.second.hr_x_nrg.empty())
      continue;
    bool left_visible = (q.second.hr_x_nrg.front() >= xmin) && (q.second.hr_x_nrg.front() <= xmax);
    bool right_visible = (q.second.hr_x_nrg.back() >= xmin) && (q.second.hr_x_nrg.back() <= xmax);
    bool deep_zoom = (q.second.hr_x_nrg.front() < xmin) && (q.second.hr_x_nrg.back() > xmax);

    if (!left_visible && !right_visible && !deep_zoom)
      continue;

    good_roi.insert(q.first);

    if (((q.second.hr_x_nrg.back() - q.second.hr_x_nrg.front()) / (xmax - xmin)) > 0.3)
      prime_roi.insert(q.first);

    for (auto &p : q.second.peaks()) {
      if ((p.second.energy().value() >= xmin) && (p.second.energy().value() <= xmax))
        good_peak.insert(p.first);
    }
  }

  std::set<double> labeled_peaks;
  if (good_peak.size() < 20)
    labeled_peaks = good_peak;
  else {

  }

  bool plot_back_poly = good_roi.size() && (good_roi.size() < 5);
  bool plot_resid_on_back = plot_back_poly;
  bool plot_peak      = ((good_roi.size() < 5) || (good_peak.size() < 10));
  bool plot_back_steps = ((good_roi.size() < 10) || plot_peak);
  bool plot_fullfit    = (good_roi.size() < 15);

  bool adjusting_background = range_->visible && range_->property("purpose").isValid() &&
      ((range_->property("purpose").toString() == "background R") ||
       (range_->property("purpose").toString() == "background L"))
      && !busy_;

  bool adjusting_sum4 = range_->visible && range_->property("purpose").isValid() &&
      (range_->property("purpose").toString() == "SUM4")
      && !busy_;
  bool selected_one = ((selected_peaks_.size() == 1)  && !busy_);

  bool button_background     = (selected_one || (selected_roi_ >=0)) && !busy_;
  bool line_background       = (button_background || adjusting_sum4) && !adjusting_background;
  bool line_sum4_selpeak     = button_background || adjusting_background;

  this->blockSignals(true);
  for (int i=0; i < itemCount(); ++i) {
    QCPAbstractItem *q =  item(i);
    double peak = -1;
    double region = -1;
    if (q->property("region").isValid())
      region = q->property("region").toDouble();
    if (q->property("peak").isValid())
      peak = q->property("peak").toDouble();
    if ((peak < 0) && (region >= 0) && selected_one
        && fit_data_->region(region).contains(*selected_peaks_.begin())) {
      peak = *selected_peaks_.begin();
      q->setProperty("peak", QVariant::fromValue(peak));
    }

    bool this_peak_selected = (selected_peaks_.count(peak) > 0);
    bool this_region_selected = !selected_peaks_.empty()
        && fit_data_->contains_region(region)
        && fit_data_->region(region).contains(peak)
        && fit_data_->region(region).contains(*selected_peaks_.begin());
    if ((selected_roi_ == region) && (region >= 0))
      this_region_selected = true;
    bool range_this_peak = ((range_->property("peak").toDouble() == peak) && range_->visible);
    bool range_this_region = ((range_->property("region").toDouble() == region) && range_->visible);

    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      if (peak >= 0) {
        txt->setSelected(selected_peaks_.count(peak));
        txt->setVisible(labeled_peaks.count(peak));
      }
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("SUM4 peak").isValid()) {
        line->setVisible((this_peak_selected && line_sum4_selpeak)
                         || (this_region_selected && !range_this_peak)
                         || (range_this_region && !range_this_peak)
                         || (range_this_region && adjusting_background));
        line->setSelected(selected_peaks_.count(peak));
      } else if (line->property("background edge").isValid()) {
        line->setVisible((this_peak_selected || this_region_selected || range_this_region)
                         && line_background);
      } else if ((line->property("tracer").isNull()) && (peak >= 0)) {
        line->setSelected(selected_peaks_.count(peak));
        line->setVisible(good_peak.count(peak));
      }
    }
    else if (QPlot::Button *button = qobject_cast<QPlot::Button*>(q))
    {
      if ((button->name() == "background L begin") || (button->name() == "background R begin")) {
        button->setVisible((this_region_selected || this_peak_selected)
                           && button_background && !range_->visible);
      } else if (button->name() == "region options") {
        button->setVisible(prime_roi.count(button->property("region").toDouble())
                           && selected_peaks_.empty()
                           && !range_->visible
                           && !busy_
                           && (selected_roi_ < 0));
      } else if (button->property("peak").isValid()) {
        button->setVisible(selected_one && this_peak_selected);
      } else if (button->name() == "delete peaks") {
        button->setVisible(!selected_peaks_.empty() && !busy_);
      } else
        button->setVisible(true);

    }
  }
  this->blockSignals(false);

  int total = graphCount();
  for (int i=0; i < total; i++) {
    QCPGraph *graph = QCustomPlot::graph(i);

    double peak = -1;
    double region = -1;
    if (graph->property("region").isValid())
      region = graph->property("region").toDouble();
    if (graph->property("peak").isValid())
      peak = graph->property("peak").toDouble();
    if ((peak < 0) && (region >= 0) && selected_one
        && fit_data_->region(region).contains(*selected_peaks_.begin())) {
      peak = *selected_peaks_.begin();
      graph->setProperty("peak", QVariant::fromValue(peak));
    }

    bool this_peak_selected = (selected_peaks_.count(peak) > 0);
    bool this_region_selected = !selected_peaks_.empty()
        && fit_data_->contains_region(region)
        && fit_data_->region(region).contains(peak)
        && fit_data_->region(region).contains(*selected_peaks_.begin());
    if (selected_roi_ == region)
      this_region_selected = true;
    bool range_this_peak = (range_->property("peak").toDouble() == peak);
    bool range_this_region = (range_->property("region").toDouble() == region);
    bool region_sum4_only = (fit_data_->contains_region(region)
                             && fit_data_->region(region).fit_settings().sum4_only);


    bool good = ((good_roi.count(region) || good_peak.count(peak))
                 && !adjusting_background && !adjusting_sum4);
    if (graph->name() == "Region fit")
      graph->setVisible((plot_fullfit && good) || (range_this_region && adjusting_background));
    else if (graph->name() == "Background steps")
      graph->setVisible(plot_back_steps && good);
    else if (graph->name() == "Background poly")
      graph->setVisible((plot_back_poly && good) || (range_this_region && adjusting_background));
    else if (graph->name() == "Individual peak")
      graph->setVisible((plot_peak && good) || (range_this_region && adjusting_sum4));
    else if (graph->name() == "Residuals")
      graph->setVisible(plot_resid_on_back);
    else if (graph->name() == "Background sum4") {
      graph->setVisible((range_this_region && adjusting_sum4) ||
                        (this_region_selected && region_sum4_only));
    }
  }
}

QCPRange QpFitter::getRange(QCPRange domain)
{
  QCPRange range = Multi1D::getRange(domain);

  bool markers_in_range {false};
  for (auto &peak : fit_data_->peaks())
    if ((domain.lower <= peak.second.energy().value()) &&
        (peak.second.energy().value() <= domain.upper))
      markers_in_range = true;

  int upper = 0
      + (markers_in_range ? 30 : 0)
      + ((markers_in_range && showMarkerLabels()) ? 20 : 0);

  range.upper = yAxis->pixelToCoord(yAxis->coordToPixel(range.upper) - upper);

  return range;
}

QCPGraph* QpFitter::addGraph(const std::vector<double>& x, const std::vector<double>& y, QPen appearance,
                               bool smooth, QString name, double region, double peak)
{
  auto target = QPlot::Multi1D::addGraph(x, y, appearance, name);
  if (!target)
    return target;

  target->setProperty("region", QVariant::fromValue(region));
  target->setProperty("peak", QVariant::fromValue(peak));

  if (!smooth) {
    target->setBrush(QBrush());
    target->setLineStyle(QCPGraph::lsStepCenter);
    target->setScatterStyle(QCPScatterStyle::ssNone);
  } else {
    target->setBrush(QBrush());
    target->setLineStyle(QCPGraph::lsLine);
    target->setScatterStyle(QCPScatterStyle::ssNone);
  }

  return target;
}
