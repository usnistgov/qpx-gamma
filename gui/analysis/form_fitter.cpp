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
 *      FormFitter -
 *
 ******************************************************************************/

#include "form_fitter.h"
#include "widget_detectors.h"
#include "ui_form_fitter.h"
#include "gamma_fitter.h"
#include "qt_util.h"
#include "qcp_overlay_button.h"
#include "form_fitter_settings.h"
#include "form_peak_info.h"

RollbackDialog::RollbackDialog(Gamma::ROI roi, QWidget *parent) :
  QDialog(parent),
  roi_(roi)
{
  QLabel *label;
  QFrame* line;

  QVBoxLayout *vl_fit    = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Fit with # of peaks");
  vl_fit->addWidget(label);

  QVBoxLayout *vl_rsq = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("r-squared");
  vl_rsq->addWidget(label);

  for (int i=0; i < roi_.fits_.size(); ++i) {

    QRadioButton *radio = new QRadioButton();
    radio->setLayoutDirection(Qt::LeftToRight);
    radio->setText(QString::number(roi_.fits_[i].peaks_.size()));
    radio->setFixedHeight(25);
    bool selected = false;
    if (!roi_.fits_[i].peaks_.empty() && !roi_.peaks_.empty())
      selected = (roi_.fits_[i].peaks_.begin()->second.hypermet_.rsq_ == roi_.peaks_.begin()->second.hypermet_.rsq_);
    radio->setChecked(selected);
    radios_.push_back(radio);
    vl_fit->addWidget(radio);

    label = new QLabel();
    label->setFixedHeight(25);
    double rsq = 0;
    if (!roi_.fits_[i].peaks_.empty())
      rsq = roi_.fits_[i].peaks_.begin()->second.hypermet_.rsq_;
    label->setText(QString::number(rsq));
    vl_rsq->addWidget(label);

  }

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(vl_fit);
  hl->addLayout(vl_rsq);

  label = new QLabel();
  label->setText(QString::fromStdString("<b>ROI at chan=" + std::to_string(roi_.hr_x_nrg.front()) + " rollback to</b>"));

  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setFixedHeight(3);
  line->setLineWidth(1);

  QVBoxLayout *total    = new QVBoxLayout();
  total->addWidget(label);
  total->addWidget(line);
  total->addLayout(hl);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  total->addWidget(buttonBox);

  setLayout(total);
}

int RollbackDialog::get_choice() {
  int ret = 0;
  for (int i=0; i < radios_.size(); ++i)
    if (radios_[i]->isChecked())
      ret = i;
  return ret;
}


FormFitter::FormFitter(QWidget *parent) :
  QWidget(parent),
  fit_data_(nullptr),
  ui(new Ui::FormFitter)
{
  ui->setupUi(this);

  ui->plot->setInteraction(QCP::iSelectItems, true);
  ui->plot->setInteraction(QCP::iRangeDrag, true);
  ui->plot->yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
  ui->plot->setInteraction(QCP::iRangeZoom, true);
  ui->plot->setInteraction(QCP::iMultiSelect, true);

  ui->plot->yAxis->setPadding(28);
  ui->plot->setNoAntialiasingOnDrag(true);
  ui->plot->xAxis->grid()->setVisible(true);
  ui->plot->yAxis->grid()->setVisible(true);
  ui->plot->xAxis->grid()->setSubGridVisible(true);
  ui->plot->yAxis->grid()->setSubGridVisible(true);

  connect(ui->plot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->plot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->plot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
  connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));
  connect(ui->plot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->plot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));

  minx_zoom = 0; maxx_zoom = 0;
  minx = std::numeric_limits<double>::max();
  maxx = - std::numeric_limits<double>::max();
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  mouse_pressed_ = false;
  hold_selection_ = false;

  selected_roi_ = -1;

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;

  scale_type_ = "Logarithmic";
  ui->plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  setColorScheme(Qt::black, Qt::white, QColor(112, 112, 112), QColor(170, 170, 170));

  ui->plot->xAxis->grid()->setVisible(true);
  ui->plot->yAxis->grid()->setVisible(true);
  ui->plot->xAxis->grid()->setSubGridVisible(true);
  ui->plot->yAxis->grid()->setSubGridVisible(true);

  menuExportFormat.addAction("png");
  menuExportFormat.addAction("jpg");
  menuExportFormat.addAction("pdf");
  menuExportFormat.addAction("bmp");
  connect(&menuExportFormat, SIGNAL(triggered(QAction*)), this, SLOT(exportRequested(QAction*)));

  connect(&menuOptions, SIGNAL(triggered(QAction*)), this, SLOT(optionsChanged(QAction*)));

  menuROI.addAction("History...");
  menuROI.addAction("Adjust bounds");
  menuROI.addAction("Adjust background");
  menuROI.addAction("Refit");
  menuROI.addAction("Delete");
  connect(&menuROI, SIGNAL(triggered(QAction*)), this, SLOT(changeROI(QAction*)));

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), ui->plot);
  connect(shortcut, SIGNAL(activated()), this, SLOT(zoom_out()));

  QShortcut *shortcut2 = new QShortcut(QKeySequence(Qt::Key_Insert), ui->plot);
  connect(shortcut2, SIGNAL(activated()), this, SLOT(add_peak()));

  QShortcut* shortcut3 = new QShortcut(QKeySequence(QKeySequence::Delete), ui->plot);
  connect(shortcut3, SIGNAL(activated()), this, SLOT(delete_selected_peaks()));

  QShortcut* shortcut4 = new QShortcut(QKeySequence(QKeySequence(Qt::Key_L)), ui->plot);
  connect(shortcut4, SIGNAL(activated()), this, SLOT(switch_scale_type()));


  connect(&thread_fitter_, SIGNAL(fit_updated(Gamma::Fitter)), this, SLOT(fit_updated(Gamma::Fitter)));
  connect(&thread_fitter_, SIGNAL(fitting_done()), this, SLOT(fitting_complete()));

  build_menu();
  plotButtons();

  QMovie *movie = new QMovie(":/loader.gif");
  ui->labelMovie->setMovie(movie);
  ui->labelMovie->show();
  movie->start();
  ui->labelMovie->setVisible(false);


  busy_ = false;
  thread_fitter_.start();
}

FormFitter::~FormFitter()
{
  thread_fitter_.stop_work();
  thread_fitter_.terminate(); //in thread itself?

  delete ui;
}

void FormFitter::setFit(Gamma::Fitter* fit) {
  fit_data_ = fit;
  update_spectrum();
  updateData();
}

void FormFitter::loadSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  settings_.endGroup();
}

void FormFitter::saveSettings(QSettings &settings_) {
  settings_.beginGroup("Peaks");
  settings_.setValue("scale_type", scale_type_);
  settings_.endGroup();
}

void FormFitter::clear() {
  if (!fit_data_ || busy_)
    return;

  //PL_DBG << "FormFitter::clear()";

  minx = std::numeric_limits<double>::max();
  maxx = - std::numeric_limits<double>::max();
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();

  ui->plot->clearGraphs();
  ui->plot->clearItems();
  minima_.clear();
  maxima_.clear();
  reset_scales();
  clearSelection();
  ui->plot->replot();
  toggle_push();
}

void FormFitter::update_spectrum(QString title) {
  if (!fit_data_ || busy_)
    return;

  if (fit_data_->metadata_.resolution == 0) {
    clear();
    updateData();
    return;
  }

  if (title.isEmpty())
    title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  Detector=" + QString::fromStdString(fit_data_->detector_.name_);
  title_text_ = title;

  reset_scales();

  updateData();
  toggle_push();
  minx_zoom = 0; maxx_zoom = 0;
}


void FormFitter::refit_ROI(double ROI_bin) {
  if (busy_ || (fit_data_ == nullptr))
    return;

  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.refit(ROI_bin);
}


void FormFitter::rollback_ROI(double ROI_bin) {
  if (busy_ || (fit_data_ == nullptr))
    return;

  if (fit_data_->regions_.count(ROI_bin)) {
    RollbackDialog *editor = new RollbackDialog(fit_data_->regions_.at(ROI_bin), qobject_cast<QWidget *> (parent()));
    int ret = editor->exec();
    if (ret == QDialog::Accepted) {
      fit_data_->regions_[ROI_bin].rollback(editor->get_choice());
      fit_data_->remap_region(fit_data_->regions_[ROI_bin]);
      toggle_push();
      updateData();
    }
  }

}

void FormFitter::delete_ROI(double ROI_bin) {
  if (busy_ || (fit_data_ == nullptr))
    return;

  fit_data_->delete_ROI(ROI_bin);
  toggle_push();
  updateData();
}

void FormFitter::make_range(Marker marker) {
  if (marker.visible)
    createRange(marker.pos);
  else {
    range_.visible = false;
    selected_peaks_.clear();
    selected_roi_ = -1;
    plotRange();
    calc_visible();
    ui->plot->replot();
    toggle_push();

    emit range_changed(range_);
    emit peak_selection_changed(selected_peaks_);
  }
}

void FormFitter::createRange(Coord c) {
  if (busy_ || (fit_data_ == nullptr))
    return;
  
  range_.visible = true;
  range_.base.default_pen = QPen(Qt::darkGreen);

  double x = c.bin(fit_data_->settings().bits_);

  uint16_t ch = static_cast<uint16_t>(x);
  uint16_t ch_l = fit_data_->finder_.find_left(ch);
  uint16_t ch_r = fit_data_->finder_.find_right(ch);

  for (auto &q : fit_data_->regions_) {
    if (q.second.overlaps(x)) {
      ch_l = q.second.finder_.find_left(ch);
      ch_r = q.second.finder_.find_right(ch);
    }
  }

  range_.center.set_bin(x, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.l.set_bin(ch_l, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.r.set_bin(ch_r, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.purpose = QString("range");

  selected_peaks_.clear();
  selected_roi_ = -1;
  plotRange();
  calc_visible();
  ui->plot->replot();
  toggle_push();

  emit peak_selection_changed(selected_peaks_);
  emit range_changed(range_);
}

void FormFitter::set_selected_peaks(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (!selected_peaks_.empty()) {
    range_.visible = false;
    plotRange();
    follow_selection();
  }
  calc_visible();
  ui->plot->replot();
  toggle_push();
}

std::set<double> FormFitter::get_selected_peaks() {
  return selected_peaks_;
}

void FormFitter::on_pushFindPeaks_clicked()
{
  perform_fit();
}


void FormFitter::perform_fit() {
  if (busy_ || (fit_data_ == nullptr))
    return;

  fit_data_->finder_.settings_ = fit_data_->settings();
  fit_data_->finder_.find_peaks();
  fit_data_->find_regions();
  //  PL_DBG << "number of peaks found " << fit_data_->peaks_.size();

  busy_= true;
  toggle_push();
  updateData();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.fit_peaks();

}

void FormFitter::add_peak()
{
  if (!range_.visible)
    return;

  if (range_.l.energy() >= range_.r.energy())
    return;

  range_.visible = false;
  plotRange();
  ui->plot->replot();
  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.add_peak(
        fit_data_->finder_.find_index(range_.l.bin(fit_data_->settings().bits_)),
        fit_data_->finder_.find_index(range_.r.bin(fit_data_->settings().bits_))
        );
}

void FormFitter::adjust_roi_bounds() {
  if (!range_.visible)
    return;

  if (range_.l.energy() >= range_.r.energy())
    return;

  range_.visible = false;
  plotRange();
  ui->plot->replot();
  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.adjust_roi_bounds(range_.center.bin(fit_data_->settings().bits_),
                                   fit_data_->finder_.find_index(range_.l.bin(fit_data_->settings().bits_)),
                                   fit_data_->finder_.find_index(range_.r.bin(fit_data_->settings().bits_))
                                   );
}

void FormFitter::adjust_sum4_bounds() {
  if (!range_.visible)
    return;

  Gamma::ROI *parent_region = nullptr;

  if (range_.purpose.toString() == "SUM4")
    parent_region = fit_data_->parent_of(range_.center.bin(fit_data_->settings().bits_));
  else if (fit_data_->regions_.count(range_.center.bin(fit_data_->settings().bits_))) {
    parent_region = &fit_data_->regions_[range_.center.bin(fit_data_->settings().bits_)];
    PL_DBG << "adjust ROI back " << range_.center.energy() << " " << parent_region->hr_x.front();
  }

  if (!parent_region)
    return;

  uint32_t L = parent_region->finder_.find_index(range_.l.bin(fit_data_->settings().bits_));
  uint32_t R = parent_region->finder_.find_index(range_.r.bin(fit_data_->settings().bits_));

  if (L > R) {
    uint32_t T = L;
    L = R;
    R = T;
  }

  if (L == R) {
    range_.visible = false;
    plotRange();
    ui->plot->replot();
    toggle_push();
    return;
  }

  Gamma::SUM4Edge edge(parent_region->finder_.y_, L, R);

  if (range_.purpose.toString() == "ROI back L") {
    PL_DBG << "adjusting L back";
    parent_region->set_LB(edge);
    fit_data_->remap_region(*parent_region);
    //should be in fitter thread
  } else if (range_.purpose.toString() == "ROI back R") {
    PL_DBG << "adjusting R back";
    parent_region->set_RB(edge);
    fit_data_->remap_region(*parent_region);
    //should be in fitter thread
  } else if (range_.purpose.toString() == "SUM4") {
    Gamma::Peak pk = parent_region->peaks_.at(range_.center.bin(fit_data_->settings().bits_));
    Gamma::SUM4 new_sum4(parent_region->finder_.y_, L, R, parent_region->LB(), parent_region->RB());
    pk.sum4_ = new_sum4;
    pk.construct(fit_data_->settings().cali_nrg_,
                 fit_data_->metadata_.live_time.total_milliseconds() * 0.001,
                 fit_data_->settings().bits_);
    fit_data_->replace_peak(pk);
    selected_peaks_.clear();
    selected_peaks_.insert(pk.center);
  }


  range_.visible = false;
  hold_selection_ = true;

  updateData();
  calc_visible();

  emit data_changed();
  emit peak_selection_changed(selected_peaks_);
}

void FormFitter::delete_selected_peaks()
{
  if (busy_ || (fit_data_ == nullptr))
    return;

  std::set<double> chosen_peaks = this->get_selected_peaks();
  if (chosen_peaks.empty())
    return;

  clearSelection();

  busy_= true;
  toggle_push();

  thread_fitter_.set_data(*fit_data_);
  thread_fitter_.remove_peaks(chosen_peaks);
}

void FormFitter::replace_peaks(std::vector<Gamma::Peak> pks) {
  if (busy_ || (fit_data_ == nullptr))
    return;

  if (fit_data_ == nullptr)
    return;

  for (auto &p : pks)
    fit_data_->replace_peak(p);

  updateData();
  emit data_changed();
}

void FormFitter::fit_updated(Gamma::Fitter fitter) {
  *fit_data_ = fitter;
  toggle_push();
  updateData();;
  emit data_changed();
}

void FormFitter::fitting_complete() {
  busy_= false;
  calc_visible();
  ui->plot->replot();
  toggle_push();
  emit data_changed();
}

void FormFitter::toggle_push() {
  if (fit_data_ == nullptr)
    return;

  ui->pushStopFitter->setEnabled(busy_);
  ui->pushFindPeaks->setEnabled(!busy_);
  ui->pushSettings->setEnabled(!busy_);
  ui->labelMovie->setVisible(busy_);
}

void FormFitter::on_pushStopFitter_clicked()
{
  ui->pushStopFitter->setEnabled(false);
  thread_fitter_.stop_work();
}


void FormFitter::build_menu() {
  menuOptions.clear();
  menuOptions.addSeparator();
  menuOptions.addAction("Linear");
  menuOptions.addAction("Logarithmic");

  for (auto &q : menuOptions.actions()) {
    q->setCheckable(true);
    q->setChecked(q->text() == scale_type_);
  }
}

void FormFitter::reset_scales()
{
  minx = std::numeric_limits<double>::max();
  maxx = - std::numeric_limits<double>::max();
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::max();
  ui->plot->rescaleAxes();
}

void FormFitter::clearSelection()
{
  range_.visible = false;
  selected_peaks_.clear();
  selected_roi_ = -1;
  plotRange();
  calc_visible();
  ui->plot->replot();
}


void FormFitter::plotTitle() {
  std::list<QCPAbstractItem*> to_remove;
  for (int i=0; i < ui->plot->itemCount(); ++i)
    if (ui->plot->item(i)->property("floating text").isValid())
      to_remove.push_back(ui->plot->item(i));
  for (auto &q : to_remove)
    ui->plot->removeItem(q);

  if (!title_text_.isEmpty()) {
    QCPItemText *floatingText = new QCPItemText(ui->plot);
    ui->plot->addItem(floatingText);
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

void FormFitter::set_range(Range rng) {
  //  PL_DBG << "<FormFitter> set range";
  range_ = rng;
  plotRange();
  if (range_.visible) {
    selected_peaks_.clear();
    follow_selection();
  }
  calc_visible();
  ui->plot->replot();
}

void FormFitter::addGraph(const QVector<double>& x, const QVector<double>& y,
                             QPen appearance, double bin, int fittable, QString name) {
  if (x.empty() || y.empty() || (x.size() != y.size()))
    return;

  ui->plot->addGraph();
  int g = ui->plot->graphCount() - 1;
  ui->plot->graph(g)->addData(x, y);
  ui->plot->graph(g)->setPen(appearance);
  ui->plot->graph(g)->setProperty("fittable", QVariant::fromValue(fittable));
  ui->plot->graph(g)->setName(name);
  ui->plot->graph(g)->setProperty("bin", QVariant::fromValue(bin));

  if (fittable > 0) {
    ui->plot->graph(g)->setBrush(QBrush());
    ui->plot->graph(g)->setLineStyle(QCPGraph::lsStepCenter);
    ui->plot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  } else {
    ui->plot->graph(g)->setBrush(QBrush());
    ui->plot->graph(g)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(g)->setScatterStyle(QCPScatterStyle::ssNone);
  }

  if (x[0] < minx) {
    minx = x[0];
    //PL_DBG << "new minx " << minx;
    ui->plot->xAxis->rescale();
    ui->plot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
//    ui->plot->xAxis->rescale();
  }
}

void FormFitter::plot_rezoom(bool force) {
  if (mouse_pressed_)
    return;

  if (minima_.empty() || maxima_.empty()) {
    ui->plot->yAxis->rescale();
    return;
  }

  double upperc = ui->plot->xAxis->range().upper;
  double lowerc = ui->plot->xAxis->range().lower;

  if (!force && (lowerc == minx_zoom) && (upperc == maxx_zoom))
    return;

  minx_zoom = lowerc;
  maxx_zoom = upperc;
  calc_visible();

  calc_y_bounds(lowerc, upperc);

  //PL_DBG << "Rezoom";

  if (miny <= 0)
    ui->plot->yAxis->rescale();
  else
    ui->plot->yAxis->setRangeLower(miny);
  ui->plot->yAxis->setRangeUpper(maxy);

}

void FormFitter::calc_y_bounds(double lower, double upper) {
  miny = std::numeric_limits<double>::max();
  maxy = - std::numeric_limits<double>::min();

  for (std::map<double, double>::const_iterator it = minima_.lower_bound(lower); it != minima_.upper_bound(upper); ++it)
    if (it->second < miny)
      miny = it->second;

  for (std::map<double, double>::const_iterator it = maxima_.lower_bound(lower); it != maxima_.upper_bound(upper); ++it) {
    if (it->second > maxy)
      maxy = it->second;
  }

  if (range_.visible && (edge_trc1 != nullptr) && (edge_trc2 != nullptr)) {
    if (edge_trc1->position->value() > maxy)
      maxy = edge_trc1->position->value();
    if (edge_trc1->position->value() < miny)
      miny = edge_trc1->position->value();
    if (edge_trc2->position->value() > maxy)
      maxy = edge_trc2->position->value();
    if (edge_trc2->position->value() < miny)
      miny = edge_trc2->position->value();
  }

  double miny2 = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(miny) + 20);
  maxy = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(maxy) - 100);

  if (miny2 > 1.0)
    miny = miny2;
}

void FormFitter::setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2) {
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
  ui->plot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->plot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
  ui->plot->setBackground(QBrush(back));
}

void FormFitter::plotRange() {
  std::list<QCPAbstractItem*> to_remove;
  for (int i=0; i < ui->plot->itemCount(); ++i) {
    if (ui->plot->item(i)->property("tracer").isValid())
      to_remove.push_back(ui->plot->item(i));
    else if (ui->plot->item(i)->property("button_name").isValid() &&
             (ui->plot->item(i)->property("button_name").toString() == "add peak"))
      to_remove.push_back(ui->plot->item(i));
    else if (ui->plot->item(i)->property("button_name").isValid() &&
             (ui->plot->item(i)->property("button_name").toString() == "roi bounds"))
      to_remove.push_back(ui->plot->item(i));
  }
  for (auto &q : to_remove)
    ui->plot->removeItem(q);

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;
  int fit_level = 0;

  if (range_.visible) {

    double pos_l = 0, pos_c = 0, pos_r = 0;
    pos_l = range_.l.energy();
    pos_c = range_.center.energy();
    pos_r = range_.r.energy();

    if (pos_l < pos_r) {

      int top_idx = -1;
      for (int i=0; i < ui->plot->graphCount(); i++) {

        int level = ui->plot->graph(i)->property("fittable").toInt();

        if (level < fit_level)
          continue;

        if ((ui->plot->graph(i)->data()->firstKey() > pos_l)
            || (pos_r > ui->plot->graph(i)->data()->lastKey()))
          continue;

        fit_level = level;
        top_idx = i;
      }

      if (top_idx >= 0) {
        edge_trc1 = new QCPItemTracer(ui->plot);
        edge_trc1->setStyle(QCPItemTracer::tsNone);
        edge_trc1->setGraph(ui->plot->graph(top_idx));
        edge_trc1->setGraphKey(pos_l);
        edge_trc1->setInterpolating(true);
        edge_trc1->setProperty("tracer", QVariant::fromValue(0));
        ui->plot->addItem(edge_trc1);
        edge_trc1->updatePosition();

        edge_trc2 = new QCPItemTracer(ui->plot);
        edge_trc2->setStyle(QCPItemTracer::tsNone);
        edge_trc2->setGraph(ui->plot->graph(top_idx));
        edge_trc2->setGraphKey(pos_r);
        edge_trc2->setInterpolating(true);
        edge_trc2->setProperty("tracer", QVariant::fromValue(0));
        ui->plot->addItem(edge_trc2);
        edge_trc2->updatePosition();

      } else
        fit_level = 0;
    } else {
      //      PL_DBG << "<FormFitter> bad range";
      range_.visible = false;
      //emit range_changed(range_);
    }
  }

  if (fit_level > 0) {
      DraggableTracer *ar1 = new DraggableTracer(ui->plot, edge_trc1, 12);
      ar1->setPen(QPen(range_.base.default_pen.color(), 1));
      ar1->setProperty("tracer", QVariant::fromValue(1));
      ar1->setSelectable(true);
      ar1->set_limits(minx, maxx);
      ui->plot->addItem(ar1);

      DraggableTracer *ar2 = new DraggableTracer(ui->plot, edge_trc2, 12);
      ar2->setPen(QPen(range_.base.default_pen.color(), 1));
      ar2->setProperty("tracer", QVariant::fromValue(1));
      ar2->setSelectable(true);
      ar2->set_limits(minx, maxx);
      ui->plot->addItem(ar2);

      QCPItemLine *line = new QCPItemLine(ui->plot);
      line->setSelectable(false);
      line->setProperty("tracer", QVariant::fromValue(2));
      line->start->setParentAnchor(edge_trc1->position);
      line->start->setCoords(0, 0);
      line->end->setParentAnchor(edge_trc2->position);
      line->end->setCoords(0, 0);
      line->setPen(QPen(range_.base.default_pen.color(), 2, Qt::DashLine));
      ui->plot->addItem(line);

      QCPItemTracer* higher = edge_trc1;
      if (edge_trc2->position->value() > edge_trc1->position->value())
        higher = edge_trc2;

      QCPOverlayButton *newButton;

      if (range_.purpose.toString() == "range") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/edit_add.png"),
                        "add peak", "Add peak",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (range_.purpose.toString() == "SUM4") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_yellow.png"),
                        "SUM4 adjust", "Adjust SUM4 bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (range_.purpose.toString() == "ROI back L") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_blue.png"),
                        "ROI back L adjust", "Adjust SUM4 bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (range_.purpose.toString() == "ROI back R") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_blue.png"),
                        "ROI back R adjust", "Adjust SUM4 bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      } else {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_red.png"),
                        "roi bounds", "Adjust region bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      }
      newButton->bottomRight->setParentAnchor(higher->position);
      newButton->bottomRight->setCoords(11, -20);
      ui->plot->addItem(newButton);
  }
}

void FormFitter::plotEnergyLabels() {
  if (fit_data_ == nullptr)
    return;

  for (auto &q : fit_data_->peaks()) {
    QCPItemTracer *top_crs = nullptr;

    double max = std::numeric_limits<double>::lowest();
    for (int i=0; i < ui->plot->graphCount(); i++) {

      if (ui->plot->graph(i)->property("fittable").toInt() != 1)
        continue;

      if ((ui->plot->graph(i)->data()->firstKey() >= q.second.energy)
          || (q.second.energy >= ui->plot->graph(i)->data()->lastKey()))
        continue;

      QCPItemTracer *crs = new QCPItemTracer(ui->plot);
      crs->setStyle(QCPItemTracer::tsNone); //tsCirlce?
      crs->setProperty("bin", q.second.center);

      crs->setGraph(ui->plot->graph(i));
      crs->setInterpolating(true);
      crs->setGraphKey(q.second.energy);
      ui->plot->addItem(crs);

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

      QCPItemLine *line = new QCPItemLine(ui->plot);
      line->start->setParentAnchor(top_crs->position);
      line->start->setCoords(0, -35);
      line->end->setParentAnchor(top_crs->position);
      line->end->setCoords(0, -5);
      line->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 7, 18));
      line->setPen(pen);
      line->setSelectedPen(selected_pen);
      line->setProperty("bin", top_crs->property("bin"));
      line->setSelectable(true);
      line->setSelected(selected_peaks_.count(q.second.center));
      ui->plot->addItem(line);

      QCPItemText *markerText = new QCPItemText(ui->plot);
      markerText->setProperty("bin", top_crs->property("bin"));

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
      ui->plot->addItem(markerText);

      QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
            QPixmap(":/icons/oxy/22/help_about.png"),
            "peak_info", "Do sum4 stuff",
            Qt::AlignTop | Qt::AlignLeft
            );
      newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
      newButton->topLeft->setParentAnchor(markerText->topRight);
      newButton->topLeft->setCoords(5, 0);
      newButton->setProperty("peak", QVariant::fromValue(q.first));
      ui->plot->addItem(newButton);

    }

  }
}

void FormFitter::plotROI_options() {
  for (auto &r : fit_data_->regions_) {
    if (r.second.hr_x_nrg.empty())
      continue;

    double x = r.second.hr_x_nrg.front();
    double y = r.second.hr_fullfit.front();

    QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/applications_systemg.png"),
          "ROI options", "Do ROI stuff",
          Qt::AlignTop | Qt::AlignLeft
          );

    newButton->topLeft->setType(QCPItemPosition::ptPlotCoords);
    newButton->topLeft->setCoords(x, y);
    newButton->setProperty("ROI", QVariant::fromValue(r.first));
    ui->plot->addItem(newButton);
  }
}

void FormFitter::follow_selection() {
  double min_marker = std::numeric_limits<double>::max();
  double max_marker = - std::numeric_limits<double>::max();

  for (auto &q : selected_peaks_) {
    double nrg = fit_data_->settings().cali_nrg_.transform(q);
    if (nrg > max_marker)
      max_marker = nrg;
    if (nrg < min_marker)
      min_marker = nrg;
  }

  if (range_.visible) {
    if (range_.l.energy() < min_marker)
      min_marker = range_.l.energy();
    if (range_.r.energy() < max_marker)
      max_marker = range_.r.energy();
  }

  bool xaxis_changed = false;
  double dif_lower = min_marker - ui->plot->xAxis->range().lower;
  double dif_upper = max_marker - ui->plot->xAxis->range().upper;
  if (dif_upper > 0) {
    ui->plot->xAxis->setRangeUpper(max_marker + 20);
    if (dif_lower > (dif_upper + 20))
      ui->plot->xAxis->setRangeLower(ui->plot->xAxis->range().lower + dif_upper + 20);
    xaxis_changed = true;
  }

  if (dif_lower < 0) {
    ui->plot->xAxis->setRangeLower(min_marker - 20);
    if (dif_upper < (dif_lower - 20))
      ui->plot->xAxis->setRangeUpper(ui->plot->xAxis->range().upper + dif_lower - 20);
    xaxis_changed = true;
  }

  if (xaxis_changed) {
    ui->plot->replot();
    plot_rezoom();
  }

//  miny = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(miny) + 20);
//  maxy = ui->plot->yAxis->pixelToCoord(ui->plot->yAxis->coordToPixel(maxy) - 100);


}

void FormFitter::plotButtons() {
  QCPOverlayButton *overlayButton;
  QCPOverlayButton *newButton;

  newButton = new QCPOverlayButton(ui->plot,
        QPixmap(":/icons/oxy/22/view_fullscreen.png"),
        "reset_scales", "Reset plot scales",
        Qt::AlignBottom | Qt::AlignRight);
  newButton->setClipToAxisRect(false);
  newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
  newButton->topLeft->setCoords(5, 5);
  ui->plot->addItem(newButton);
  overlayButton = newButton;

  if (!menuOptions.isEmpty()) {
    newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/view_statistics.png"),
          "options", "Style options",
          Qt::AlignBottom | Qt::AlignRight);
    newButton->setClipToAxisRect(false);
    newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
    newButton->topLeft->setCoords(0, 5);
    ui->plot->addItem(newButton);
    overlayButton = newButton;
  }

  newButton = new QCPOverlayButton(ui->plot,
        QPixmap(":/icons/oxy/22/document_save.png"),
        "export", "Export plot",
        Qt::AlignBottom | Qt::AlignRight);
  newButton->setClipToAxisRect(false);
  newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
  newButton->topLeft->setCoords(0, 5);
  ui->plot->addItem(newButton);
  overlayButton = newButton;

  newButton = new QCPOverlayButton(ui->plot,
        QPixmap(":/icons/oxy/22/editdelete.png"),
        "delete peaks", "Delete selected peaks",
        Qt::AlignBottom | Qt::AlignRight);
  newButton->setClipToAxisRect(false);
  newButton->topLeft->setParentAnchor(overlayButton->bottomLeft);
  newButton->topLeft->setCoords(0, 5);
  ui->plot->addItem(newButton);
  overlayButton = newButton;

}


void FormFitter::plot_mouse_clicked(double x, double y, QMouseEvent* event, bool on_item) {
  if (event->button() == Qt::RightButton) {
    range_.visible = false;
    selected_peaks_.clear();
    selected_roi_ = -1;

    plotRange();
    calc_visible();
    ui->plot->replot();
    toggle_push();

    emit range_changed(range_);
    emit peak_selection_changed(selected_peaks_);
  } else if (!on_item) {
    if (fit_data_ == nullptr)
      return;
    Coord c;
    if (fit_data_->settings().cali_nrg_.valid())
      c.set_energy(x, fit_data_->settings().cali_nrg_);
    else
      c.set_bin(x, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
    createRange(c);
  }
}

void FormFitter::createROI_bounds_range(double bin) {
  if (fit_data_ == nullptr)
    return;

  if (!fit_data_->regions_.count(bin))
    return;

  const Gamma::ROI &r = fit_data_->regions_.at(bin);
  if (r.finder_.x_.empty())
    return;

  double left = r.finder_.x_[0];
  double right = r.finder_.x_[r.finder_.x_.size() - 1];

  range_.visible = true;
  range_.base.default_pen = QPen(Qt::red);
  range_.center.set_bin(bin, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.l.set_bin(left, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.r.set_bin(right, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.purpose = QVariant::fromValue(bin);

  selected_peaks_.clear();
  selected_roi_ = -1;
  plotRange();
  calc_visible();
  ui->plot->replot();
  toggle_push();

  emit peak_selection_changed(selected_peaks_);
}


void FormFitter::selection_changed() {
  if (!hold_selection_) {
    selected_peaks_.clear();
    selected_roi_ = -1;

    for (auto &q : ui->plot->selectedItems())
      if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
        if (txt->property("bin").isValid())
          selected_peaks_.insert(txt->property("bin").toDouble());
      } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
        if (line->property("bin").isValid())
          selected_peaks_.insert(line->property("bin").toDouble());
      }
  }

  hold_selection_ = false;
  calc_visible();

  if (!selected_peaks_.empty()) {
    range_.visible = false;
    plotRange();
    emit range_changed(range_);
  }
  ui->plot->replot();
  toggle_push();

  emit peak_selection_changed(selected_peaks_);


//  void FormFitter:: user_selected_peaks(std::set<double> selected_peaks) {
//    bool changed = (selected_peaks_ != selected_peaks);
//    selected_peaks_ = selected_peaks;

//    if (changed && isVisible())
//      emit peak_selection_changed(selected_peaks_);
//    toggle_push();
//  }
}

void FormFitter::clicked_plottable(QCPAbstractPlottable *plt) {
  //  PL_INFO << "<FormFitter> clickedplottable";
}

void FormFitter::clicked_item(QCPAbstractItem* itm) {
  if (!itm->visible())
    return;

  if (QCPOverlayButton *button = qobject_cast<QCPOverlayButton*>(itm)) {
      if (button->name() == "export")
        menuExportFormat.exec(QCursor::pos());
      else if (button->name() == "reset_scales")
        zoom_out();
      else if (button->name() == "options")
        menuOptions.exec(QCursor::pos());
      else if (button->name() == "SUM4")
        makeSUM4_range(button->property("ROI").toDouble(), button->property("peak").toDouble(), 0);
      else if (button->name() == "ROI back L")
        makeSUM4_range(button->property("ROI").toDouble(), 0, -1);
      else if (button->name() == "ROI back R")
        makeSUM4_range(button->property("ROI").toDouble(), 0, 1);
      else if (button->name() == "ROI options")
      {
        if (!busy_) {
          menuROI.setProperty("ROI", button->property("ROI"));
          menuROI.exec(QCursor::pos());
        }
      }
      else if (button->name() == "roi bounds")
        adjust_roi_bounds();
      else if (button->name() == "add peak")
        add_peak();
      else if (button->name() == "delete peaks")
        delete_selected_peaks();
      else if (button->name() == "SUM4 adjust")
        adjust_sum4_bounds();
      else if (button->name() == "ROI back L adjust")
        adjust_sum4_bounds();
      else if (button->name() == "ROI back R adjust")
        adjust_sum4_bounds();
      else if (button->name() == "peak_info")
        peak_info(button->property("peak").toDouble());
  }
}

void FormFitter::makeSUM4_range(double roi, double peak_chan, int edge)
{
  PL_DBG << "Make sum4 range";

  if (fit_data_ == nullptr)
    return;

  if (!fit_data_->regions_.count(roi))
    return;

  const Gamma::ROI &parent_region = fit_data_->regions_.at(roi);

  int32_t L = 0;
  int32_t R = 0;
  QString purpose;

  if (edge == -1) {
    L = parent_region.LB().start();
    R = parent_region.LB().end();
    purpose = "ROI back L";
    range_.base.default_pen = QPen(Qt::darkMagenta);
    range_.center.set_bin(roi, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  } else if (edge == 1) {
    L = parent_region.RB().start();
    R = parent_region.RB().end();
    purpose = "ROI back R";
    range_.base.default_pen = QPen(Qt::darkMagenta);
    range_.center.set_bin(roi, fit_data_->settings().bits_,fit_data_->settings().cali_nrg_);
  } else if (edge == 0) {
    Gamma::Peak pk = fit_data_->peaks().at(peak_chan);
    L = pk.sum4_.Lpeak;
    R = pk.sum4_.Rpeak;
    purpose = "SUM4";
    range_.base.default_pen = QPen(Qt::darkYellow);
    range_.center.set_bin(pk.center, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  }

  double left = fit_data_->settings().cali_nrg_.transform(parent_region.finder_.x_[L], fit_data_->settings().bits_);
  double right = fit_data_->settings().cali_nrg_.transform(parent_region.finder_.x_[R], fit_data_->settings().bits_);

  range_.visible = true;

  range_.l.set_energy(left, fit_data_->settings().cali_nrg_);
  range_.r.set_energy(right, fit_data_->settings().cali_nrg_);
  range_.purpose = purpose;

//  selected_peaks_.clear();
  plotRange();
  calc_visible();
  ui->plot->replot();
  toggle_push();

  emit peak_selection_changed(selected_peaks_);

}

void FormFitter::zoom_out() {
  ui->plot->rescaleAxes();
  plot_rezoom(true);
  ui->plot->replot();

}

void FormFitter::plot_mouse_press(QMouseEvent*) {
  disconnect(ui->plot, 0, this, 0);
  connect(ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(plot_mouse_release(QMouseEvent*)));
  connect(ui->plot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(clicked_plottable(QCPAbstractPlottable*)));
  connect(ui->plot, SIGNAL(clickedAbstractItem(QCPAbstractItem*)), this, SLOT(clicked_item(QCPAbstractItem*)));
//  connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  mouse_pressed_ = true;
}

void FormFitter::plot_mouse_release(QMouseEvent*) {
  connect(ui->plot, SIGNAL(mouse_clicked(double,double,QMouseEvent*,bool)), this, SLOT(plot_mouse_clicked(double,double,QMouseEvent*,bool)));
  connect(ui->plot, SIGNAL(beforeReplot()), this, SLOT(plot_rezoom()));
  connect(ui->plot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(plot_mouse_press(QMouseEvent*)));
  connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(selection_changed()));

  mouse_pressed_ = false;

  plot_rezoom(true);

  if ((edge_trc1 != nullptr) || (edge_trc2 != nullptr)) {
    double l = edge_trc1->graphKey();
    double r = edge_trc2->graphKey();

    if (r < l) {
      double t = l;
      l = r;
      r = t;
    }

    if (fit_data_->settings().cali_nrg_.valid()) {
      range_.l.set_energy(l, fit_data_->settings().cali_nrg_);
      range_.r.set_energy(r, fit_data_->settings().cali_nrg_);
    } else {
      range_.l.set_bin(l, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
      range_.r.set_bin(r, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
    }

    plotRange();
    toggle_push();

    emit range_changed(range_);
    emit peak_selection_changed(selected_peaks_);
  }
  ui->plot->replot();
}

void FormFitter::optionsChanged(QAction* action) {
  this->setCursor(Qt::WaitCursor);
  QString choice = action->text();
  if (choice == "Linear") {
    scale_type_ = choice;
    ui->plot->yAxis->setScaleType(QCPAxis::stLinear);
  } else if (choice == "Logarithmic") {
    ui->plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    scale_type_ = choice;
  }

  build_menu();
  ui->plot->replot();
  this->setCursor(Qt::ArrowCursor);
}

void FormFitter::changeROI(QAction* action) {
  QString choice = action->text();
  double ROI_bin = menuROI.property("ROI").toDouble();
  if (choice == "History...") {
    rollback_ROI(ROI_bin);
  } else if (choice == "Adjust bounds") {
    createROI_bounds_range(ROI_bin);
  } else if (choice == "Adjust background") {
    selected_roi_ = ROI_bin;
    calc_visible();
    ui->plot->replot();
  } else if (choice == "Refit") {
    refit_ROI(ROI_bin);
  } else if (choice == "Delete") {
    delete_ROI(ROI_bin);
  }
}

QString FormFitter::scale_type() {
  return scale_type_;
}

void FormFitter::switch_scale_type() {
  if (scale_type_ == "Linear")
    set_scale_type("Logarithmic");
  else
    set_scale_type("Linear");
}

void FormFitter::set_scale_type(QString sct) {
  this->setCursor(Qt::WaitCursor);
  scale_type_ = sct;
  if (scale_type_ == "Linear")
    ui->plot->yAxis->setScaleType(QCPAxis::stLinear);
  else if (scale_type() == "Logarithmic")
    ui->plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->plot->replot();
  build_menu();
  this->setCursor(Qt::ArrowCursor);
}

void FormFitter::exportRequested(QAction* choice) {
  QString filter = choice->text() + "(*." + choice->text() + ")";
  QString fileName = CustomSaveFileDialog(this, "Export plot",
                                          QStandardPaths::locate(QStandardPaths::HomeLocation, ""),
                                          filter);
  if (validateFile(this, fileName, true)) {
    QFileInfo file(fileName);
    if (file.suffix() == "png") {
      //      PL_INFO << "Exporting plot to png " << fileName.toStdString();
      ui->plot->savePng(fileName,0,0,1,100);
    } else if (file.suffix() == "jpg") {
      //      PL_INFO << "Exporting plot to jpg " << fileName.toStdString();
      ui->plot->saveJpg(fileName,0,0,1,100);
    } else if (file.suffix() == "bmp") {
      //      PL_INFO << "Exporting plot to bmp " << fileName.toStdString();
      ui->plot->saveBmp(fileName);
    } else if (file.suffix() == "pdf") {
      //      PL_INFO << "Exporting plot to pdf " << fileName.toStdString();
      ui->plot->savePdf(fileName, true);
    }
  }
}


void FormFitter::add_bounds(const QVector<double>& x, const QVector<double>& y) {
  if (x.size() != y.size())
    return;

  for (int i=0; i < x.size(); ++i) {
    if (!minima_.count(x[i]) || (minima_[x[i]] > y[i]))
      minima_[x[i]] = y[i];
    if (!maxima_.count(x[i]) || (maxima_[x[i]] < y[i]))
      maxima_[x[i]] = y[i];
  }
}

void FormFitter::updateData() {
  if (fit_data_ == nullptr)
    return;

  QColor plotcolor;
  plotcolor.setHsv(QColor::fromRgba(fit_data_->metadata_.appearance).hsvHue(), 48, 160);
  QPen main_graph = QPen(plotcolor, 1);

  QPen back_poly = QPen(Qt::darkGray, 1);
  QPen back_with_steps = QPen(Qt::darkBlue, 1);
  QPen full_fit = QPen(Qt::red, 1);

  QPen peak  = QPen(Qt::darkCyan, 1);
  peak.setStyle(Qt::DotLine);

  QPen resid = QPen( Qt::darkGreen, 1);
  resid.setStyle(Qt::DashLine);

//  QColor flagged_color;
//  flagged_color.setHsv(QColor(Qt::green).hsvHue(), QColor(Qt::green).hsvSaturation(), 192);
//  QPen flagged =  QPen(flagged_color, 1);
//  flagged.setStyle(Qt::DotLine);

  QPen sum4edge = QPen(Qt::darkMagenta, 3);
  QPen sum4back = QPen(Qt::darkYellow, 3);

  ui->plot->clearGraphs();

  minima_.clear(); maxima_.clear();

  ui->plot->xAxis->setLabel(QString::fromStdString(fit_data_->settings().cali_nrg_.units_));
  ui->plot->yAxis->setLabel("count");

  QVector<double> xx, yy;

  xx = QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(fit_data_->finder_.x_, fit_data_->settings().bits_));
  yy = QVector<double>::fromStdVector(fit_data_->finder_.y_);

  addGraph(xx, yy, main_graph, -1, 1, "Data");
  add_bounds(xx, yy);

  for (auto &q : fit_data_->regions_) {
    xx = QVector<double>::fromStdVector(q.second.hr_x_nrg);

    yy = QVector<double>::fromStdVector(q.second.hr_fullfit);
    trim_log_lower(yy);
    addGraph(xx, yy, full_fit, q.first, false, "Region fit");
    add_bounds(xx, yy);

    if (!xx.empty()) {
      addEdge(q.second.LB(), q.second.finder_.x_, sum4edge, q.first);
      addEdge(q.second.RB(), q.second.finder_.x_, sum4edge, q.first);
    }

    for (auto & p : q.second.peaks_) {
      yy = QVector<double>::fromStdVector(p.second.hr_fullfit_);
      trim_log_lower(yy);
//      QPen pen = p.second.flagged ? flagged : peak;
      addGraph(xx, yy, peak, p.first, false, "Individual peak");

      if (p.second.sum4_.peak_width > 0) {

        if (!p.second.sum4_.bx.empty()) {
          std::vector<double> x_sum4;
          for (auto &i : p.second.sum4_.bx)
            x_sum4.push_back(q.second.finder_.x_[i]);
          addGraph(QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(x_sum4, fit_data_->settings().bits_)),
                               QVector<double>::fromStdVector(p.second.sum4_.by),
                               sum4back, p.first, -1, "SUM4 background");
        }

      }
    }

    if (!q.second.peaks_.empty()) {

      yy = QVector<double>::fromStdVector(q.second.hr_background);
      trim_log_lower(yy);
      addGraph(xx, yy, back_poly, q.first, false, "Background poly");
      add_bounds(xx, yy);

      yy = QVector<double>::fromStdVector(q.second.hr_back_steps);
      trim_log_lower(yy);
      addGraph(xx, yy, back_with_steps, q.first, false, "Background steps");

      xx = QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(q.second.finder_.x_, fit_data_->settings().bits_));
      yy = QVector<double>::fromStdVector(q.second.finder_.y_resid_on_background_);
//      trim_log_lower(yy); //maybe not?
      addGraph(xx, yy, resid, q.first, 2, "Residuals");
//      add_bounds(xx, yy);
    }

  }

  ui->plot->clearItems();

  plotEnergyLabels();
  plotRange();
  plotButtons();
  plotROI_options();
  plotSUM4_options();

  calc_visible();
  plot_rezoom(true);
  ui->plot->replot();
}

void FormFitter::plotSUM4_options() {
  QCPOverlayButton *newButton;
  for (auto &r : fit_data_->regions_) {

    double x = fit_data_->settings().cali_nrg_.transform(r.second.finder_.x_[r.second.LB().start()], fit_data_->settings().bits_);
    double y = r.second.LB().average();

    newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/system_switch_user.png"),
          "ROI back L", "Do sum4 LB stuff",
          Qt::AlignTop | Qt::AlignLeft
          );
    newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    newButton->bottomRight->setCoords(x, y);
    newButton->setProperty("ROI", QVariant::fromValue(r.first));
    ui->plot->addItem(newButton);

    x = fit_data_->settings().cali_nrg_.transform(r.second.finder_.x_[r.second.RB().start()], fit_data_->settings().bits_);
    y = r.second.RB().average();
    newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/system_switch_user.png"),
          "ROI back R", "Do sum4 RB stuff",
          Qt::AlignTop | Qt::AlignLeft
          );
    newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    newButton->bottomRight->setCoords(x, y);
    newButton->setProperty("ROI", QVariant::fromValue(r.first));
    ui->plot->addItem(newButton);

    for (auto &p : r.second.peaks_) {

      x = fit_data_->settings().cali_nrg_.transform(r.second.finder_.x_[p.second.sum4_.bx.front()], fit_data_->settings().bits_);
      y = p.second.sum4_.by.front();

      newButton = new QCPOverlayButton(ui->plot,
            QPixmap(":/icons/oxy/22/system_switch_user.png"),
            "SUM4", "Do sum4 stuff",
            Qt::AlignTop | Qt::AlignLeft
            );
      newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
      newButton->bottomRight->setCoords(x, y);
      newButton->setProperty("peak", QVariant::fromValue(p.first));
      newButton->setProperty("ROI", QVariant::fromValue(r.first));
      ui->plot->addItem(newButton);

    }
  }
}

void FormFitter::addEdge(Gamma::SUM4Edge edge, std::vector<double> &x, QPen pen, double roi) {
  std::vector<double> x_edge, y_edge;
  for (int i = edge.start(); i <= edge.end(); ++i) {
    x_edge.push_back(x[i]);
    y_edge.push_back(edge.average());
  }

  addGraph(QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(x_edge, fit_data_->settings().bits_)),
                       QVector<double>::fromStdVector(y_edge),
                       pen, roi, -1, "SUM4 edge");
}


void FormFitter::trim_log_lower(QVector<double> &array) {
  for (auto &r : array)
    if (r < 1)
      r = std::floor(r * 10 + 0.5)/10;
}

void FormFitter::calc_visible() {
  std::set<double> good_roi;
  std::set<double> good_peak;
  std::set<double> prime_roi;

  if (fit_data_ == nullptr)
    return;

  bool sum4_visible = (selected_peaks_.size() == 1);

  for (auto &q : fit_data_->regions_) {
    if (q.second.hr_x_nrg.empty())
      continue;
    bool left_visible = (q.second.hr_x_nrg.front() >= minx_zoom) && (q.second.hr_x_nrg.front() <= maxx_zoom);
    bool right_visible = (q.second.hr_x_nrg.back() >= minx_zoom) && (q.second.hr_x_nrg.back() <= maxx_zoom);
    bool deep_zoom = (q.second.hr_x_nrg.front() < minx_zoom) && (q.second.hr_x_nrg.back() > maxx_zoom);

    if (!left_visible && !right_visible && !deep_zoom)
      continue;

    good_roi.insert(q.first);

    if (((q.second.hr_x_nrg.back() - q.second.hr_x_nrg.front()) / (maxx_zoom - minx_zoom)) > 0.5)
      prime_roi.insert(q.first);

    for (auto &p : q.second.peaks_) {
      if ((p.second.energy >= minx_zoom) && (p.second.energy <= maxx_zoom))
        good_peak.insert(p.first);
    }
  }

  std::set<double> labeled_peaks;
  if (good_peak.size() < 20)
    labeled_peaks = good_peak;
  else {

  }

  bool plot_back_poly = (good_roi.size() < 4);
  bool plot_resid_on_back = plot_back_poly;
  bool plot_peak      = ((good_roi.size() < 4) || (good_peak.size() < 10));
  bool plot_back_steps = ((good_roi.size() < 10) || plot_peak);
  bool plot_fullfit    = (good_roi.size() < 15);

  this->blockSignals(true);
  for (int i=0; i < ui->plot->itemCount(); ++i) {
    QCPAbstractItem *q =  ui->plot->item(i);
    if (QCPItemText *txt = qobject_cast<QCPItemText*>(q)) {
      if (txt->property("bin").isValid()) {
        txt->setSelected(selected_peaks_.count(txt->property("bin").toDouble()));
        txt->setVisible(labeled_peaks.count(txt->property("bin").toDouble()));
      }
    } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
      if (line->property("bin").isValid()) {
        line->setSelected(selected_peaks_.count(line->property("bin").toDouble()));
        line->setVisible(good_peak.count(line->property("bin").toDouble()));
      }
    } else if (QCPOverlayButton *button = qobject_cast<QCPOverlayButton*>(q)) {
      if ((button->name() == "ROI back L") || (button->name() == "ROI back R")) {
        button->setVisible(!busy_
                           && !range_.visible
                           && button->property("ROI").isValid()
                           && (button->property("ROI").toDouble() == selected_roi_));
      } else if (button->name() == "ROI options") {
        button->setVisible(prime_roi.count(button->property("ROI").toDouble())
                           && selected_peaks_.empty()
                           && !range_.visible
                           && !busy_
                           && (selected_roi_ < 0));
      } else if (button->property("peak").isValid()) {
        button->setVisible(sum4_visible
                           && selected_peaks_.count(button->property("peak").toDouble())
                           && !busy_);
      } else if (button->name() == "delete peaks") {
        button->setVisible(!selected_peaks_.empty() && !busy_);
      } else
        button->setVisible(true);

    }
  }
  this->blockSignals(false);

  int total = ui->plot->graphCount();
  for (int i=0; i < total; i++) {
    QCPGraph *graph = ui->plot->graph(i);
    bool good = (good_roi.count(graph->property("bin").toDouble())
                 || good_peak.count(graph->property("bin").toDouble()));
    if (graph->name() == "Region fit")
      graph->setVisible(plot_fullfit && good);
    else if (graph->name() == "Background steps")
      graph->setVisible(plot_back_steps && good);
    else if (graph->name() == "Background poly")
      graph->setVisible(plot_back_poly && good);
    else if (graph->name() == "Individual peak")
      graph->setVisible(plot_peak && good);
    else if (graph->name() == "Residuals")
      graph->setVisible(plot_resid_on_back && good);
    else if (graph->name() == "SUM4 edge")
      graph->setVisible(selected_roi_ == graph->property("bin").toDouble());
    else if (graph->name() == "SUM4 background")
      graph->setVisible(sum4_visible && selected_peaks_.count(graph->property("bin").toDouble()));
  }
}


void FormFitter::on_pushSettings_clicked()
{
  if (!fit_data_)
    return;

  FitSettings fs = fit_data_->settings();
  FormFitterSettings *FitterSettings = new FormFitterSettings(fs, this);
  FitterSettings->setWindowTitle("Fitter settings");
  int ret = FitterSettings->exec();

  if (ret == QDialog::Accepted) {
    fit_data_->apply_settings(fs);
    updateData();
  }
}

void FormFitter::peak_info(double bin)
{
  if (!fit_data_)
    return;

  Gamma::ROI *parent_region = fit_data_->parent_of(bin);
  if (!parent_region || !fit_data_->peaks().count(bin))
    return;


  PL_DBG << "Peak info for " << fit_data_->peaks().at(bin).energy;

  Hypermet hm = fit_data_->peaks().at(bin).hypermet_;
  FormPeakInfo *peakInfo = new FormPeakInfo(hm, this);
  peakInfo->setWindowTitle("Parameters for peak at " + QString::number(fit_data_->peaks().at(bin).energy));
  int ret = peakInfo->exec();

  if (ret == QDialog::Accepted) {
    Gamma::Peak pk = fit_data_->peaks().at(bin);
//    if (pk.hypermet_ == hm)
//      return;

    pk.hypermet_ = hm;

    //what if centroid has changed?
    fit_data_->replace_peak(pk);

    selected_peaks_.clear();
    selected_peaks_.insert(pk.center);

    hold_selection_ = true;

    updateData();
    calc_visible();

    emit data_changed();
    emit peak_selection_changed(selected_peaks_);
  }
}