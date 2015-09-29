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

#include "gui/form_list_daq.h"
#include "ui_form_list_daq.h"
#include "custom_logger.h"
#include "qt_util.h"

FormListDaq::FormListDaq(ThreadRunner &thread, QSettings &settings, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormListDaq),
  settings_(settings),
  runner_thread_(thread),
  list_data_(nullptr),
  interruptor_(false),
  list_data_model_(),
  my_run_(false),
  list_selection_model_(&list_data_model_)
{
  ui->setupUi(this);

  loadSettings();

  connect(&runner_thread_, SIGNAL(listComplete(Qpx::ListData*)), this, SLOT(list_completed(Qpx::ListData*)));

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
}

void FormListDaq::loadSettings() {
  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpxdata").toString();
  settings_.endGroup();

  settings_.beginGroup("Lab");
  ui->boxListMins->setValue(settings_.value("list_mins", 5).toInt());
  ui->boxListSecs->setValue(settings_.value("list_secs", 0).toInt());
  settings_.endGroup();
}

void FormListDaq::saveSettings() {
  settings_.beginGroup("Lab");
  settings_.setValue("list_mins", ui->boxListMins->value());
  settings_.setValue("list_secs", ui->boxListSecs->value());
  settings_.endGroup();
}

void FormListDaq::toggle_push(bool enable, Qpx::LiveStatus live) {
  bool online = (live == Qpx::LiveStatus::online);
  ui->pushListStart->setEnabled(enable && online);
  ui->boxListMins->setEnabled(enable && online);
  ui->boxListSecs->setEnabled(enable && online);
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
  } else {
    runner_thread_.terminate();
    runner_thread_.wait();
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
  QVector<QColor> palette {Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow};

  bool drawit[4];
  drawit[0] = ui->boxChan0->isChecked();
  drawit[1] = ui->boxChan1->isChecked();
  drawit[2] = ui->boxChan2->isChecked();
  drawit[3] = ui->boxChan3->isChecked();

  int actual_graphs = 0;
  for (int i=0; i < list_data_->run.detectors.size(); i++) {
    uint32_t trace_length = list_data_->hits[chosen_trace].trace.size();
    if ((drawit[i]) && (trace_length > 0)) {
      QVector<double> x(trace_length), y(trace_length);
      Gamma::Detector this_det = list_data_->run.detectors[i];
      int highest_res = 0;
      for (auto &q : this_det.energy_calibrations_.my_data_)
        if (q.bits_ > highest_res)
          highest_res = q.bits_;
      Gamma::Calibration this_calibration = this_det.energy_calibrations_.get(highest_res);
      int shift_by = 0;
      if (this_calibration.bits_)
        shift_by = 16 - this_calibration.bits_;
      for (std::size_t j=0; j<trace_length; j++) {
        x[j] = j;
        y[j] = this_calibration.transform(list_data_->hits[chosen_trace].trace[j] >> shift_by);
      }
      ui->tracePlot->addGraph();
      ui->tracePlot->graph(actual_graphs)->addData(x, y);
      ui->tracePlot->graph(actual_graphs)->setPen(QPen(palette[i]));
      actual_graphs++;
    }
  }
  ui->tracePlot->xAxis->setLabel("time (ticks)"); //can do better....
  ui->tracePlot->yAxis->setLabel("keV");
  ui->tracePlot->rescaleAxes();
  ui->tracePlot->replot();
}

void FormListDaq::on_pushListStart_clicked()
{
  emit statusText("List mode acquisition in progress...");

  emit toggleIO(false);
  ui->pushListStop->setEnabled(true);
  my_run_ = true;

  runner_thread_.do_list(interruptor_, ui->boxListMins->value() * 60 + ui->boxListSecs->value());
}

void FormListDaq::on_pushListStop_clicked()
{
  ui->pushListStop->setEnabled(false);
  PL_INFO << "List acquisition interrupted by user";
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
    emit toggleIO(true);
    my_run_ = false;
  }
}

void FormListDaq::list_selection_changed(QItemSelection, QItemSelection) {
  displayTraces();
}

void FormListDaq::on_boxChan0_clicked()
{
  displayTraces();
}

void FormListDaq::on_boxChan1_clicked()
{
  displayTraces();
}

void FormListDaq::on_boxChan2_clicked()
{
  displayTraces();
}

void FormListDaq::on_boxChan3_clicked()
{
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
  return 6;
}

QVariant TableListData::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();
  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(to_simple_string(mystuff->hits[index.row()].to_posix_time() * time_factor_));
    //case 1:
      //return QVariant::fromValue(QpxPattern(mystuff->hits[index.row()].pattern, false));
    default:
      int chan = col - 2;
      if (chan < 4) {
        int shiftby = 0;
        if (calibrations_[chan].bits_)
          shiftby = 16 - calibrations_[chan].bits_;
        double energy = calibrations_[chan].transform(mystuff->hits[row].energy >> shiftby);
        if (energy < 0)
          return "N";  //negative energy looks ugly :(
        else
          return QString::number(energy) + QString::fromStdString(calibrations_[chan].units_);
      }
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
        return QString("Time (s)");
      case 1:
        return QString("Hit pattern");
      case 2:
        return QString("Energy[0]");
      case 3:
        return QString("Energy[1]");
      case 4:
        return QString("Energy[2]");
      case 5:
        return QString("Energy[3]");
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
    time_factor_ = stuff->run.time_scale_factor();
    for (int i =0; i < mystuff->run.detectors.size(); i++) {
      Gamma::Detector thisdet = mystuff->run.detectors[i];
      int hi_res = 0;
      for (auto &q : thisdet.energy_calibrations_.my_data_) {
        if (q.bits_ > hi_res)
          hi_res = q.bits_;
      }
      calibrations_[i] = thisdet.energy_calibrations_.get(Gamma::Calibration("Energy", hi_res));
    }
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

void FormListDaq::on_pushListFileSort_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load list output", data_directory_, "qpx list output manifest (*.txt)");
  if (!validateFile(this, fileName, false))
    return;

  PL_INFO << "Reading list output metadata from " << fileName.toStdString();
  Qpx::Sorter sorter(fileName.toStdString());
  if (sorter.valid()) {
    QString fileName2 = CustomSaveFileDialog(this, "Save list output",
                                             data_directory_, "qpx list output (*.bin)");
    if (validateFile(this, fileName2, true)) {
      QFileInfo file(fileName2);
      if (file.suffix() != "bin")
        fileName2 += ".bin";
      PL_INFO << "Writing sorted list data file " << fileName2.toStdString();
      sorter.order(fileName2.toStdString());
    }
  }
}
