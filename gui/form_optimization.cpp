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
 *      Optimization interface
 *
 ******************************************************************************/

#include "form_optimization.h"
#include "ui_form_optimization.h"
#include "custom_logger.h"
#include "fityk.h"
#include "qt_util.h"
#include "daq_sink_factory.h"
#include <QSettings>


FormOptimization::FormOptimization(ThreadRunner& thread, XMLableDB<Qpx::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  opt_runner_thread_(thread),
  detectors_(detectors),
  interruptor_(false),
  opt_plot_thread_(current_spectra_),
  my_run_(false)
{
  ui->setupUi(this);
  this->setWindowTitle("Optimization");

  //loadSettings();
  connect(&opt_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  optimizing_  = Qpx::SinkFactory::getInstance().create_prototype("1D");

  optimizing_.name = "Optimizing";
  Qpx::Setting vis = optimizing_.attributes.branches.get(Qpx::Setting("visible"));
  vis.value_int = true;
  optimizing_.attributes.branches.replace(vis);

  style_pts.default_pen = QPen(Qt::darkBlue, 7);
  style_pts.themes["selected"] = QPen(Qt::red, 7);

  ui->plotSpectrum->setFit(&fitter_opt_);
  //connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  ui->PlotCalib->setLabels("setting", "FWHM");

  ui->tableResults->setColumnCount(5);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem("Setting value", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("%error", QTableWidgetItem::Type));
  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(resultChosen()));

  connect(&opt_plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  loadSettings();

  update_settings();

  opt_plot_thread_.start();
}


void FormOptimization::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Optimization");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
//  ui->spinOptChan->setValue(settings_.value("channel", 0).toInt());
  ui->doubleError->setValue(settings_.value("error", 1.0).toDouble());

  ui->comboSetting->setCurrentText(settings_.value("setting_name", QString("Manual...")).toString());
  ui->doubleSpinStart->setValue(settings_.value("setting_start", 5.97).toDouble());
  ui->doubleSpinDelta->setValue(settings_.value("setting_step", 0.213333).toDouble());
  ui->doubleSpinEnd->setValue(settings_.value("setting_end", 12.37).toDouble());

  ui->plotSpectrum->loadSettings(settings_);

  settings_.endGroup();
}

void FormOptimization::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Optimization");
  settings_.setValue("bits", ui->spinBits->value());
//  settings_.setValue("channel", ui->spinOptChan->value());
  settings_.setValue("error", ui->doubleError->value());

  settings_.setValue("setting_name", ui->comboSetting->currentText());
  settings_.setValue("setting_start", ui->doubleSpinStart->value());
  settings_.setValue("setting_step", ui->doubleSpinDelta->value());
  settings_.setValue("setting_end", ui->doubleSpinEnd->value());

  ui->plotSpectrum->saveSettings(settings_);

  settings_.endGroup();
}

FormOptimization::~FormOptimization()
{
  delete ui;
}

void FormOptimization::update_settings() {
  ui->comboTarget->clear();

  //should come from other thread?
  std::vector<Qpx::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  for (int i=0; i < chans.size(); ++i) {
    QString text = "[" + QString::number(i) + "] "
        + QString::fromStdString(chans[i].name_);
    ui->comboTarget->addItem(text, QVariant::fromValue(i));
  }
}

void FormOptimization::closeEvent(QCloseEvent *event) {
  if (my_run_ && opt_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      val_current += val_max;
      QThread::sleep(2);

      opt_runner_thread_.terminate();
      opt_runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  if (!spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Optimization data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes)
      current_spectra_.clear();
    else {
      event->ignore();
      return;
    }
  }
  current_spectra_.terminate();
  opt_plot_thread_.wait();

  saveSettings();
  event->accept();
}

void FormOptimization::toggle_push(bool enable, Qpx::SourceStatus status) {
  bool online = (status & Qpx::SourceStatus::can_run);
  bool offline = online || (status & Qpx::SourceStatus::loaded);

  ui->pushStart->setEnabled(enable && online && !my_run_);
  ui->comboSetting->setEnabled(enable);
  ui->doubleSpinStart->setEnabled(enable);
  ui->doubleSpinDelta->setEnabled(enable);
  ui->doubleSpinEnd->setEnabled(enable);
  ui->comboTarget->setEnabled(enable);
  ui->spinBits->setEnabled(enable);
}

void FormOptimization::on_pushStart_clicked()
{

  spectra_.clear();
  fitter_opt_ = Qpx::Fitter();
  peaks_.clear();
  setting_values_.clear();
  setting_fwhm_.clear();
  val_min = ui->doubleSpinStart->value();
  val_max = ui->doubleSpinEnd->value();
  val_d = ui->doubleSpinDelta->value();
  val_current = val_min;

  current_setting_ = ui->comboSetting->currentText().toStdString();

  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();

  ui->tableResults->clear();
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("%error", QTableWidgetItem::Type));

  do_run();
}

void FormOptimization::on_pushStop_clicked()
{
  PL_INFO << "Acquisition interrupted by user";
  val_current = val_max + 1;
  interruptor_.store(true);
}

void FormOptimization::do_run()
{
  ui->pushStop->setEnabled(true);
  my_run_ = true;
  emit toggleIO(false);

  bits = ui->spinBits->value();

  optimizing_.bits = bits;
  int optchan = ui->comboTarget->currentData().toInt();

  std::vector<bool> match_pattern(optchan + 1, false),
                    add_pattern(optchan + 1, false);
  add_pattern[optchan] = true;
  match_pattern[optchan] = true;

  Qpx::Setting pattern;
  pattern = optimizing_.attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.set_gates(match_pattern);
  pattern.value_pattern.set_theshold(1);
  optimizing_.attributes.branches.replace(pattern);
  pattern = optimizing_.attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.set_gates(add_pattern);
  pattern.value_pattern.set_theshold(1);
  optimizing_.attributes.branches.replace(pattern);


//  optimizing_.add_pattern.resize(optchan + 1, 0);
//  optimizing_.add_pattern[optchan] = 1;
//  optimizing_.match_pattern.resize(optchan + 1, 0);
//  optimizing_.match_pattern[optchan] = 1;
//  optimizing_.appearance = generateColor().rgba();

  XMLableDB<Qpx::Metadata> db("SpectrumTemplates");
  db.add(optimizing_);

  current_spectra_.clear();
  current_spectra_.set_prototypes(db);
  peaks_.push_back(Qpx::Peak());
  spectra_.push_back(Qpx::Fitter());

  ui->plotSpectrum->update_spectrum();

  Qpx::Setting set(current_setting_);
  set.indices.clear();
  set.indices.insert(optchan);
  set.value_dbl = val_current;

  if (current_setting_ == "Manual...")
  {
    bool ok;
    double d = QInputDialog::getDouble(this, QString::fromStdString(current_setting_),
                                       "Value?=", val_current, -10000, 10000, 2, &ok);
    if (ok)
      set.value_dbl = d;
  }
  else
  {
    Qpx::Engine::getInstance().set_setting(set, Qpx::Match::id | Qpx::Match::indices);
    QThread::sleep(1);
    Qpx::Engine::getInstance().get_all_settings();
    set = Qpx::Engine::getInstance().pull_settings().get_setting(set, Qpx::Match::id | Qpx::Match::indices);
  }

  setting_values_.push_back(set.value_dbl);
  setting_fwhm_.push_back(0);
  emit settings_changed();

  interruptor_.store(false);
  opt_runner_thread_.do_run(current_spectra_, interruptor_, 0);
}

void FormOptimization::run_completed() {
  if (my_run_) {
    PL_INFO << "<Optimization> Run completed";

    if (val_current < val_max) {
      PL_INFO << "<Optimization> Completed test " << val_current << " < " << val_max;
      val_current += val_d;
      do_run();
    } else {
      PL_INFO << "<Optimization> Done optimizing";
      my_run_ = false;
      ui->pushStop->setEnabled(false);
      emit toggleIO(true);
    }
  }
}

void FormOptimization::do_post_processing() {
  PL_INFO << "<Optimization> Pos-proc";

}

bool FormOptimization::find_peaks() {

    ui->plotSpectrum->perform_fit();

    peaks_[peaks_.size() - 1] = Qpx::Peak();

    for (auto &q : fitter_opt_.peaks())
      if (q.second.area_best.val > peaks_[peaks_.size() - 1].area_best.val)
          peaks_[peaks_.size() - 1] = q.second;

    return fitter_opt_.peaks().size();
}

void FormOptimization::update_plots() {

  bool have_data = false;
  bool have_peaks = false;

  for (auto &q: current_spectra_.get_sinks()) {
    Qpx::Metadata md;
    if (q.second)
      md = q.second->metadata();

    if (md.total_count > 0) {
      have_data = true;
      fitter_opt_.setData(q.second);
    }
  }

  if (have_data) {
    have_peaks = find_peaks();
    int current_spec = spectra_.size() - 1;
    spectra_[current_spec] = fitter_opt_;
    setting_fwhm_[current_spec] = peaks_[current_spec].fwhm_hyp;

    ui->tableResults->setRowCount(peaks_.size());

    QTableWidgetItem *st = new QTableWidgetItem(QString::number(setting_values_[current_spec]));
      st->setFlags(st->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 0, st);

      QTableWidgetItem *en = new QTableWidgetItem(QString::fromStdString(peaks_[current_spec].energy.to_string()));
      en->setFlags(en->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 1, en);

      QTableWidgetItem *fw = new QTableWidgetItem(QString::number(peaks_[current_spec].fwhm_hyp));
      fw->setFlags(fw->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 2, fw);

      QTableWidgetItem *area = new QTableWidgetItem(QString::number(peaks_[current_spec].area_best.val));
      area->setFlags(area->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 3, area);

      QTableWidgetItem *err = new QTableWidgetItem(QString::number(peaks_[current_spec].sum4_.peak_area.err()));
      err->setFlags(err->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 4, err);
  }

  if (ui->plotSpectrum->isVisible())
    ui->plotSpectrum->update_spectrum();

  if (have_peaks && !peaks_.empty() && (peaks_.back().sum4_.peak_area.err() < ui->doubleError->value()))
    interruptor_.store(true);

  resultChosen();
}

void FormOptimization::resultChosen() {
  this->setCursor(Qt::WaitCursor);

  QVector<double> xx = QVector<double>::fromStdVector(setting_values_);
  QVector<double> yy = QVector<double>::fromStdVector(setting_fwhm_);
  QVector<double> yy_sigma(yy.size(), 0);

  ui->PlotCalib->clearGraphs();
  ui->PlotCalib->addPoints(xx, yy, yy_sigma, style_pts);

  std::set<double> chosen_peaks_chan;

  //if selected insert;

  ui->PlotCalib->set_selected_pts(chosen_peaks_chan);

  ui->PlotCalib->setLabels(QString::fromStdString(current_setting_), "FWHM");


  this->setCursor(Qt::ArrowCursor);
}

void FormOptimization::on_comboTarget_currentIndexChanged(int index)
{
  int optchan = ui->comboTarget->currentData().toInt();

  std::vector<Qpx::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  ui->comboSetting->clear();
  if (optchan < chans.size()) {
    for (auto &q : chans[optchan].settings_.branches.my_data_) {
      if (q.metadata.flags.count("optimize") > 0) {
        QString text = QString::fromStdString(q.id_);
        ui->comboSetting->addItem(text);
      }
    }
  }
  ui->comboSetting->addItem(QString("Manual..."));
}

void FormOptimization::on_comboSetting_activated(const QString &arg1)
{
  int optchan = ui->comboTarget->currentData().toInt();
  current_setting_ = ui->comboSetting->currentText().toStdString();

  if (current_setting_ == "Manual...")
    return;

  Qpx::Setting set(current_setting_);
  set.indices.clear();
  set.indices.insert(optchan);

  set = Qpx::Engine::getInstance().pull_settings().get_setting(set, Qpx::Match::id | Qpx::Match::indices);

  ui->doubleSpinStart->setValue(set.metadata.minimum);
  ui->doubleSpinEnd->setValue(set.metadata.maximum);
  ui->doubleSpinDelta->setValue(set.metadata.step);

}

void FormOptimization::on_pushAddCustom_clicked()
{

}
