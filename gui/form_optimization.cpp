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
#include "spectrum1D.h"
#include "custom_logger.h"
#include "fityk.h"
#include "qt_util.h"

FormOptimization::FormOptimization(ThreadRunner& thread, QSettings& settings, XMLableDB<Gamma::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  opt_runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  interruptor_(false),
  opt_plot_thread_(current_spectra_),
  my_run_(false)
{
  ui->setupUi(this);

  //loadSettings();
  connect(&opt_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  if (Qpx::Spectrum::Template *temp = Qpx::Spectrum::Factory::getInstance().create_template("1D")) {
    optimizing_  = *temp;
    delete temp;
  }

  optimizing_.name_ = "Optimizing";
  optimizing_.visible = true;

  moving_.appearance.themes["light"] = QPen(Qt::darkBlue, 2);
  moving_.appearance.themes["dark"] = QPen(Qt::blue, 2);

  a.appearance.themes["light"] = QPen(Qt::darkBlue, 2);
  a.appearance.themes["dark"] = QPen(Qt::blue, 2);

  QColor translu(Qt::blue);
  translu.setAlpha(32);
  b.appearance.themes["light"] = QPen(translu, 2);
  translu.setAlpha(64);
  b.appearance.themes["dark"] = QPen(translu, 2);

  ui->plotSpectrum->setFit(&fitter_opt_);
  //connect(ui->plotSpectrum, SIGNAL(peaks_changed(bool)), this, SLOT(update_peaks(bool)));

  ui->comboSetting->addItem("ENERGY_RISETIME");
  ui->comboSetting->addItem("ENERGY_FLATTOP");
  ui->comboSetting->addItem("TRIGGER_RISETIME");
  ui->comboSetting->addItem("TRIGGER_FLATTOP");
  ui->comboSetting->addItem("TRIGGER_THRESHOLD");
  ui->comboSetting->addItem("TAU");
  ui->comboSetting->addItem("BLCUT");
  ui->comboSetting->addItem("BASELINE_PERCENT");
  ui->comboSetting->addItem("CFD_THRESHOLD");

  ui->tableResults->setColumnCount(4);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem("Setting value", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("%error", QTableWidgetItem::Type));
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
  settings_.beginGroup("Optimization");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
//  ui->spinOptChan->setValue(settings_.value("channel", 0).toInt());
  ui->timeDuration->set_total_seconds(settings_.value("mca_secs", 0).toULongLong());
  ui->comboSetting->setCurrentText(settings_.value("setting_name", QString("ENERGY_RISETIME")).toString());
  ui->doubleSpinStart->setValue(settings_.value("setting_start", 5.97).toDouble());
  ui->doubleSpinDelta->setValue(settings_.value("setting_step", 0.213333).toDouble());
  ui->doubleSpinEnd->setValue(settings_.value("setting_end", 12.37).toDouble());

  ui->plotSpectrum->loadSettings(settings_);

  settings_.endGroup();
}

void FormOptimization::saveSettings() {
  settings_.beginGroup("Optimization");
  settings_.setValue("bits", ui->spinBits->value());
//  settings_.setValue("channel", ui->spinOptChan->value());
  settings_.setValue("mca_secs", QVariant::fromValue(ui->timeDuration->total_seconds()));
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
  std::vector<Gamma::Detector> chans = Qpx::Engine::getInstance().get_detectors();

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
  } else {
    opt_runner_thread_.terminate();
    opt_runner_thread_.wait();
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

void FormOptimization::toggle_push(bool enable, Qpx::DeviceStatus status) {
  bool online = (status & Qpx::DeviceStatus::can_run);
  bool offline = online || (status & Qpx::DeviceStatus::loaded);

  ui->pushStart->setEnabled(enable && online);
  ui->comboSetting->setEnabled(enable);
  ui->doubleSpinStart->setEnabled(enable);
  ui->doubleSpinDelta->setEnabled(enable);
  ui->doubleSpinEnd->setEnabled(enable);
  ui->timeDuration->setEnabled(enable && offline);
  ui->comboTarget->setEnabled(enable);
  ui->spinBits->setEnabled(enable);
}

void FormOptimization::on_pushStart_clicked()
{

  spectra_.clear();
  fitter_opt_ = Gamma::Fitter();
  peaks_.clear();
  setting_values_.clear();
  setting_fwhm_.clear();
  val_min = ui->doubleSpinStart->value();
  val_max = ui->doubleSpinEnd->value();
  val_d = ui->doubleSpinDelta->value();
  val_current = val_min;

  current_setting_ = ui->comboSetting->currentText().toStdString();

  ui->plot3->clearGraphs();
  ui->plot3->reset_scales();

  ui->tableResults->clear();
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Energy", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("%error", QTableWidgetItem::Type));

  ui->plot3->setLabels(QString::fromStdString(current_setting_), "FWHM");
  ui->plot3->setTitle(QString::fromStdString("FWHM vs. " + current_setting_));

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
  emit toggleIO(false);
  my_run_ = true;

  bits = ui->spinBits->value();

  optimizing_.bits = bits;
  int optchan = ui->comboTarget->currentData().toInt();
  optimizing_.add_pattern.resize(optchan + 1, 0);
  optimizing_.add_pattern[optchan] = 1;
  optimizing_.match_pattern.resize(optchan + 1, 0);
  optimizing_.match_pattern[optchan] = 1;
  optimizing_.appearance = generateColor().rgba();

  XMLableDB<Qpx::Spectrum::Template> db("SpectrumTemplates");
  db.add(optimizing_);

  current_spectra_.clear();
  current_spectra_.set_spectra(db);
  peaks_.push_back(Gamma::Peak());
  spectra_.push_back(Gamma::Fitter());

  ui->plotSpectrum->new_spectrum();

  Gamma::Setting set(current_setting_, optchan);

  set.value_dbl = val_current;
  Qpx::Engine::getInstance().set_setting(set, Gamma::Match::id | Gamma::Match::indices);
  QThread::sleep(1);
  Qpx::Engine::getInstance().get_all_settings();
  set = Qpx::Engine::getInstance().pull_settings().get_setting(set, Gamma::Match::id | Gamma::Match::indices);
  setting_values_.push_back(set.value_dbl);
  setting_fwhm_.push_back(0);
  emit settings_changed();

  interruptor_.store(false);
  uint64_t duration = ui->timeDuration->total_seconds();
  opt_runner_thread_.do_run(current_spectra_, interruptor_, duration);
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

    peaks_[peaks_.size() - 1] = Gamma::Peak();

    if (fitter_opt_.peaks_.size()) {
      for (auto &q : fitter_opt_.peaks_) {
        double center = q.first;
        if ((q.second.gaussian_.height_ > peaks_[peaks_.size() - 1].height)
            && ((!moving_.visible)
                || ((center > a.pos.bin(a.pos.bits())) && (center < b.pos.bin(b.pos.bits())))
                )
            )
          peaks_[peaks_.size() - 1] = q.second;
      }
    }

    return true;
}

void FormOptimization::update_plots() {
  std::map<double, double> minima, maxima;

  for (auto &q: current_spectra_.by_type("1D")) {
    Qpx::Spectrum::Metadata md;
    if (q)
      md = q->metadata();

    if (md.total_count > 0) {
      int current_spec = spectra_.size() - 1;

      fitter_opt_.setData(q);

      ui->plotSpectrum->update_spectrum();

      find_peaks();
      spectra_[current_spec] = fitter_opt_;
      setting_fwhm_[current_spec] = peaks_[current_spec].fwhm_sum4;

      ui->tableResults->setRowCount(peaks_.size());

      QTableWidgetItem *st = new QTableWidgetItem(QString::number(setting_values_[current_spec]));
      st->setFlags(st->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 0, st);

      QTableWidgetItem *en = new QTableWidgetItem(QString::number(peaks_[current_spec].energy));
      en->setFlags(en->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 1, en);

      QTableWidgetItem *fw = new QTableWidgetItem(QString::number(peaks_[current_spec].fwhm_sum4));
      fw->setFlags(fw->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 2, fw);

      QTableWidgetItem *err = new QTableWidgetItem(QString::number(peaks_[current_spec].sum4_.err));
      err->setFlags(err->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec, 3, err);

    }
  }

  resultChosen();
}

void FormOptimization::on_pushSaveOpti_clicked()
{
  Qpx::Engine::getInstance().save_optimization();
  std::vector<Gamma::Detector> dets = Qpx::Engine::getInstance().get_detectors();
  for (auto &q : dets) {
    if (detectors_.has_a(q)) {
      Gamma::Detector modified = detectors_.get(q);
      modified.settings_ = q.settings_;
      detectors_.replace(modified);
    }
  }
  emit optimization_approved();
}

void FormOptimization::resultChosen() {
  this->setCursor(Qt::WaitCursor);

  ui->plot3->clearGraphs();

  ui->plot3->addGraph(QVector<double>::fromStdVector(setting_values_), QVector<double>::fromStdVector(setting_fwhm_), AppearanceProfile());

  for (auto &q : ui->tableResults->selectedRanges()) {
    if (q.rowCount() > 0)
      for (int j = q.topRow(); j <= q.bottomRow(); j++) {

        Marker cursor = moving_;
        cursor.pos.set_bin(setting_values_[j], fitter_opt_.metadata_.bits, fitter_opt_.nrg_cali_);
        cursor.visible = true;
        ui->plot3->set_cursors(std::list<Marker>({cursor}));

      }
  }

  ui->plot3->rescale();

  ui->plot3->redraw();

  this->setCursor(Qt::ArrowCursor);
}

void FormOptimization::on_comboTarget_currentIndexChanged(int index)
{
  int optchan = ui->comboTarget->currentData().toInt();

  std::vector<Gamma::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  ui->comboSetting->clear();
  if (optchan < chans.size()) {
    for (auto &q : chans[optchan].settings_.branches.my_data_) {
      if (q.metadata.flags.count("optimize") > 0) {
        QString text = QString::fromStdString(q.id_);
        ui->comboSetting->addItem(text);
      }
    }
  }
}

void FormOptimization::on_comboSetting_activated(const QString &arg1)
{
  int optchan = ui->comboTarget->currentData().toInt();
  current_setting_ = ui->comboSetting->currentText().toStdString();

  Gamma::Setting set(current_setting_, optchan);

  set = Qpx::Engine::getInstance().pull_settings().get_setting(set, Gamma::Match::id | Gamma::Match::indices);

  ui->doubleSpinStart->setValue(set.metadata.minimum);
  ui->doubleSpinEnd->setValue(set.metadata.maximum);
  ui->doubleSpinDelta->setValue(set.metadata.step);

}
