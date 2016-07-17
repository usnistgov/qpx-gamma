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
 *      List mode interface
 *
 ******************************************************************************/

#include "form_list_daq.h"
#include "ui_form_list_daq.h"
#include "custom_logger.h"
#include "qt_util.h"
#include <QSettings>


FormListDaq::FormListDaq(ThreadRunner &thread, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormListDaq),
  runner_thread_(thread),
  spill_detectors_("Detectors"),
  attr_model_(this),
  interruptor_(false),
  my_run_(false)
{
  ui->setupUi(this);

  this->setWindowTitle("List LIVE");

  loadSettings();

  connect(&runner_thread_, SIGNAL(listComplete(Qpx::ListData)), this, SLOT(list_completed(Qpx::ListData)));

  connect(ui->listSpills, SIGNAL(currentRowChanged(int)), this, SLOT(spillSelectionChanged(int)));

  ui->tableHits->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableHits->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableHits->horizontalHeader()->setStretchLastSection(true);
  ui->tableHits->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableHits->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(hit_selection_changed(QItemSelection,QItemSelection)));

  ui->tableStats->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableStats->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableStats->horizontalHeader()->setStretchLastSection(true);
  ui->tableStats->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableStats->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(stats_selection_changed(QItemSelection,QItemSelection)));

  ui->tableHitValues->setSelectionMode(QAbstractItemView::NoSelection);
  ui->tableHitValues->horizontalHeader()->setStretchLastSection(true);
  ui->tableHitValues->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  det_table_model_.setDB(spill_detectors_);
  ui->tableDetectors->setModel(&det_table_model_);
  ui->tableDetectors->horizontalHeader()->setStretchLastSection(true);
  ui->tableDetectors->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableDetectors->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableDetectors->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableDetectors->show();

  ui->treeAttribs->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->treeAttribs->setModel(&attr_model_);
  ui->treeAttribs->setItemDelegate(&attr_delegate_);
  ui->treeAttribs->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  attr_model_.set_show_address_(false);

  ui->timeDuration->set_us_enabled(false);

  ui->treeAttribs->setVisible(false);
  ui->labelState->setVisible(false);

  ui->tableDetectors->setVisible(false);
  ui->labelDetectors->setVisible(false);

//  ui->labelStats->setVisible(false);
//  ui->tableStats->setVisible(false);
//  ui->labelStatsInfo->setVisible(false);

//  ui->labelEvents->setVisible(false);
//  ui->tableHits->setVisible(false);
//  ui->labelEventVals->setVisible(false);
//  ui->tableHitValues->setVisible(false);
}

void FormListDaq::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("ListDaq");
  ui->timeDuration->set_total_seconds(settings_.value("run_secs", 60).toULongLong());
  settings_.endGroup();
}

void FormListDaq::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("ListDaq");
  settings_.setValue("run_secs", QVariant::fromValue(ui->timeDuration->total_seconds()));
  settings_.endGroup();
}

void FormListDaq::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);
  ui->pushListStart->setEnabled(enable && online);
  ui->timeDuration->setEnabled(enable && online);
}

FormListDaq::~FormListDaq()
{
  delete ui;
}

void FormListDaq::closeEvent(QCloseEvent *event) {
  if (my_run_ && runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      runner_thread_.terminate();
      runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }


  if (!list_data_.empty()) {
    int reply = QMessageBox::warning(this, "Contents present",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
    {
      event->ignore();
      return;
    }
  }

  saveSettings();
  event->accept();
}

void FormListDaq::displayHit(int idx)
{
  ui->tracePlot->clearGraphs();

  if ( (idx < 0) || (idx >= hits_.size()))
  {
    ui->tableHitValues->clear();
    ui->tracePlot->replot();
    return;
  }

  Qpx::Hit hit = hits_.at(idx);
  int chan = hit.source_channel();
  Qpx::HitModel model;
  if (hitmodels_.count(chan))
    model = hitmodels_.at(chan);

  ui->tableHitValues->setRowCount(hit.value_count());
  ui->tableHitValues->setColumnCount(3);
  ui->tableHitValues->setHorizontalHeaderItem(0, new QTableWidgetItem("Name", QTableWidgetItem::Type));
  ui->tableHitValues->setHorizontalHeaderItem(1, new QTableWidgetItem("Value", QTableWidgetItem::Type));
  ui->tableHitValues->setHorizontalHeaderItem(2, new QTableWidgetItem("Calibrated", QTableWidgetItem::Type));

  for (int i = 0; i < hit.value_count(); ++i)
  {
    if (i < model.idx_to_name.size())
      add_to_table(ui->tableHitValues, i, 0, model.idx_to_name.at(i));
    else
      add_to_table(ui->tableHitValues, i, 0, std::to_string(i));
    add_to_table(ui->tableHitValues, i, 1, hit.value(i).to_string());


    if ((chan > -1) && (chan < dets_.size()))
    {
      Qpx::Detector det = dets_.at(chan);

      Qpx::Calibration cal;
      if ((i < model.idx_to_name.size()) && (model.idx_to_name.at(i) == "energy"))
        cal = det.best_calib(hit.value(i).bits());

      if (cal.valid()) {
        double energy = cal.transform(hit.value(i).val(hit.value(i).bits()), hit.value(i).bits());
        std::string nrg = to_str_decimals(energy, 0);
        if (cal.valid())
          nrg += " " + cal.units_;

        add_to_table(ui->tableHitValues, i, 2, nrg);
      }
    }

  }

  uint32_t trace_length = hit.trace().size();
  if (trace_length > 0) {
    QVector<double> x(trace_length), y(trace_length);
    int chan = hit.source_channel();
    Qpx::Detector this_det;

    //    if ((chan > -1) && (chan < list_data_->run.detectors.size()))
    //      this_det = list_data_->run.detectors[chan];

    Qpx::Calibration this_calibration = this_det.best_calib(16);

    for (std::size_t j=0; j<trace_length; j++) {
      x[j] = j;
      y[j] = this_calibration.transform(hit.trace().at(j), 16);
    }

    ui->tracePlot->addGraph();
    ui->tracePlot->graph(0)->addData(x, y);
    ui->tracePlot->graph(0)->setPen(QPen(Qt::darkGreen));
  }

  ui->tracePlot->xAxis->setLabel("time (ticks)"); //can do better....
  ui->tracePlot->yAxis->setLabel("keV");
  ui->tracePlot->rescaleAxes();
  ui->tracePlot->replot();
}

void FormListDaq::displayStats(int idx)
{
  if ( (idx < 0) || (idx >= stats_.size()))
  {
    ui->labelStatsInfo->clear();
    return;
  }

  Qpx::StatsUpdate stats;

  int i=0;
  for (auto &s : stats_)
  {
    if (i == idx)
      stats = s.second;
    i++;
  }

  std::string info = stats.model_hit.to_string();

  i = 0;
  for (auto &q : stats.items)
  {
    info += "\n" + q.first + " = " + to_string( q.second );
    i++;
  }

  ui->labelStatsInfo->setText(QString::fromStdString(info));
}

void FormListDaq::on_pushListStart_clicked()
{
  if (!list_data_.empty()) {
    int reply = QMessageBox::warning(this, "Contents present",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply != QMessageBox::Yes)
      return;
    my_run_ = true;
    list_completed(Qpx::ListData());
  }

  emit statusText("List mode acquisition in progress...");

  this->setWindowTitle("List LIVE  \u25b6");

  emit toggleIO(false);
  ui->pushListStop->setEnabled(true);
  my_run_ = true;

  uint64_t duration = ui->timeDuration->total_seconds();

  if (duration == 0)
    return;

  runner_thread_.do_list(interruptor_, duration);
}

void FormListDaq::on_pushListStop_clicked()
{
  ui->pushListStop->setEnabled(false);
  LINFO << "List acquisition interrupted by user";
  interruptor_.store(true);
}

void FormListDaq::list_completed(Qpx::ListData newEvents) {
  if (my_run_) {
    list_data_ = newEvents;

    ui->listSpills->clear();
    for (auto &s : list_data_)
      ui->listSpills->addItem(QString::fromStdString(s->to_string()));

    ui->pushListStop->setEnabled(false);
    this->setWindowTitle("List LIVE");

    emit toggleIO(true);
    my_run_ = false;
  }
}

void FormListDaq::spillSelectionChanged(int row)
{
  this->setCursor(Qt::WaitCursor);
  hits_.clear();
  dets_.clear();
  hitmodels_.clear();
  stats_.clear();
  spill_detectors_.clear();
  attr_model_.update(Qpx::Setting());
  ui->tableStats->clearSelection();

  ui->treeAttribs->setVisible(false);
  ui->labelState->setVisible(false);

  ui->tableDetectors->setVisible(false);
  ui->labelDetectors->setVisible(false);

//  ui->labelStats->setVisible(false);
//  ui->tableStats->setVisible(false);
//  ui->labelStatsInfo->setVisible(false);

//  ui->labelEvents->setVisible(false);
//  ui->tableHits->setVisible(false);
//  ui->labelEventVals->setVisible(false);
//  ui->tableHitValues->setVisible(false);

  if ((row >= 0) && (row < list_data_.size()))
  {

    for (size_t i = 0; i < row; i++)
    {
      Qpx::SpillPtr sp = list_data_.at(i);
      if (!sp)
        continue;
      if (sp->detectors.size())
        for (int di = 0; di < sp->detectors.size(); di++)
        {
          if ((di+1) > dets_.size())
            dets_.resize(di+1);
          dets_[di] = sp->detectors.at(di);
        }
      for (auto &stats : sp->stats)
      {
        hitmodels_[stats.first] = stats.second.model_hit;
//        DBG << "s" << i <<  "c" << stats.first << " = " << stats.second.model_hit.to_string();
      }
    }


    Qpx::SpillPtr sp = list_data_.at(row);
    if (sp)
    {
      hits_ = std::vector<Qpx::Hit>(sp->hits.begin(), sp->hits.end());
      stats_ = sp->stats;
      for (auto &q: sp->detectors)
        spill_detectors_.add_a(q);
      det_table_model_.update();
      attr_model_.update(sp->state);

      ui->treeAttribs->setVisible(sp->state != Qpx::Setting());
      ui->labelState->setVisible(sp->state != Qpx::Setting());

      ui->tableDetectors->setVisible(sp->detectors.size());
      ui->labelDetectors->setVisible(sp->detectors.size());

//      ui->labelStats->setVisible(stats_.size());
//      ui->tableStats->setVisible(stats_.size());
//      ui->labelStatsInfo->setVisible(stats_.size());

//      ui->labelEvents->setVisible(hits_.size());
//      ui->tableHits->setVisible(hits_.size());
//      ui->labelEventVals->setVisible(hits_.size());
//      ui->tableHitValues->setVisible(hits_.size());
    }
  }


  ui->tableHits->setRowCount(hits_.size());
  ui->tableHits->setColumnCount(3);
  ui->tableHits->setHorizontalHeaderItem(0, new QTableWidgetItem("Det", QTableWidgetItem::Type));
  ui->tableHits->setHorizontalHeaderItem(1, new QTableWidgetItem("Time (native)", QTableWidgetItem::Type));
  ui->tableHits->setHorizontalHeaderItem(2, new QTableWidgetItem("Time (ns)", QTableWidgetItem::Type));
  for (int i=0; i < hits_.size(); i++)
  {
    Qpx::Hit hit = hits_.at(i);
    int chan = hit.source_channel();

    std::string det = std::to_string(chan);
    if ((chan > -1) && (chan < dets_.size()))
      det += " (" + dets_[chan].name_ + ")";

    add_to_table(ui->tableHits, i, 0, det);
    add_to_table(ui->tableHits, i, 1, hit.timestamp().to_string() );
    add_to_table(ui->tableHits, i, 2, to_str_decimals(hit.timestamp().to_nanosec(), 0) );
  }


  ui->tableStats->setRowCount(stats_.size());
  ui->tableStats->setColumnCount(5);
  ui->tableStats->setHorizontalHeaderItem(0, new QTableWidgetItem("Chan", QTableWidgetItem::Type));
  ui->tableStats->setHorizontalHeaderItem(1, new QTableWidgetItem("Type", QTableWidgetItem::Type));
  ui->tableStats->setHorizontalHeaderItem(2, new QTableWidgetItem("Time", QTableWidgetItem::Type));
  ui->tableStats->setHorizontalHeaderItem(3, new QTableWidgetItem("Items", QTableWidgetItem::Type));
  ui->tableStats->setHorizontalHeaderItem(4, new QTableWidgetItem("Model", QTableWidgetItem::Type));
  int i= 0;
  for (auto &s : stats_)
  {
    int ch_i = s.first;
    int ch_d = s.second.source_channel;
    std::string ch = std::to_string(ch_i) + "(" + std::to_string(ch_d) + ")";

    add_to_table(ui->tableStats, i, 0, ch);
    add_to_table(ui->tableStats, i, 1, Qpx::StatsUpdate::type_to_str(s.second.stats_type) );
    add_to_table(ui->tableStats, i, 2, boost::posix_time::to_iso_extended_string(s.second.lab_time) );
    add_to_table(ui->tableStats, i, 3, std::to_string(s.second.items.size()) );
    add_to_table(ui->tableStats, i, 4, std::to_string(s.second.model_hit.name_to_idx.size()) );
    i++;
  }


  displayHit(-1);
  displayStats(-1);

  this->setCursor(Qt::ArrowCursor);
}

void FormListDaq::hit_selection_changed(QItemSelection, QItemSelection) {
  int idx = -1;
  if (!ui->tableHits->selectionModel()->selectedIndexes().empty())
    idx = ui->tableHits->selectionModel()->selectedIndexes().first().row();
  displayHit(idx);
}

void FormListDaq::stats_selection_changed(QItemSelection, QItemSelection) {
  int idx = -1;
  if (!ui->tableStats->selectionModel()->selectedIndexes().empty())
    idx = ui->tableStats->selectionModel()->selectedIndexes().first().row();
  displayStats(idx);
}
