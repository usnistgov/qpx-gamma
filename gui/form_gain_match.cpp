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
 *      Gain matching interface
 *
 ******************************************************************************/

#include "form_gain_match.h"
#include "ui_form_gain_match.h"
#include "spectrum1D.h"
#include "custom_logger.h"
#include "fityk.h"


FormGainMatch::FormGainMatch(ThreadRunner& thread, QSettings& settings, XMLableDB<Gamma::Detector>& detectors, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormGainMatch),
  gm_runner_thread_(thread),
  settings_(settings),
  detectors_(detectors),
  gm_spectra_(),
  gm_interruptor_(false),
  gm_plot_thread_(gm_spectra_),
  my_run_(false)
{
  ui->setupUi(this);
  this->setWindowTitle("Gain matching");

  loadSettings();

  if (Qpx::Spectrum::Template *temp = Qpx::Spectrum::Factory::getInstance().create_template("1D")) {
    reference_   = *temp;
    optimizing_  = *temp;
    delete temp;
  }

  current_pass = 0;

  reference_.name_ = "Reference";
  reference_.visible = true;
  QColor col;
  col.setHsv(QColor(Qt::blue).hsvHue(), 48, 160);
  reference_.appearance = col.rgba();

  optimizing_.name_ = "Optimizing";
  optimizing_.visible = true;
  col.setHsv(QColor(Qt::red).hsvHue(), 48, 160);
  optimizing_.appearance = col.rgba();

  ap_reference_.default_pen = QPen(Qt::blue, 0);
  ap_optimized_.default_pen = QPen(Qt::red, 0);

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 9);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 9);
  style_fit.default_pen = QPen(Qt::darkCyan, 2);

  ui->PlotCalib->setLabels("gain", "peak center bin");

  ui->tableResults->setColumnCount(5);
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem("Setting value", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Bin", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("%error", QTableWidgetItem::Type));
  ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
  ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  //connect(ui->tableResults, SIGNAL(itemSelectionChanged()), this, SLOT(resultChosen()));

  connect(&gm_plot_thread_, SIGNAL(plot_ready()), this, SLOT(update_plots()));
  connect(&gm_runner_thread_, SIGNAL(runComplete()), this, SLOT(run_completed()));
  connect(ui->plotOpt, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotRef, SIGNAL(peak_selection_changed(std::set<double>)),
          this, SLOT(update_peak_selection(std::set<double>)));
  connect(ui->plotOpt, SIGNAL(data_changed()), this, SLOT(update_fits()));
  connect(ui->plotRef, SIGNAL(data_changed()), this, SLOT(update_fits()));


  ui->plotOpt->setFit(&fitter_opt_);
  ui->plotRef->setFit(&fitter_ref_);

  update_settings();

  gm_plot_thread_.start();
}

FormGainMatch::~FormGainMatch()
{
  delete ui;
}

void FormGainMatch::update_settings() {
  ui->comboReference->clear();
  ui->comboTarget->clear();

  //should come from other thread?
  std::vector<Gamma::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  for (int i=0; i < chans.size(); ++i) {
    QString text = "[" + QString::number(i) + "] "
        + QString::fromStdString(chans[i].name_);
    ui->comboReference->addItem(text, QVariant::fromValue(i));
    ui->comboTarget->addItem(text, QVariant::fromValue(i));
  }
}

void FormGainMatch::loadSettings() {
  settings_.beginGroup("GainMatching");
  ui->spinBits->setValue(settings_.value("bits", 14).toInt());
//  ui->spinRefChan->setValue(settings_.value("reference_channel", 0).toInt());
//  ui->spinOptChan->setValue(settings_.value("optimized_channel", 1).toInt());
  ui->doubleError->setValue(settings_.value("error", 1.0).toDouble());
  ui->doubleSpinDeltaV->setValue(settings_.value("delta_V", 0.000300).toDouble());
  settings_.endGroup();
}

void FormGainMatch::saveSettings() {
  settings_.beginGroup("GainMatching");
  settings_.setValue("bits", ui->spinBits->value());
//  settings_.setValue("reference_channel", ui->spinRefChan->value());
//  settings_.setValue("optimized_channel", ui->spinOptChan->value());
  settings_.setValue("error", ui->doubleError->value());
  settings_.setValue("delta_V", ui->doubleSpinDeltaV->value());
  settings_.endGroup();
}

void FormGainMatch::closeEvent(QCloseEvent *event) {
  if (my_run_ && gm_runner_thread_.running()) {
    int reply = QMessageBox::warning(this, "Ongoing data acquisition",
                                     "Terminate?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      gm_runner_thread_.terminate();
      gm_runner_thread_.wait();
    } else {
      event->ignore();
      return;
    }
  }

  if (!gm_spectra_.empty()) {
    int reply = QMessageBox::warning(this, "Gain matching data still open",
                                     "Discard?",
                                     QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
      gm_spectra_.clear();
    } else {
      event->ignore();
      return;
    }
  }

  gm_spectra_.terminate();
  gm_plot_thread_.wait();
  saveSettings();

  event->accept();
}

void FormGainMatch::toggle_push(bool enable, Qpx::DeviceStatus status) {
  bool online = (status & Qpx::DeviceStatus::can_run);
  ui->pushMatchGain->setEnabled(enable && online && !my_run_);

  ui->comboReference->setEnabled(enable);
  ui->comboTarget->setEnabled(enable);
  ui->spinBits->setEnabled(enable && online);
}

void FormGainMatch::do_run()
{
  ui->pushStop->setEnabled(true);
  my_run_ = true;
  int optchan = ui->comboTarget->currentData().toInt();
  emit toggleIO(false);

  Gamma::Setting gain_setting(current_setting_);
  gain_setting.indices.clear();
  gain_setting.indices.insert(optchan);

  Qpx::Engine::getInstance().get_all_settings();
  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  double old_gain = gain_setting.value_dbl;


  bits = ui->spinBits->value();

  if (gm_spectra_.empty()) {
    reference_.bits = bits;
    int refchan = ui->comboReference->currentData().toInt();
    reference_.add_pattern.resize(refchan + 1, 0);
    reference_.add_pattern[refchan] = 1;
    reference_.match_pattern.resize(refchan + 1, 0);
    reference_.match_pattern[refchan] = 1;
    gm_spectra_.add_spectrum(reference_);
  } else {
    gm_spectra_.delete_spectrum("Optimizing");
  }

  Gamma::Setting set_gain("Gain");
  set_gain.value_dbl = old_gain;
  Gamma::Setting set_pass("Pass");
  set_pass.value_int = current_pass;
  optimizing_.generic_attributes.replace(set_gain);
  optimizing_.generic_attributes.replace(set_pass);
  optimizing_.bits = bits;
  optimizing_.add_pattern.resize(optchan + 1, 0);
  optimizing_.add_pattern[optchan] = 1;
  optimizing_.match_pattern.resize(optchan + 1, 0);
  optimizing_.match_pattern[optchan] = 1;
  gm_spectra_.add_spectrum(optimizing_);


  gm_plot_thread_.start();

  fitter_ref_.setData(gm_spectra_.by_name("Reference"));
  fitter_opt_.setData(gm_spectra_.by_name("Optimizing"));
  ui->plotRef->update_spectrum();
  ui->plotOpt->update_spectrum();


  gains.push_back(old_gain);
  peaks_.push_back(Gamma::Peak());
  ui->tableResults->setRowCount(peaks_.size());

  gm_runner_thread_.do_run(gm_spectra_, gm_interruptor_, 0);
}

void FormGainMatch::run_completed() {
  if (my_run_) {
    gm_spectra_.terminate();
    gm_plot_thread_.wait();
    peak_ref_.area_best = FitParam();
    peak_opt_.area_best = FitParam();
    //replot_markers();

    if (!gm_runner_thread_.terminating()) {
      PL_INFO << "<FormGainMatch> Completed pass # " << current_pass;
      current_pass++;
      do_post_processing();
    } else {
      ui->pushStop->setEnabled(false);
      my_run_ = false;
      emit toggleIO(true);
    }
  }
}

void FormGainMatch::do_post_processing() {

//  update_plots();

  double old_gain = gains.back();
  double delta = peak_opt_.center - peak_ref_.center;

  deltas.push_back(peak_opt_.center);

  double new_gain = old_gain;
  if (peak_opt_.center < peak_ref_.center)
    new_gain += ui->doubleSpinDeltaV->value();
  else if (peak_opt_.center > peak_ref_.center)
    new_gain -= ui->doubleSpinDeltaV->value();

  std::vector<double> sigmas(deltas.size(), 1);

  if (gains.size() > 1) {
    PolyBounded p;
    p.add_coeff(0, -50, 50, 1);
    p.add_coeff(1, -50, 50, 1);
    p.add_coeff(2, -50, 50, 1);
    p.fit(gains, deltas, sigmas);
    double predict = p.eval_inverse(peak_ref_.center/*, ui->doubleThreshold->value() / 4.0*/);
    if (!std::isnan(predict)) {
      new_gain = predict;
    }


    QVector<double> xx = QVector<double>::fromStdVector(gains);
    QVector<double> yy = QVector<double>::fromStdVector(deltas);
    QVector<double> yy_sigma(yy.size(), 0);

    xx.push_back(new_gain);
    yy.push_back(peak_ref_.center);

    double xmin = std::numeric_limits<double>::max();
    double xmax = - std::numeric_limits<double>::max();

    for (auto &q : xx) {
      if (q < xmin)
        xmin = q;
      if (q > xmax)
        xmax = q;
    }

    double x_margin = (xmax - xmin) / 10;
    xmax += x_margin;
    xmin -= x_margin;

    ui->PlotCalib->clear_data();
    ui->PlotCalib->addPoints(xx, yy, yy_sigma, style_pts);


    xx.clear();
    yy.clear();

    double step = (xmax-xmin) / 50.0;

    for (double i=xmin; i < xmax; i+=step) {
      xx.push_back(i);
      yy.push_back(p.eval(i));
    }

    ui->PlotCalib->addFit(xx, yy, style_fit);
    ui->PlotCalib->setFloatingText("E = " + QString::fromStdString(p.to_UTF8(3, true)));
    ui->PlotCalib->redraw();

  }

  if (std::abs(delta) < ui->doubleThreshold->value()) {
    PL_INFO << "<FormGainMatch> gain matching complete";
    ui->pushStop->setEnabled(false);
    my_run_ = false;
    emit toggleIO(true);
    return;
  }

  int optchan = ui->comboTarget->currentData().toInt();
  Gamma::Setting gain_setting(current_setting_);
  gain_setting.indices.clear();
  gain_setting.indices.insert(optchan);

  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  gain_setting.value_dbl = new_gain;

  Qpx::Engine::getInstance().set_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  QThread::sleep(2);
  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);
  new_gain = gain_setting.value_dbl;


  PL_INFO << "<FormGainMatch> gain changed from " << std::fixed << std::setprecision(6)
          << old_gain << " to " << new_gain;

  QThread::sleep(2);
  do_run();
}

void FormGainMatch::update_fits() {
  if (!ui->plotRef->busy()) {
    std::set<double> selref = ui->plotRef->get_selected_peaks();
    std::set<double> newselr;
    for (auto &p : fitter_ref_.peaks()) {
      for (auto &s : selref)
        if (abs(p.second.center - s) < 2) {
          newselr.insert(p.second.center);
//          PL_DBG << "match selr " << p.second.center;
        }
    }
//    PL_DBG << "refsel close enough " << newsel.size();
    ui->plotRef->set_selected_peaks(newselr);
  }

  if (!ui->plotOpt->busy()) {
    std::set<double> selopt = ui->plotOpt->get_selected_peaks();
    std::set<double> newselo;
    for (auto &p : fitter_opt_.peaks()) {
      for (auto &s : selopt)
        if (abs(p.second.center - s) < 2) {
          newselo.insert(p.second.center);
//          PL_DBG << "match selo " << p.second.center;
        }
    }
//    PL_DBG << "optsel close enough " << newsel.size();
    ui->plotOpt->set_selected_peaks(newselo);
  }

  update_peak_selection(std::set<double>());
}


void FormGainMatch::update_peak_selection(std::set<double> dummy) {
  peak_ref_ = Gamma::Peak();
  peak_opt_ = Gamma::Peak();

  std::set<double> selref = ui->plotRef->get_selected_peaks();
  std::set<double> selopt = ui->plotOpt->get_selected_peaks();

  if ((selref.size() != 1) || (selopt.size() != 1))
    return;

//  PL_DBG << "Have one peak each";

  if (fitter_ref_.peaks().count(*selref.begin()))
    peak_ref_ = fitter_ref_.peaks().at(*selref.begin());
  if (fitter_opt_.peaks().count(*selopt.begin()))
    peak_opt_ = fitter_opt_.peaks().at(*selopt.begin());

  if ((peak_ref_.area_best.val == 0) || (peak_opt_.area_best.val == 0))
    return;


  Gamma::Setting set_gain("Gain");
  Gamma::Setting set_pass("Pass");
  double gain = fitter_opt_.metadata_.attributes.get(set_gain).value_dbl;
  int    pass = fitter_opt_.metadata_.attributes.get(set_pass).value_int;

  peaks_[pass] = peak_opt_;

  if (peaks_.size()) {

    QTableWidgetItem *st = new QTableWidgetItem(QString::number(gains[pass]));
    st->setFlags(st->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(pass, 0, st);

    QTableWidgetItem *ctr = new QTableWidgetItem(QString::number(peaks_[pass].center));
    ctr->setFlags(ctr->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(pass, 1, ctr);

    QTableWidgetItem *fw = new QTableWidgetItem(QString::number(peaks_[pass].fwhm_sum4));
    fw->setFlags(fw->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(pass, 2, fw);

    QTableWidgetItem *area = new QTableWidgetItem(QString::number(peaks_[pass].area_best.val));
    area->setFlags(area->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(pass, 3, area);

    QTableWidgetItem *err = new QTableWidgetItem(QString::number(peaks_[pass].sum4_.peak_area.err()));
    err->setFlags(err->flags() ^ Qt::ItemIsEditable);
    ui->tableResults->setItem(pass, 4, err);
  }


  if ((pass == current_pass) && (peaks_.back().sum4_.peak_area.err() < ui->doubleError->value()))
    gm_interruptor_.store(true);

}


void FormGainMatch::update_plots() {
  bool have_data = false;

  for (auto &q: gm_spectra_.by_type("1D")) {
    Qpx::Spectrum::Metadata md;
    if (q)
      md = q->metadata();

    if (md.total_count > 0) {
      have_data = true;

      if (md.name == "Reference")
        fitter_ref_.setData(q);
      else
        fitter_opt_.setData(q);
    }
  }

  if (have_data) {
//    PL_DBG << "Have data!";
    if (!ui->plotRef->busy()) {
      ui->plotRef->updateData();
      ui->plotRef->perform_fit();
    }
    if (!ui->plotOpt->busy()) {
      ui->plotOpt->updateData();
      ui->plotOpt->perform_fit();
    }
  }

  this->setCursor(Qt::ArrowCursor);
}

void FormGainMatch::on_pushMatchGain_clicked()
{
  gains.clear();
  deltas.clear();
  peaks_.clear();

  current_setting_ = ui->comboSetting->currentText().toStdString();

  ui->tableResults->clear();
  ui->tableResults->setHorizontalHeaderItem(0, new QTableWidgetItem(QString::fromStdString(current_setting_), QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(1, new QTableWidgetItem("Bin", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(2, new QTableWidgetItem("FWHM", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(3, new QTableWidgetItem("area", QTableWidgetItem::Type));
  ui->tableResults->setHorizontalHeaderItem(4, new QTableWidgetItem("%error", QTableWidgetItem::Type));

  ui->PlotCalib->setFloatingText("");
  ui->PlotCalib->clearGraphs();

  current_pass = 0;
  gm_spectra_.clear();
  fitter_ref_.clear();
  fitter_opt_.clear();
  do_run();
}

void FormGainMatch::on_pushStop_clicked()
{
  gm_interruptor_.store(true);
}

void FormGainMatch::on_comboTarget_currentIndexChanged(int index)
{
  int optchan = ui->comboTarget->currentData().toInt();

  std::vector<Gamma::Detector> chans = Qpx::Engine::getInstance().get_detectors();

  ui->comboSetting->clear();
  if (optchan < chans.size()) {
    for (auto &q : chans[optchan].settings_.branches.my_data_) {
      if (q.metadata.flags.count("gain") > 0) {
        QString text = QString::fromStdString(q.id_);
        ui->comboSetting->addItem(text);
      }
    }
  }

}

void FormGainMatch::on_comboSetting_activated(int index)
{
  int optchan = ui->comboTarget->currentData().toInt();
  current_setting_ = ui->comboSetting->currentText().toStdString();

  Gamma::Setting gain_setting(current_setting_);
  gain_setting.indices.clear();
  gain_setting.indices.insert(optchan);


  gain_setting = Qpx::Engine::getInstance().pull_settings().get_setting(gain_setting, Gamma::Match::id | Gamma::Match::indices);

  ui->doubleSpinDeltaV->setValue(gain_setting.metadata.step * 10);
}
