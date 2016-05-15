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
#include "fitter.h"
#include "qt_util.h"
#include "qcp_overlay_button.h"
#include "form_fitter_settings.h"
#include "form_peak_info.h"

RollbackDialog::RollbackDialog(Qpx::ROI roi, QWidget *parent) :
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
      selected = (roi_.fits_[i].peaks_.begin()->second.hypermet().rsq_ == roi_.peaks_.begin()->second.hypermet().rsq_);
    radio->setChecked(selected);
    radios_.push_back(radio);
    vl_fit->addWidget(radio);

    label = new QLabel();
    label->setFixedHeight(25);
    double rsq = 0;
    if (!roi_.fits_[i].peaks_.empty())
      rsq = roi_.fits_[i].peaks_.begin()->second.hypermet().rsq_;
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

  player = new QMediaPlayer(this);

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
  menuROI.addAction("Region settings...");
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


  connect(&thread_fitter_, SIGNAL(fit_updated(Qpx::Fitter)), this, SLOT(fit_updated(Qpx::Fitter)));
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

void FormFitter::setFit(Qpx::Fitter* fit) {
  fit_data_ = fit;
  update_spectrum(title_text_);
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

  //DBG << "FormFitter::clear()";

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

  if (fit_data_->metadata_.bits <= 0) {
    clear();
    updateData();
    return;
  }

//  if (title.isEmpty())
//    title = "Spectrum=" + QString::fromStdString(fit_data_->metadata_.name) + "  Detector=" + QString::fromStdString(fit_data_->detector_.name_);
  title_text_ = title;

  if (fit_data_->peaks().empty())
    selected_peaks_.clear();

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

void FormFitter::make_range(Coord marker) {
  if (!marker.null())
    createRange(marker);
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
  range_.appearance.default_pen = QPen(Qt::darkGreen);

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

  range_.setProperty("purpose", QString("add peak"));
  range_.setProperty("region", -1);
  range_.setProperty("peak", -1);
  range_.c.set_bin(ch, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.l.set_bin(ch_l, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.r.set_bin(ch_r, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.latch_to.clear();
  range_.latch_to.push_back("Residuals");
  range_.latch_to.push_back("Data");

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
  fit_data_->find_regions();
  //  DBG << "number of peaks found " << fit_data_->peaks_.size();

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
  thread_fitter_.add_peak(range_.l.bin(fit_data_->settings().bits_),
                          range_.r.bin(fit_data_->settings().bits_));
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
  thread_fitter_.adjust_roi_bounds(range_.property("region").toDouble(),
                                   fit_data_->finder_.find_index(range_.l.bin(fit_data_->settings().bits_)),
                                   fit_data_->finder_.find_index(range_.r.bin(fit_data_->settings().bits_))
                                   );
}

void FormFitter::adjust_sum4() {
  if (!range_.visible)
    return;

  double ROI_id = range_.property("region").toDouble();
  double peak_id = range_.property("peak").toDouble();

  if (!fit_data_->regions_.count(ROI_id))
    return;

  Qpx::ROI &parent_region = fit_data_->regions_[ROI_id];

  if (!parent_region.peaks_.count(peak_id))
    return;

  uint32_t L = parent_region.finder_.find_index(range_.l.bin(fit_data_->settings().bits_));
  uint32_t R = parent_region.finder_.find_index(range_.r.bin(fit_data_->settings().bits_));

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


  Qpx::Peak pk = parent_region.peaks_.at(peak_id);
  Qpx::SUM4 new_sum4(parent_region.finder_.x_, parent_region.finder_.y_, L, R, parent_region.sum4_background_, parent_region.LB(), parent_region.RB());
  pk = Qpx::Peak(pk.hypermet(), new_sum4, parent_region.settings_);
//  pk.sum4_ = new_sum4;
//  pk.construct(fit_data_->settings());
  fit_data_->replace_peak(pk);
  selected_peaks_.clear();
  selected_peaks_.insert(pk.center().value());

  range_.visible = false;
  hold_selection_ = true;

  updateData();
  calc_visible();

  emit data_changed();
  emit peak_selection_changed(selected_peaks_);
}

void FormFitter::adjust_background() {
  if (!range_.visible)
    return;

  double ROI_id = range_.property("region").toDouble();

  if (!fit_data_->regions_.count(ROI_id))
    return;

  Qpx::ROI &parent_region = fit_data_->regions_[ROI_id];

  if (parent_region.finder_.x_.empty())
    return;

  double l = range_.l.bin(fit_data_->settings().bits_);
  double r = range_.r.bin(fit_data_->settings().bits_);

  uint32_t L = parent_region.finder_.find_index(l);
  uint32_t R = parent_region.finder_.find_index(r);

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

  range_.visible = false;
  plotRange();
  ui->plot->replot();
  busy_= true;
  toggle_push();


  double peak_id = range_.property("peak").toDouble();
  if (parent_region.peaks_.count(peak_id)) {
    selected_peaks_.clear();
    selected_peaks_.insert(peak_id);
    hold_selection_ = true;
  }

  QString purpose = range_.property("purpose").toString();
  if (purpose == "background L")
    thread_fitter_.adjust_LB(ROI_id, L, R);
  else if (purpose == "background R")
    thread_fitter_.adjust_RB(ROI_id, L, R);
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

void FormFitter::replace_peaks(std::vector<Qpx::Peak> pks) {
  if (busy_ || (fit_data_ == nullptr))
    return;

  if (fit_data_ == nullptr)
    return;

  for (auto &p : pks)
    fit_data_->replace_peak(p);

  updateData();
  emit data_changed();
}

void FormFitter::fit_updated(Qpx::Fitter fitter) {
//  while (player->state() == QMediaPlayer::PlayingState)
//    player->stop();

//  if (player->state() != QMediaPlayer::PlayingState) {
//    player->setMedia(QUrl("qrc:/sounds/laser6.wav"));
//    player->setVolume(100);
//    player->setPosition(0);
//    player->play();
////    while (player->state() == QMediaPlayer::PlayingState) {}
//  }

  *fit_data_ = fitter;
  toggle_push();
  updateData();;
  emit data_changed();
}

void FormFitter::fitting_complete() {
//  while (player->state() == QMediaPlayer::PlayingState)
//    player->stop();

//  if (player->state() != QMediaPlayer::PlayingState) {
//    player->setMedia(QUrl("qrc:/sounds/laser12.wav"));
//    player->setVolume(100);
//    player->setPosition(0);
//    player->play();
////    while (player->state() == QMediaPlayer::PlayingState) {}
//  }

  busy_= false;
  calc_visible();
  ui->plot->replot();
  toggle_push();
  emit data_changed();
  emit fitting_done();
}

void FormFitter::toggle_push() {
  if (fit_data_ == nullptr)
    return;

  ui->pushStopFitter->setEnabled(busy_);
  ui->pushFindPeaks->setEnabled(!busy_);
  ui->pushSettings->setEnabled(!busy_);
  ui->labelMovie->setVisible(busy_);
  emit fitter_busy(busy_);
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
    floatingText->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    floatingText->position->setType(QCPItemPosition::ptAxisRectRatio);
    floatingText->position->setCoords(1, 0); // place position at center/top of axis rect
    floatingText->setText(title_text_);
    floatingText->setProperty("floating text", true);
    floatingText->setFont(QFont("Helvetica", 10));
    floatingText->setSelectable(false);
    floatingText->setColor(Qt::black);
  }
}

void FormFitter::set_range(Range rng) {
  //  DBG << "<FormFitter> set range";
  range_ = rng;
  plotRange();
  if (range_.visible) {
    selected_peaks_.clear();
    follow_selection();
  }
  calc_visible();
  ui->plot->replot();
}

QCPGraph *FormFitter::addGraph(const QVector<double>& x, const QVector<double>& y,
                               QPen appearance, bool smooth, QString name,
                               double region, double peak) {

  if (x.empty() || y.empty() || (x.size() != y.size()))
    return nullptr;

  ui->plot->addGraph();
  QCPGraph *target = ui->plot->graph(ui->plot->graphCount() - 1);

  target->addData(x, y);
  target->setPen(appearance);
  target->setName(name);
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

  if (x[0] < minx) {
    minx = x[0];
    //DBG << "new minx " << minx;
    ui->plot->xAxis->rescale();
    ui->plot->xAxis->rescale();
  }
  if (x[x.size() - 1] > maxx) {
    maxx = x[x.size() - 1];
//    ui->plot->xAxis->rescale();
  }

  return target;
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

  //DBG << "Rezoom";

//  if (miny <= 0)
//    ui->plot->yAxis->rescale();
//  else
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

  if ((miny2 > 0) && (miny2 < 1.0))
    miny2 = 0;
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
             ui->plot->item(i)->property("button_name").toString().contains("commit"))
      to_remove.push_back(ui->plot->item(i));
  }
  for (auto &q : to_remove)
    ui->plot->removeItem(q);

  edge_trc1 = nullptr;
  edge_trc2 = nullptr;

  if (range_.visible) {

    double pos_l = 0, pos_r = 0;
    pos_l = range_.l.energy();
    pos_r = range_.r.energy();

    if (pos_l < pos_r) {

      int idx = -1;

      for (auto &q : range_.latch_to) {
        for (int i=0; i < ui->plot->graphCount(); i++) {
          if ((ui->plot->graph(i)->data()->firstKey() > pos_l)
              || (pos_r > ui->plot->graph(i)->data()->lastKey()))
            continue;

          if (ui->plot->graph(i)->name() == q ) {
            idx = i;
            break;
          }
        }
        if (idx != -1)
          break;
      }

      if (idx >= 0) {
        edge_trc1 = new QCPItemTracer(ui->plot);
        edge_trc1->setStyle(QCPItemTracer::tsNone);
        edge_trc1->setGraph(ui->plot->graph(idx));
        edge_trc1->setGraphKey(pos_l);
        edge_trc1->setInterpolating(true);
        edge_trc1->setProperty("tracer", QVariant::fromValue(0));
        ui->plot->addItem(edge_trc1);
        edge_trc1->updatePosition();

        edge_trc2 = new QCPItemTracer(ui->plot);
        edge_trc2->setStyle(QCPItemTracer::tsNone);
        edge_trc2->setGraph(ui->plot->graph(idx));
        edge_trc2->setGraphKey(pos_r);
        edge_trc2->setInterpolating(true);
        edge_trc2->setProperty("tracer", QVariant::fromValue(0));
        ui->plot->addItem(edge_trc2);
        edge_trc2->updatePosition();

      } else
        range_.visible = false;
    } else
      range_.visible = false;
  }

  if (range_.visible) {
      DraggableTracer *ar1 = new DraggableTracer(ui->plot, edge_trc1, 12);
      ar1->setPen(QPen(range_.appearance.default_pen.color(), 1));
      ar1->setSelectedPen(QPen(range_.appearance.default_pen.color(), 1));
      ar1->setProperty("tracer", QVariant::fromValue(1));
      ar1->setSelectable(true);
      ar1->set_limits(minx, maxx);
      ui->plot->addItem(ar1);

      DraggableTracer *ar2 = new DraggableTracer(ui->plot, edge_trc2, 12);
      ar2->setPen(QPen(range_.appearance.default_pen.color(), 1));
      ar2->setSelectedPen(QPen(range_.appearance.default_pen.color(), 1));
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
      line->setPen(QPen(range_.appearance.default_pen.color(), 2, Qt::DashLine));
      line->setSelectedPen(QPen(range_.appearance.default_pen.color(), 2, Qt::DashLine));
      ui->plot->addItem(line);

      QCPItemTracer* higher = edge_trc1;
      if (edge_trc2->position->value() > edge_trc1->position->value())
        higher = edge_trc2;

      QCPOverlayButton *newButton = nullptr;

      QString purpose("");
      if (range_.property("purpose").isValid())
        purpose = range_.property("purpose").toString();

      if (purpose == "add peak") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/edit_add.png"),
                        "add peak commit", "Add peak",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (purpose == "SUM4") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_blue.png"),
                        "SUM4 commit", "Adjust SUM4 peak bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (purpose == "background L") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_blue.png"),
                        "background L commit", "Adjust left background",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (purpose == "background R") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_blue.png"),
                        "background R commit", "Adjust right background",
                        Qt::AlignTop | Qt::AlignLeft);
      } else if (purpose == "region bounds") {
        newButton = new QCPOverlayButton(ui->plot,
                        QPixmap(":/icons/oxy/22/flag_red.png"),
                        "region bounds commit", "Adjust region bounds",
                        Qt::AlignTop | Qt::AlignLeft);
      }

      if (newButton) {
        newButton->bottomRight->setParentAnchor(higher->position);
        newButton->bottomRight->setCoords(11, -20);
        ui->plot->addItem(newButton);
      }
  }
}

void FormFitter::plotEnergyLabel(double peak_id, double peak_energy, QCPItemTracer *crs) {
      QPen pen = QPen(Qt::darkGray, 1);
      QPen selected_pen = QPen(Qt::black, 2);

      QCPItemLine *line = new QCPItemLine(ui->plot);
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
      ui->plot->addItem(line);

      QCPItemText *markerText = new QCPItemText(ui->plot);
      markerText->position->setParentAnchor(crs->position);
      markerText->setPositionAlignment(Qt::AlignHCenter|Qt::AlignBottom);
      markerText->position->setCoords(0, -35);
      markerText->setText(QString::number(peak_energy));
      markerText->setTextAlignment(Qt::AlignLeft);
      markerText->setFont(QFont("Helvetica", 9));
      markerText->setPen(pen);
      markerText->setColor(pen.color());
      markerText->setSelectedColor(selected_pen.color());
      markerText->setSelectedPen(selected_pen);
      markerText->setPadding(QMargins(1, 1, 1, 1));
      markerText->setSelectable(true);
      markerText->setSelected(selected_peaks_.count(peak_id));
      markerText->setProperty("region", crs->property("region"));
      markerText->setProperty("peak", crs->property("peak"));
      ui->plot->addItem(markerText);

      //make this optional?
      QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
            QPixmap(":/icons/oxy/22/help_about.png"),
            "peak_info", "Do sum4 stuff",
            Qt::AlignTop | Qt::AlignLeft
            );
      newButton->topLeft->setType(QCPItemPosition::ptAbsolute);
      newButton->topLeft->setParentAnchor(markerText->topRight);
      newButton->topLeft->setCoords(5, 0);
      newButton->setProperty("region", crs->property("region"));
      newButton->setProperty("peak", crs->property("peak"));
      ui->plot->addItem(newButton);
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
  newButton->topLeft->setCoords(0, 32);
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

  const Qpx::ROI &r = fit_data_->regions_.at(bin);

  if (r.finder_.x_.empty())
    return;

  double left = r.finder_.x_[0];
  double right = r.finder_.x_[r.finder_.x_.size() - 1];

  range_.setProperty("purpose", QString("region bounds"));
  range_.visible = true;
  range_.appearance.default_pen = QPen(Qt::red);
  range_.setProperty("region", QVariant::fromValue(bin));
  range_.l.set_bin(left, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.r.set_bin(right, fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.latch_to.clear();
//  range_.latch_to.push_back("Background poly");
  range_.latch_to.push_back("Data");

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
        if (txt->property("peak").isValid())
          selected_peaks_.insert(txt->property("peak").toDouble());
      } else if (QCPItemLine *line = qobject_cast<QCPItemLine*>(q)) {
        if (line->property("label").isValid() && line->property("peak").isValid())
          selected_peaks_.insert(line->property("peak").toDouble());
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
  //  INFO << "<FormFitter> clickedplottable";
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
      else if (button->name() == "SUM4 begin")
        make_SUM4_range(button->property("region").toDouble(), button->property("peak").toDouble());
      else if (button->name() == "background L begin")
        make_background_range(button->property("region").toDouble(), true);
      else if (button->name() == "background R begin")
        make_background_range(button->property("region").toDouble(), false);
      else if (button->name() == "region options")
      {
        if (!busy_) {
          menuROI.setProperty("region", button->property("region"));
          menuROI.exec(QCursor::pos());
        }
      }
      else if (button->name() == "region bounds commit")
        adjust_roi_bounds();
      else if (button->name() == "add peak commit")
        add_peak();
      else if (button->name() == "delete peaks")
        delete_selected_peaks();
      else if (button->name() == "SUM4 commit")
        adjust_sum4();
      else if (button->name() == "background L commit")
        adjust_background();
      else if (button->name() == "background R commit")
        adjust_background();
      else if (button->name() == "peak_info")
        peak_info(button->property("peak").toDouble());
  }
}

void FormFitter::make_SUM4_range(double region, double peak)
{
  if (fit_data_ == nullptr)
    return;

  if (!fit_data_->regions_.count(region))
    return;

  const Qpx::ROI &parent_region = fit_data_->regions_.at(region);

  Qpx::Peak pk = fit_data_->peaks().at(peak);
  range_.setProperty("purpose", QString("SUM4"));
  range_.latch_to.clear();
  range_.latch_to.push_back("Background sum4");
  range_.appearance.default_pen = QPen(Qt::darkCyan);
  range_.setProperty("region", QVariant::fromValue(region));
  range_.setProperty("peak", QVariant::fromValue(peak));

  range_.l.set_bin(pk.sum4().left(), fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.r.set_bin(pk.sum4().right(), fit_data_->settings().bits_, fit_data_->settings().cali_nrg_);
  range_.visible = true;

//  selected_peaks_.clear();
  plotRange();
  calc_visible();
  ui->plot->replot();
  toggle_push();

  emit peak_selection_changed(selected_peaks_);

}


void FormFitter::make_background_range(double roi, bool left)
{
  if (fit_data_ == nullptr)
    return;

  if (!fit_data_->regions_.count(roi))
    return;

  const Qpx::ROI &parent_region = fit_data_->regions_.at(roi);

  int32_t L = 0;
  int32_t R = 0;

  if (left) {
    range_.setProperty("purpose", "background L");
    L = parent_region.LB().left();
    R = parent_region.LB().right();
  } else {
    range_.setProperty("purpose", "background R");
    L = parent_region.RB().left();
    R = parent_region.RB().right();
  }

  double l = fit_data_->settings().cali_nrg_.transform(L, fit_data_->settings().bits_);
  double r = fit_data_->settings().cali_nrg_.transform(R, fit_data_->settings().bits_);

  range_.setProperty("region",  QVariant::fromValue(roi));
  range_.l.set_energy(l, fit_data_->settings().cali_nrg_);
  range_.r.set_energy(r, fit_data_->settings().cali_nrg_);
  range_.latch_to.clear();
  range_.latch_to.push_back("Data");
  range_.appearance.default_pen = QPen(Qt::darkMagenta);
  range_.visible = true;

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
  double region = menuROI.property("region").toDouble();
  if (choice == "History...") {
    rollback_ROI(region);
  } else if (choice == "Adjust bounds") {
    createROI_bounds_range(region);
  } else if (choice == "Region settings...") {
    roi_settings(region);
  } else if (choice == "Adjust background") {
    selected_roi_ = region;
    calc_visible();
    ui->plot->replot();
  } else if (choice == "Refit") {
    refit_ROI(region);
  } else if (choice == "Delete") {
    delete_ROI(region);
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
      //      INFO << "Exporting plot to png " << fileName.toStdString();
      ui->plot->savePng(fileName,0,0,1,100);
    } else if (file.suffix() == "jpg") {
      //      INFO << "Exporting plot to jpg " << fileName.toStdString();
      ui->plot->saveJpg(fileName,0,0,1,100);
    } else if (file.suffix() == "bmp") {
      //      INFO << "Exporting plot to bmp " << fileName.toStdString();
      ui->plot->saveBmp(fileName);
    } else if (file.suffix() == "pdf") {
      //      INFO << "Exporting plot to pdf " << fileName.toStdString();
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

  QColor color_data;
  color_data.setHsv(QColor(QString::fromStdString(fit_data_->metadata_.attributes.branches.get(Qpx::Setting("appearance")).value_text)).hsvHue(), 48, 160);
  QPen pen_data = QPen(color_data, 1);

  ui->plot->clearGraphs();
  ui->plot->clearItems();

  minima_.clear(); maxima_.clear();

  ui->plot->xAxis->setLabel(QString::fromStdString(fit_data_->settings().cali_nrg_.units_));
  ui->plot->yAxis->setLabel("count");

  QVector<double> xx, yy;

  xx = QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(fit_data_->finder_.x_, fit_data_->settings().bits_));
  yy = QVector<double>::fromStdVector(fit_data_->finder_.y_);

  QCPGraph *data_graph = addGraph(xx, yy, pen_data, false, "Data");
  add_bounds(xx, yy);

  for (auto &r : fit_data_->regions_)
    plotRegion(r.first, r.second, data_graph);

  plotRange();
  plotButtons();
  plotTitle();

  calc_visible();
  plot_rezoom(true);
  ui->plot->replot();
}

void FormFitter::plotRegion(double region_id, Qpx::ROI &region, QCPGraph *data_graph)
{
  QPen pen_back_sum4 = QPen(Qt::darkMagenta, 1);
  QPen pen_back_poly = QPen(Qt::darkGray, 1);
  QPen pen_back_with_steps = QPen(Qt::darkBlue, 1);
  QPen pen_full_fit = QPen(Qt::red, 1);

  QPen pen_peak  = QPen(Qt::darkCyan, 1);
  pen_peak.setStyle(Qt::DotLine);

  QPen pen_resid = QPen( Qt::darkGreen, 1);
  pen_resid.setStyle(Qt::DashLine);

//  QColor flagged_color;
//  flagged_color.setHsv(QColor(Qt::green).hsvHue(), QColor(Qt::green).hsvSaturation(), 192);
//  QPen flagged =  QPen(flagged_color, 1);
//  flagged.setStyle(Qt::DotLine);

  QVector<double> xx;
  QVector<double> yy;

  if (!region.settings_.sum4_only) {
    xx = QVector<double>::fromStdVector(region.hr_x_nrg);
    yy = QVector<double>::fromStdVector(region.hr_fullfit);
//    trim_log_lower(yy);
    addGraph(xx, yy, pen_full_fit, true, "Region fit", region_id);
    add_bounds(xx, yy);
  } else {
    xx = QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(region.finder_.x_, fit_data_->settings().bits_));
    yy = QVector<double>::fromStdVector(region.finder_.y_);
    addGraph(xx, yy, pen_full_fit, false, "Region fit", region_id);
  }


  if (!region.finder_.x_.empty()) {
    plotBackgroundEdge(region.LB(), region.finder_.x_, region_id, "background L begin");
    plotBackgroundEdge(region.RB(), region.finder_.x_, region_id, "background R begin");

    QVector<double> xb;
    QVector<double> yb;

    xb.push_back(fit_data_->settings().cali_nrg_.transform(region.finder_.x_.front(), fit_data_->settings().bits_));
    xb.push_back(fit_data_->settings().cali_nrg_.transform(region.finder_.x_.back(), fit_data_->settings().bits_));
    yb.push_back(region.sum4_background_.eval(region.finder_.x_.front()));
    yb.push_back(region.sum4_background_.eval(region.finder_.x_.back()));

    addGraph(xb, yb, pen_back_sum4, true, "Background sum4", region_id);
    add_bounds(xb, yb);
  }

  if (!region.peaks_.empty() && !region.settings_.sum4_only) {

    yy = QVector<double>::fromStdVector(region.hr_background);
//    trim_log_lower(yy);
    addGraph(xx, yy, pen_back_poly, true, "Background poly", region_id);
    add_bounds(xx, yy);

    yy = QVector<double>::fromStdVector(region.hr_back_steps);
//    trim_log_lower(yy);
    addGraph(xx, yy, pen_back_with_steps, true, "Background steps", region_id);

    QVector<double> x = QVector<double>::fromStdVector(fit_data_->settings().cali_nrg_.transform(region.finder_.x_, fit_data_->settings().bits_));
    QVector<double> y = QVector<double>::fromStdVector(region.finder_.y_resid_on_background_);
//      trim_log_lower(yy); //maybe not?
    addGraph(x, y, pen_resid, false, "Residuals", region_id);
//      add_bounds(xx, yy);
  }

  if (!region.hr_x_nrg.empty()) {
    QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/applications_systemg.png"),
          "region options", "Region options",
          Qt::AlignTop | Qt::AlignLeft
          );
    newButton->topLeft->setType(QCPItemPosition::ptPlotCoords);
    newButton->topLeft->setCoords(region.hr_x_nrg.front(), region.hr_fullfit.front());
    newButton->setProperty("region", QVariant::fromValue(region_id));
    ui->plot->addItem(newButton);
  }

  for (auto & p : region.peaks_) {
    QCPGraph *peak_graph = nullptr;
    if (!region.settings_.sum4_only) {
      yy = QVector<double>::fromStdVector(p.second.hr_fullfit_);
//      trim_log_lower(yy);
      //      QPen pen = p.second.flagged ? flagged : peak;
      peak_graph = addGraph(xx, yy, pen_peak, true, "Individual peak", region_id, p.first);
    }

    QCPItemTracer *crs = new QCPItemTracer(ui->plot);
    crs->setStyle(QCPItemTracer::tsNone);
    crs->setProperty("region", region_id);
    crs->setProperty("peak", p.first);
    if (peak_graph)
      crs->setGraph(peak_graph);
    else
      crs->setGraph(data_graph);
    crs->setInterpolating(true);
    crs->setGraphKey(p.second.energy().value());
    ui->plot->addItem(crs);
    crs->updatePosition();

    plotEnergyLabel(p.first, p.second.energy().value(), crs);
    plotPeak(region_id, p.first, p.second);
  }

}

void FormFitter::plotPeak(double region_id, double peak_id, Qpx::Peak &peak)
{
  QPen pen_sum4_peak     = QPen(Qt::darkYellow, 3);
  QPen pen_sum4_selected = QPen(Qt::darkCyan, 3);

  if (peak.sum4().peak_width()) {

    double x1 = fit_data_->settings().cali_nrg_.transform(peak.sum4().left() - 0.5,
                                                          fit_data_->settings().bits_);
    double x2 = fit_data_->settings().cali_nrg_.transform(peak.sum4().right() + 0.5,
                                                          fit_data_->settings().bits_);
    double y1 = fit_data_->finder_.y_.at(fit_data_->finder_.find_index(peak.sum4().left()));
    double y2 = fit_data_->finder_.y_.at(fit_data_->finder_.find_index(peak.sum4().right()));

    QCPItemLine *line = new QCPItemLine(ui->plot);
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
    ui->plot->addItem(line);


    QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
          QPixmap(":/icons/oxy/22/system_switch_user.png"),
          "SUM4 begin", "Do sum4 stuff",
          Qt::AlignTop | Qt::AlignLeft
          );
    newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
    newButton->bottomRight->setCoords(x1, y1);
    newButton->setProperty("peak", QVariant::fromValue(peak_id));
    newButton->setProperty("region", QVariant::fromValue(region_id));
    ui->plot->addItem(newButton);
  }
}

void FormFitter::plotBackgroundEdge(Qpx::SUM4Edge edge,
                                    std::vector<double> &x,
                                    double region_id,
                                    QString button_name) {

  QPen pen_background_edge = QPen(Qt::darkMagenta, 3);


  double x1 = fit_data_->settings().cali_nrg_.transform(edge.left() - 0.5,
                                                        fit_data_->settings().bits_);
  double x2 = fit_data_->settings().cali_nrg_.transform(edge.right() + 0.5,
                                                        fit_data_->settings().bits_);
  double y = edge.average();

  QCPItemLine *line = new QCPItemLine(ui->plot);
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
  ui->plot->addItem(line);

  QCPOverlayButton *newButton = new QCPOverlayButton(ui->plot,
                   QPixmap(":/icons/oxy/22/system_switch_user.png"),
                   button_name, "Set background edge",
                   Qt::AlignTop | Qt::AlignLeft
                   );
  newButton->bottomRight->setType(QCPItemPosition::ptPlotCoords);
  newButton->bottomRight->setCoords(x1, y);
  newButton->setProperty("region", QVariant::fromValue(region_id));
  newButton->setProperty("peak", QVariant::fromValue(-1));
  ui->plot->addItem(newButton);
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
      if ((p.second.energy().value() >= minx_zoom) && (p.second.energy().value() <= maxx_zoom))
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

  bool adjusting_background = range_.visible && range_.property("purpose").isValid() &&
      ((range_.property("purpose").toString() == "background R") ||
       (range_.property("purpose").toString() == "background L"))
       && !busy_;

  bool adjusting_sum4 = range_.visible && range_.property("purpose").isValid() &&
       (range_.property("purpose").toString() == "SUM4")
        && !busy_;
  bool selected_one = ((selected_peaks_.size() == 1)  && !busy_);

  bool button_background     = (selected_one || (selected_roi_ >=0)) && !busy_;
  bool line_background       = (button_background || adjusting_sum4) && !adjusting_background;
  bool line_sum4_selpeak     = button_background || adjusting_background;

  this->blockSignals(true);
  for (int i=0; i < ui->plot->itemCount(); ++i) {
    QCPAbstractItem *q =  ui->plot->item(i);
    double peak = -1;
    double region = -1;
    if (q->property("region").isValid())
      region = q->property("region").toDouble();
    if (q->property("peak").isValid())
      peak = q->property("peak").toDouble();
    if ((peak < 0) && (region >= 0) && selected_one
             && fit_data_->regions_.at(region).peaks_.count(*selected_peaks_.begin())) {
      peak = *selected_peaks_.begin();
      q->setProperty("peak", QVariant::fromValue(peak));
    }

    bool this_peak_selected = (selected_peaks_.count(peak) > 0);
    bool this_region_selected = !selected_peaks_.empty()
        && fit_data_->regions_.count(region)
        && fit_data_->regions_[region].peaks_.count(peak)
        && fit_data_->regions_[region].peaks_.count(*selected_peaks_.begin());
    if ((selected_roi_ == region) && (region >= 0))
      this_region_selected = true;
    bool range_this_peak = ((range_.property("peak").toDouble() == peak) && range_.visible);
    bool range_this_region = ((range_.property("region").toDouble() == region) && range_.visible);

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
    } else if (QCPOverlayButton *button = qobject_cast<QCPOverlayButton*>(q)) {
      if ((button->name() == "background L begin") || (button->name() == "background R begin")) {
        button->setVisible((this_region_selected || this_peak_selected)
                           && button_background && !range_.visible);
      } else if (button->name() == "region options") {
        button->setVisible(prime_roi.count(button->property("region").toDouble())
                           && selected_peaks_.empty()
                           && !range_.visible
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

  int total = ui->plot->graphCount();
  for (int i=0; i < total; i++) {
    QCPGraph *graph = ui->plot->graph(i);

    double peak = -1;
    double region = -1;
    if (graph->property("region").isValid())
      region = graph->property("region").toDouble();
    if (graph->property("peak").isValid())
      peak = graph->property("peak").toDouble();
    if ((peak < 0) && (region >= 0) && selected_one
             && fit_data_->regions_.at(region).peaks_.count(*selected_peaks_.begin())) {
      peak = *selected_peaks_.begin();
      graph->setProperty("peak", QVariant::fromValue(peak));
    }

    bool this_peak_selected = (selected_peaks_.count(peak) > 0);
    bool this_region_selected = !selected_peaks_.empty()
        && fit_data_->regions_.count(region)
        && fit_data_->regions_[region].peaks_.count(peak)
        && fit_data_->regions_[region].peaks_.count(*selected_peaks_.begin());
    if (selected_roi_ == region)
      this_region_selected = true;
    bool range_this_peak = (range_.property("peak").toDouble() == peak);
    bool range_this_region = (range_.property("region").toDouble() == region);
    bool region_sum4_only = (fit_data_->regions_.count(region)
                             && fit_data_->regions_.at(region).settings_.sum4_only);


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
      graph->setVisible(plot_resid_on_back && good);
    else if (graph->name() == "Background sum4") {
      graph->setVisible((range_this_region && adjusting_sum4) ||
                        (this_region_selected && region_sum4_only));
    }
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

void FormFitter::roi_settings(double roi)
{
  if (!fit_data_ || !fit_data_->regions_.count(roi))
    return;

  Qpx::ROI& target  = fit_data_->regions_[roi];

  FitSettings fs = target.settings_;

  FormFitterSettings *FitterSettings = new FormFitterSettings(fs, this);
  FitterSettings->setWindowTitle("Region settings");
  int ret = FitterSettings->exec();

  if (ret == QDialog::Accepted) {
    // needs function to save previous state to history?
    target.settings_ = fs;
    updateData();
  }

}

void FormFitter::peak_info(double bin)
{
  if (!fit_data_)
    return;

  Qpx::ROI *parent_region = fit_data_->parent_of(bin);
  if (!parent_region || !fit_data_->peaks().count(bin))
    return;


//  DBG << "Peak info for " << fit_data_->peaks().at(bin).energy;

  Hypermet hm = fit_data_->peaks().at(bin).hypermet();
  FormPeakInfo *peakInfo = new FormPeakInfo(hm, this);
  peakInfo->setWindowTitle("Parameters for peak at " + QString::number(fit_data_->peaks().at(bin).energy().value()));
  int ret = peakInfo->exec();

  if (ret == QDialog::Accepted) {
    Qpx::Peak pk = fit_data_->peaks().at(bin);
//    if (pk.hypermet_ == hm)
//      return;

    pk = Qpx::Peak(hm, pk.sum4(), parent_region->settings_);

//    pk.hypermet_ = hm;

    //what if centroid has changed?
    fit_data_->replace_peak(pk);

    selected_peaks_.clear();
    selected_peaks_.insert(pk.center().value());

    hold_selection_ = true;

    updateData();
    calc_visible();

    emit data_changed();
    emit peak_selection_changed(selected_peaks_);
  }
}
