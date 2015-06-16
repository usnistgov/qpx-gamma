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

FormOptimization::FormOptimization(ThreadRunner& thread, QSettings& settings, XMLableDB<Pixie::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormOptimization),
  opt_runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  interruptor_(false),
  opt_plot_thread_(current_spectra_),
  my_run_(false),
  pixie_(Pixie::Wrapper::getInstance())
{
  ui->setupUi(this);

  //loadSettings();
  connect(&opt_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));

  if (Pixie::Spectrum::Template *temp = Pixie::Spectrum::Factory::getInstance().create_template("1D")) {
    optimizing_  = *temp;
    delete temp;
  }

  optimizing_.name_ = "Optimizing";
  optimizing_.visible = true;

  moving_.themes["light"] = QPen(Qt::darkBlue, 2);
  moving_.themes["dark"] = QPen(Qt::blue, 2);

  a.themes["light"] = QPen(Qt::darkBlue, 2);
  a.themes["dark"] = QPen(Qt::blue, 2);

  QColor translu(Qt::blue);
  translu.setAlpha(32);
  b.themes["light"] = QPen(translu, 2);
  translu.setAlpha(64);
  b.themes["dark"] = QPen(translu, 2);

  ui->plot2->set_scale_type("Linear");
  ui->plot2->setTitle("Peak analysis");
  ui->plot3->set_scale_type("Linear");
  ui->plot->setLabels("channel", "count");
  ui->plot2->setLabels("channel", "count");

  ui->comboSetting->addItem("ENERGY_RISETIME");
  ui->comboSetting->addItem("ENERGY_FLATTOP");
  ui->comboSetting->addItem("TRIGGER_RISETIME");
  ui->comboSetting->addItem("TRIGGER_FLATTOP");
  ui->comboSetting->addItem("TRIGGER_THRESHOLD");
  ui->comboSetting->addItem("TAU");
  ui->comboSetting->addItem("BLCUT");
  ui->comboSetting->addItem("BASELINE_PERCENT");
  ui->comboSetting->addItem("CFD_THRESHOLD");

  ui->tableResults->setColumnCount(2);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem("Setting value", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(resultChosen()));

  connect(&opt_plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));

  connect(ui->plot, SIGNAL(clickedLeft(double)), this, SLOT(addMovingMarker(double)));
  connect(ui->plot, SIGNAL(clickedRight(double)), this, SLOT(removeMovingMarker(double)));
  connect(this, SIGNAL(restart_run()), this, SLOT(do_run()));
  connect(this, SIGNAL(post_proc()), this, SLOT(do_post_processing()));

  loadSettings();

  opt_plot_thread_.start();
}


void FormOptimization::loadSettings() {
  settings_.beginGroup("Optimization");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
  ui->spinOptChan->setValue(settings_.value("channel", 0).toInt());
  ui->spinTimeMins->setValue(settings_.value("sample_minutes", 5).toInt());
  ui->spinTimeSecs->setValue(settings_.value("sample_seconds", 0).toInt());
  ui->comboSetting->setCurrentText(settings_.value("setting_name", QString("ENERGY_RISETIME")).toString());
  ui->doubleSpinStart->setValue(settings_.value("setting_start", 5.97).toDouble());
  ui->doubleSpinDelta->setValue(settings_.value("setting_step", 0.213333).toDouble());
  ui->doubleSpinEnd->setValue(settings_.value("setting_end", 12.37).toDouble());
  settings_.endGroup();
}

void FormOptimization::saveSettings() {
  settings_.beginGroup("Optimization");
  settings_.setValue("bits", ui->spinBits->value());
  settings_.setValue("channel", ui->spinOptChan->value());
  settings_.setValue("sample_minutes", ui->spinTimeMins->value());
  settings_.setValue("sample_seconds", ui->spinTimeSecs->value());
  settings_.setValue("setting_name", ui->comboSetting->currentText());
  settings_.setValue("setting_start", ui->doubleSpinStart->value());
  settings_.setValue("setting_step", ui->doubleSpinDelta->value());
  settings_.setValue("setting_end", ui->doubleSpinEnd->value());
  settings_.endGroup();
}

FormOptimization::~FormOptimization()
{
  delete ui;
}

void FormOptimization::closeEvent(QCloseEvent *event) {
  if (my_run_ && opt_runner_thread_.isRunning()) {
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

  if (!spectra_y_.empty()) {
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

void FormOptimization::toggle_push(bool enable, Pixie::LiveStatus live) {
  bool online = (live == Pixie::LiveStatus::online);
  ui->pushStart->setEnabled(enable && online);
  ui->comboSetting->setEnabled(enable);
  ui->doubleSpinStart->setEnabled(enable);
  ui->doubleSpinDelta->setEnabled(enable);
  ui->doubleSpinEnd->setEnabled(enable);
  ui->spinTimeSecs->setEnabled(enable);
  ui->spinTimeMins->setEnabled(enable);
  ui->spinOptChan->setEnabled(enable);
  ui->spinBits->setEnabled(enable);
}

void FormOptimization::on_pushStart_clicked()
{
  spectra_y_.clear();
  spectra_app_.clear();
  peaks_.clear();
  setting_values_.clear();
  setting_fwhm_.clear();
  val_min = ui->doubleSpinStart->value();
  val_max = ui->doubleSpinEnd->value();
  val_d = ui->doubleSpinDelta->value();
  val_current = val_min;

  current_setting_ = ui->comboSetting->currentText().toStdString();

  ui->plot->clearGraphs();
  ui->plot->reset_scales();
  ui->plot2->clearGraphs();
  ui->plot2->reset_scales();
  ui->plot3->clearGraphs();
  ui->plot3->reset_scales();

  ui->tableResults->clear();
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));

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
  optimizing_.add_pattern.resize(Pixie::kNumChans,0);
  optimizing_.add_pattern[ui->spinOptChan->value()] = 1;
  optimizing_.match_pattern.resize(Pixie::kNumChans,0);
  optimizing_.match_pattern[ui->spinOptChan->value()] = 1;
  optimizing_.appearance = generateColor().rgba();

  XMLableDB<Pixie::Spectrum::Template> db("SpectrumTemplates");
  db.add(optimizing_);

  current_spectra_.clear();
  current_spectra_.set_spectra(db);
  peaks_.push_back(Peak());
  spectra_y_.push_back(std::vector<double>());
  spectra_app_.push_back(0);

  ui->plot->clearGraphs();
  ui->plot2->clearGraphs();

  x.resize(pow(2,bits), 0.0);
  y_opt.resize(pow(2,bits), 0.0);

  //pixie_.settings().set_chan(current_setting_, val_current, Pixie::Channel(ui->spinOptChan->value()), Pixie::Module::current);
  QThread::sleep(1);
  pixie_.settings().get_all_settings();
  double got = 1;//pixie_.settings().get_chan(current_setting_, Pixie::Channel(ui->spinOptChan->value()));
  setting_values_.push_back(got);
  setting_fwhm_.push_back(0);
  emit settings_changed();

  interruptor_.store(false);
  opt_runner_thread_.do_run(current_spectra_, interruptor_, Pixie::RunType::compressed, ui->spinTimeMins->value() * 60 + ui->spinTimeSecs->value(), true);
}

void FormOptimization::run_completed() {
  if (my_run_) {

    y_opt.clear();

    if ((val_current < val_max)  && !opt_runner_thread_.terminating()) {
      PL_INFO << "[Optimization] Completed test " << val_current << " < " << val_max;
      val_current += val_d;
      emit restart_run();
    } else
      ui->pushStop->setEnabled(false);
      emit toggleIO(true);
      my_run_ = false;
      emit post_proc();
  }
}

void FormOptimization::do_post_processing() {


}

bool FormOptimization::find_peaks() {

  int xmin = moving_.channel - ui->spinPeakWindow->value() / 2;
  int xmax = moving_.channel + ui->spinPeakWindow->value() / 2;

  if (xmin < 0) xmin = 0;
  if (xmax >= x.size()) xmax = x.size() - 1;

  if (moving_.visible) {

    UtilXY finder_opt(x, y_opt, xmin, xmax, 25);
    finder_opt.find_peaks(5);
    if (finder_opt.peaks_.size())
      peaks_[peaks_.size() - 1] = finder_opt.peaks_[0];

    replot_markers();

    return true;
  } else
    return false;
}

void FormOptimization::update_plots() {
  std::map<double, double> minima, maxima;

  for (auto &q: current_spectra_.by_type("1D")) {
    if (q->total_count()) {
      int current_spec = spectra_y_.size() - 1;

      uint32_t res = pow(2, bits);
      spectra_app_[current_spec] = q->appearance();
      std::shared_ptr<Pixie::Spectrum::EntryList> spectrum_data =
          std::move(q->get_spectrum({{0, res}}));

      y_opt.resize(res, 0);
      int xx = 0;
      for (auto it : *spectrum_data) {
        double yy = it.second;
        y_opt[it.first[0]] = yy;
        if (!minima.count(xx) || (minima[xx] > yy))
          minima[xx] = yy;
        if (!maxima.count(xx) || (maxima[xx] < yy))
          maxima[xx] = yy;
        ++xx;
      }

      find_peaks();
      spectra_y_[current_spec] = y_opt;
      setting_fwhm_[current_spec] = peaks_[current_spec].gaussian_.hwhm_ * 2;

      for (std::size_t i=0; i < res; i++)
        x[i] = i;

      ui->tableResults->setRowCount(peaks_.size());
      QTableWidgetItem *en = new QTableWidgetItem(QString::number(setting_values_[current_spec]));
      en->setFlags(en->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec,0, en);
      QTableWidgetItem *abu = new QTableWidgetItem(QString::number(setting_fwhm_[current_spec]));
      abu->setFlags(abu->flags() ^ Qt::ItemIsEditable);
      ui->tableResults->setItem(current_spec,1, abu);

    }
  }

  if (ui->plot->isVisible()) {
    this->setCursor(Qt::WaitCursor);

    ui->plot->clearGraphs();
    std::string new_label = boost::algorithm::trim_copy(current_spectra_.status());
    ui->plot->setTitle(QString::fromStdString(new_label));

    for (int i=0; i < spectra_y_.size(); ++i) {
      QColor this_color = QColor::fromRgba(spectra_app_[i]);
      if (i + 1 == peaks_.size()) {
        this_color.setAlpha(255);
//        ui->plot->addGraph(QVector<double>::fromStdVector(peaks_[ii].x_), QVector<double>::fromStdVector(peaks_[ii].y_fullfit_), this_color, 0);
      }

      ui->plot->addGraph(QVector<double>::fromStdVector(x),
                         QVector<double>::fromStdVector(spectra_y_[i]),
                         this_color, 1);

    }
    ui->plot->setYBounds(minima, maxima);
    ui->plot->replot_markers();
    ui->plot->redraw();

    this->setCursor(Qt::ArrowCursor);
  }

  resultChosen();
}

void FormOptimization::addMovingMarker(double x) {
  moving_.channel = x;
  moving_.visible = true;
  a.channel = x - ui->spinPeakWindow->value() / 2;
  a.visible = true;
  b.channel = x + ui->spinPeakWindow->value() / 2;
  b.visible = true;
  replot_markers();
}

void FormOptimization::removeMovingMarker(double x) {
  moving_.visible = false;
  a.visible = false;
  b.visible = false;
  replot_markers();
}


void FormOptimization::replot_markers() {
  ui->plot->set_block(a,b);
  ui->plot->replot_markers();
  ui->plot->redraw();
}


void FormOptimization::on_pushSaveOpti_clicked()
{
  pixie_.settings().save_optimization();
  std::vector<Pixie::Detector> dets = pixie_.settings().get_detectors();
  for (auto &q : dets) {
    if (detectors_.has_a(q)) {
      Pixie::Detector modified = detectors_.get(q);
      modified.setting_values_ = q.setting_values_;
      detectors_.replace(modified);
    }
  }
  emit optimization_approved();
}

void FormOptimization::resultChosen() {
  this->setCursor(Qt::WaitCursor);

  ui->plot2->clearGraphs();
  ui->plot3->clearGraphs();

  ui->plot3->addGraph(QVector<double>::fromStdVector(setting_values_), QVector<double>::fromStdVector(setting_fwhm_), Qt::darkMagenta, 0);

  for (auto &q : ui->tableResults->selectedRanges()) {
    if (q.rowCount() > 0)
      for (int j = q.topRow(); j <= q.bottomRow(); j++) {

        Marker cursor = moving_;
        cursor.visible = true;
        cursor.channel = setting_values_[j];
        ui->plot3->set_cursors(std::list<Marker>({cursor}));

        ui->plot2->addGraph(QVector<double>::fromStdVector(peaks_[j].x_), QVector<double>::fromStdVector(peaks_[j].y_), Qt::black, 2);
        ui->plot2->addGraph(QVector<double>::fromStdVector(peaks_[j].x_), QVector<double>::fromStdVector(peaks_[j].y_fullfit_), Qt::magenta, 0);
        ui->plot2->addGraph(QVector<double>::fromStdVector(peaks_[j].x_), QVector<double>::fromStdVector(peaks_[j].y_baseline_), Qt::red, 0);
      }
  }

  ui->plot2->rescale();
  ui->plot3->rescale();

  ui->plot2->redraw();
  ui->plot3->redraw();

  this->setCursor(Qt::ArrowCursor);
}
