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
  list_data_(nullptr),
  interruptor_(false),
  list_data_model_(),
  my_run_(false),
  list_selection_model_(&list_data_model_)
{
  ui->setupUi(this);

  this->setWindowTitle("List view");

  loadSettings();

  connect(&runner_thread_, SIGNAL(listComplete(Qpx::ListData*)), this, SLOT(list_completed(Qpx::ListData*)));

  ui->tableExtras->setSelectionMode(QAbstractItemView::NoSelection);
  ui->tableExtras->horizontalHeader()->setStretchLastSection(true);
  ui->tableExtras->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  list_data_model_.eat_list(list_data_);
  ui->listOutputView->setModel(&list_data_model_);
  ui->listOutputView->setSelectionModel(&list_selection_model_);
  ui->listOutputView->setItemDelegate(&list_delegate_);
  ui->listOutputView->horizontalHeader()->setStretchLastSection(true);
  ui->listOutputView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->listOutputView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->listOutputView->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->listOutputView->show();
  connect(&list_selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(list_selection_changed(QItemSelection,QItemSelection)));

  ui->timeDuration->set_us_enabled(false);

}

void FormListDaq::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();


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

  if (list_data_ != nullptr)
    delete list_data_;

  saveSettings();
  event->accept();
}

void FormListDaq::displayTraces()
{
  ui->tracePlot->clearGraphs();
  if (list_selection_model_.selectedIndexes().empty()) {
    ui->tracePlot->replot(); return;
  }

  long int chosen_trace = list_selection_model_.selectedIndexes().first().row();
  if (chosen_trace >= list_data_->hits.size()) {
    ui->tracePlot->replot(); return;
  }

  Qpx::Hit hit = list_data_->hits.at(chosen_trace);
  ui->tableExtras->setRowCount(hit.extras.size());
  ui->tableExtras->setColumnCount(2);
  ui->tableExtras->setHorizontalHeaderItem(0, new QTableWidgetItem("Name", QTableWidgetItem::Type));
  ui->tableExtras->setHorizontalHeaderItem(1, new QTableWidgetItem("Value", QTableWidgetItem::Type));

  int i=0;
  for (auto &e : hit.extras)
  {
    add_to_table(ui->tableExtras, i, 0, e.first);
    add_to_table(ui->tableExtras, i, 1, std::to_string(e.second));
    i++;
  }

  uint32_t trace_length = hit.trace.size();
  if (trace_length > 0) {
    QVector<double> x(trace_length), y(trace_length);
    int chan = list_data_->hits[chosen_trace].source_channel;
    Qpx::Detector this_det;

    if ((chan > -1) && (chan < list_data_->run.detectors.size()))
      this_det = list_data_->run.detectors[chan];

    Qpx::Calibration this_calibration = this_det.best_calib(16);

    for (std::size_t j=0; j<trace_length; j++) {
      x[j] = j;
      y[j] = this_calibration.transform(list_data_->hits[chosen_trace].trace[j], 16);
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

void FormListDaq::on_pushListStart_clicked()
{
  emit statusText("List mode acquisition in progress...");

  this->setWindowTitle("List view >>");

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
  INFO << "List acquisition interrupted by user";
  interruptor_.store(true);
}

void FormListDaq::list_completed(Qpx::ListData* newEvents) {
  if (my_run_) {
    if (list_data_ != nullptr)
      delete list_data_;
    list_data_ = newEvents;
    list_data_model_.eat_list(list_data_);
    list_data_model_.update();
    list_selection_model_.reset();
    displayTraces();

    ui->pushListStop->setEnabled(false);
    this->setWindowTitle("List view");

    emit toggleIO(true);
    my_run_ = false;
  }
}

void FormListDaq::list_selection_changed(QItemSelection, QItemSelection) {
  displayTraces();
}

TableListData::TableListData(QObject *parent)
  :QAbstractTableModel(parent), time_factor_(1.0)
{
}

int TableListData::rowCount(const QModelIndex & /*parent*/) const
{
  if (mystuff == nullptr)
    return 0;
  else
    return mystuff->hits.size();
}

int TableListData::columnCount(const QModelIndex & /*parent*/) const
{
  return 3;
}

QVariant TableListData::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();
  int chan = mystuff->hits[row].source_channel;
  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0: {
      QString det = QString::number(chan);
      if ((chan > -1) && (chan < dets_.size()))
        det += " [" + QString::fromStdString(dets_[chan].name_) + "]";
      return det;
    }
    case 1:
      return QString::number(mystuff->hits[row].timestamp.to_nanosec() * time_factor_);
//    case 2:
//      return QVariant::fromValue(QpxPattern(mystuff->hits[index.row()].pattern, false));
    case 2:
      Qpx::Calibration cal;
      Qpx::DigitizedVal en = mystuff->hits[row].energy;
      double energy = en.val(en.bits());
      if ((chan > -1) && (chan < dets_.size())) {
        cal = dets_[chan].best_calib(en.bits());
        energy = cal.transform(energy, en.bits());
      }
      return QString::number(energy) + QString::fromStdString(cal.units_);
    }
  }
  return QVariant();
}

QVariant TableListData::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("Detector");
      case 1:
        return QString("Time (s)");
      case 2:
        return QString("Energy");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}


void TableListData::eat_list(Qpx::ListData* stuff) {
  mystuff = stuff;
  if (stuff != nullptr) {
    dets_ = mystuff->run.detectors;
    time_factor_ = 1; //wrong!
  }
}

void TableListData::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( (rowCount() - 1), (columnCount() - 1) );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableListData::flags(const QModelIndex &index) const
{
  Qt::ItemFlags myflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index) & ~Qt::ItemIsSelectable;
  return myflags;
}
