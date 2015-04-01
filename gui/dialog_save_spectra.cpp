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
 * Description:
 *      WidgetSaveTypes - widget for selecting file types for saving spectra
 *      DialogSaveSpectra  - dialog for chosing location and types
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/


#include "spectrum.h"
#include <algorithm>
#include <QPaintEvent>
#include <QPainter>
#include "dialog_save_spectra.h"
#include "ui_dialog_save_spectra.h"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

WidgetSaveTypes::WidgetSaveTypes(QWidget *parent)
  : QWidget(parent),
    max_formats_(0)
{
  setMouseTracking(true);
  setAutoFillBackground(true);
  w_ = 50;
  h_ = 25;
}

void WidgetSaveTypes::initialize(std::vector<std::string> types) {
  if (types.empty())
    return;

  spectrum_types = types;
  file_formats.resize(spectrum_types.size());
  selections.resize(spectrum_types.size());
  for (std::size_t i = 0; i < spectrum_types.size(); i++) {
    Pixie::Spectrum::Template* type_template = Pixie::Spectrum::Factory::getInstance().create_template(spectrum_types[i]);
    file_formats[i] = std::vector<std::string>(type_template->output_types.begin(), type_template->output_types.end());
    selections[i].resize(file_formats[i].size(), false);
    max_formats_ = std::max(max_formats_, static_cast<int>(file_formats[i].size()));
    delete type_template;
  }
}

QSize WidgetSaveTypes::sizeHint() const
{
  return QSize(max_formats_*w_, spectrum_types.size()*h_);
}

void WidgetSaveTypes::paintEvent(QPaintEvent *evt)
{
  QPainter painter(this);
  QRectF one_rect = QRectF(0, 0, w_, h_);

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(QFont("Times", 10, QFont::Normal));

  for (std::size_t i = 0; i < file_formats.size(); i++) {

    painter.setBrush(this->palette().background());
    painter.setPen(QPen(this->palette().foreground().color()));
    painter.resetTransform();
    painter.translate(evt->rect().x(), evt->rect().y() + i*h_);
    painter.drawRect(one_rect);
    painter.drawText(one_rect, Qt::AlignCenter, QString::fromStdString(spectrum_types[i]));

    painter.setPen(QPen(Qt::white));
    for(std::size_t j=0; j < file_formats[i].size(); j++){
      if (selections[i][j])
        painter.setBrush(Qt::blue);
      else
        painter.setBrush(Qt::black);

      painter.resetTransform();
      painter.translate(evt->rect().x() + (j+1)*w_, evt->rect().y() + i*h_);
      painter.drawRect(one_rect);
      painter.drawText(one_rect, Qt::AlignCenter, QString("*.") + QString::fromStdString(file_formats[i][j]));
    }
  }
}

void WidgetSaveTypes::mouseReleaseEvent(QMouseEvent *event)
{
  std::size_t phigh = event->y() / h_;
  std::size_t pwide = (event->x() / w_) - 1;
  if ((phigh >= spectrum_types.size()) || (pwide >= file_formats[phigh].size()))
    return;

  selections[phigh][pwide] = !selections[phigh][pwide];
  update();
  emit stateChanged();
}



////Dialog///////////////

DialogSaveSpectra::DialogSaveSpectra(Pixie::SpectraSet& newset, QString outdir, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSaveSpectra)
{
  my_set_ = &newset;

  ui->setupUi(this);
  ui->typesWidget->initialize(my_set_->types());

  root_dir_ = outdir;
  std::string timenow = boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());

  ui->lineName->setText(QString("Qpx") + QString::fromStdString(timenow));
  if (my_set_->runInfo().total_events == 0)
    ui->boxQpxFile->setChecked(false);
}

DialogSaveSpectra::~DialogSaveSpectra()
{
  delete ui;
}

void DialogSaveSpectra::on_lineName_textChanged(const QString &arg1)
{
  ui->boxQpxFile->setText(arg1 + ".qpx");
  namespace fs = boost::filesystem;
  fs::path dir(root_dir_.toStdString());
  dir.make_preferred();
  dir /= boost::algorithm::trim_copy(arg1.toStdString());

  total_dir_ = dir.string();
  QString total = QString::fromStdString(total_dir_);

  if (fs::is_directory(dir)) {
    total = "<font color='red'>" + total + " already exists </font>";
    ui->buttonBox->setEnabled(false);
  } else if (!fs::portable_directory_name(arg1.toStdString())) {
    total = "<font color='red'>" + total + " invalid name </font>";
    ui->buttonBox->setEnabled(false);
  } else
    ui->buttonBox->setEnabled(true);

  ui->labelDirectory->setText(total);
}

void DialogSaveSpectra::on_buttonBox_accepted()
{
  boost::filesystem::path dir(total_dir_);
  if (boost::filesystem::create_directory(dir))
    PL_INFO << "Created directory " << total_dir_;
  else {
    PL_ERR << "Error creating directory";
    emit accepted();
    return;
  }

  CustomTimer filetime(true);

  for (std::size_t i = 0; i < ui->typesWidget->spectrum_types.size(); i ++) {
    std::list<Pixie::Spectrum::Spectrum*> thistype = my_set_->by_type(ui->typesWidget->spectrum_types[i]);
    for (std::size_t j = 0; j < ui->typesWidget->file_formats[i].size(); j++) {
      if (ui->typesWidget->selections[i][j]) {
        PL_INFO << "Saving " << ui->typesWidget->spectrum_types[i] << " spectra as " << ui->typesWidget->file_formats[i][j];
        for (auto &q : thistype)
          q->write_file(dir.string(), ui->typesWidget->file_formats[i][j]);
      }
    }
  }

  if (ui->boxQpxFile->isChecked()) {
    dir /= ui->lineName->text().toStdString() + ".qpx";
    my_set_->write_xml(dir.string());
  }

  filetime.stop();

  PL_INFO << "File writing time " << filetime.s() << " sec";
  emit accepted();
}

void DialogSaveSpectra::on_buttonBox_rejected()
{
  emit accepted();
}
