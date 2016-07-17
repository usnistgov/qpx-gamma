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

#include "rollback_dialog.h"
#include <QBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include "custom_logger.h"

RollbackDialog::RollbackDialog(Qpx::ROI roi, QWidget *parent) :
  QDialog(parent),
  roi_(roi)
{
  QLabel *label;
  QFrame* line;

  QVBoxLayout *vl_descr    = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Fit description");
  vl_descr->addWidget(label);

  QVBoxLayout *vl_fit    = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("# of peaks");
  vl_fit->addWidget(label);

  QVBoxLayout *vl_rsq = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("r-squared");
  vl_rsq->addWidget(label);

  QVBoxLayout *vl_s4a = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("sum4 aggregate");
  vl_s4a->addWidget(label);

  std::vector<Qpx::FitDescription> history = roi_.history();
  for (size_t i=0; i < history.size(); ++i) {

    QRadioButton *radio = new QRadioButton();
    radio->setLayoutDirection(Qt::LeftToRight);
    radio->setText(QString::fromStdString(history[i].description));
    radio->setFixedHeight(25);
    radio->setChecked(i == roi_.current_fit());
    radios_.push_back(radio);
    vl_descr->addWidget(radio);

    label = new QLabel();
    label->setFixedHeight(25);
    label->setText(QString::number(history[i].peaknum));
    vl_fit->addWidget(label);

    label = new QLabel();
    label->setFixedHeight(25);
    label->setText(QString::number(history[i].rsq));
    vl_rsq->addWidget(label);

    label = new QLabel();
    label->setFixedHeight(25);
    label->setText(QString::number(history[i].sum4aggregate));
    vl_s4a->addWidget(label);

  }

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(vl_descr);
  hl->addLayout(vl_fit);
  hl->addLayout(vl_rsq);
  hl->addLayout(vl_s4a);

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
  for (size_t i=0; i < radios_.size(); ++i)
    if (radios_[i]->isChecked())
      ret = i;
  return ret;
}
