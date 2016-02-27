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
 *      QpxSpecialDelegate - displays colors, patterns, allows chosing of
 *      detectors.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "binary_checklist.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <bitset>


BinaryChecklist::BinaryChecklist(Gamma::Setting setting, QWidget *parent) :
  QDialog(parent),
  setting_(setting)
{
  int wordsize = setting.metadata.maximum;
  if (wordsize < 0)
    wordsize = 0;
  if (wordsize > 64)
    wordsize = 64;

  std::bitset<64> bs(setting.value_int);

  QLabel *label;
  QFrame* line;

  QVBoxLayout *vl_bit    = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Bit");
  vl_bit->addWidget(label);

  QVBoxLayout *vl_checks = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Value");
  vl_checks->addWidget(label);

  QVBoxLayout *vl_descr  = new QVBoxLayout();
  label = new QLabel();
  label->setFixedHeight(25);
  label->setText("Description");
  vl_descr->addWidget(label);

  bool showall = false;
  for (int i=0; i < wordsize; ++i) {
    bool exists = setting_.metadata.int_menu_items.count(i);
    bool visible = setting_.metadata.int_menu_items.count(i) || showall;

    bool is_branch = false;
    Gamma::Setting sub;
    if (exists)
      for (auto &q : setting_.branches.my_data_)
        if (q.id_ == setting_.metadata.int_menu_items[i]) {
          is_branch = true;
          sub = q;
        }

    bool is_int = (is_branch && (sub.metadata.setting_type == Gamma::SettingType::integer));
    bool is_menu = (is_branch && (sub.metadata.setting_type == Gamma::SettingType::int_menu));

    label = new QLabel();
    label->setFixedHeight(25);
    label->setText(QString::number(i));
    label->setVisible(visible);
    vl_bit->addWidget(label);

    QCheckBox *box = new QCheckBox();
    box->setLayoutDirection(Qt::RightToLeft);
    box->setFixedHeight(25);
    box->setChecked(bs[i] && !is_branch);
    box->setEnabled(setting.metadata.writable && setting_.metadata.int_menu_items.count(i) && !is_branch);
    box->setVisible(visible && !is_branch);
    boxes_.push_back(box);
    if (!is_branch)
      vl_checks->addWidget(box);

    QDoubleSpinBox *spin = new QDoubleSpinBox();
    spin->setFixedHeight(25);
    spin->setEnabled(setting.metadata.writable && setting_.metadata.int_menu_items.count(i) && is_int);
    spin->setVisible(visible && is_int);
    spins_.push_back(spin);
    if (is_int) {
      uint64_t num = setting.value_int;
      num = num << uint8_t(64 - sub.metadata.maximum - i);
      num = num >> uint8_t(64 - sub.metadata.maximum);
      uint64_t max = pow(2, uint16_t(sub.metadata.maximum)) - 1;

      spin->setMinimum(sub.metadata.minimum);    //sub.metadata.minimum
      spin->setSingleStep(sub.metadata.step);
      spin->setMaximum(sub.metadata.minimum + sub.metadata.step * max);
      spin->setValue(sub.metadata.minimum + sub.metadata.step * num);    //sub.metadata.minimum + step*
      //      PL_DBG << "converted num " << num << " to " << spin->value();
      if (sub.metadata.step == 1.0)
        spin->setDecimals(0);
      vl_checks->addWidget(spin);
    }

    QComboBox *menu = new QComboBox();
    menu->setFixedHeight(25);
    menu->setEnabled(setting.metadata.writable && setting_.metadata.int_menu_items.count(i) && is_menu);
    menu->setVisible(visible && is_menu);
    menus_.push_back(menu);
    if (is_menu) {
      int max = 0;
      for (auto &q : sub.metadata.int_menu_items)
        if (q.first > max)
          max = q.first;
      int bits = log2(max);
      if (pow(2, bits) < max)
        bits++;

      uint64_t num = setting.value_int;
      num = num << uint8_t(64 - bits - i);
      num = num >> uint8_t(64 - bits);

      for (auto &q : sub.metadata.int_menu_items)
        menu->addItem(QString::fromStdString(q.second), QVariant::fromValue(q.first));
      int menuIndex = menu->findText(QString::fromStdString(sub.metadata.int_menu_items[num]));
      if(menuIndex >= 0)
        menu->setCurrentIndex(menuIndex);
      vl_checks->addWidget(menu);
    }

    label = new QLabel();
    label->setFixedHeight(25);
    if (setting_.metadata.int_menu_items.count(i) && !is_branch)
      label->setText(QString::fromStdString(setting_.metadata.int_menu_items[i]));
    else if (is_branch)
      label->setText(QString::fromStdString(sub.metadata.name));
    else
      label->setText("N/A");
    label->setVisible(visible);
    vl_descr->addWidget(label);

    if (exists) {
      line = new QFrame();
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      line->setFixedHeight(3);
      line->setLineWidth(1);
      vl_bit->addWidget(line);


      line = new QFrame();
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      line->setFixedHeight(3);
      line->setLineWidth(1);
      vl_checks->addWidget(line);

      line = new QFrame();
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      line->setFixedHeight(3);
      line->setLineWidth(1);
      vl_descr->addWidget(line);
    }

  }

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(vl_bit);
  hl->addLayout(vl_checks);
  hl->addLayout(vl_descr);

  label = new QLabel();
  label->setText(QString::fromStdString("<b>" + setting_.id_ + ": " + setting_.metadata.name + "</b>"));

  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setFixedHeight(3);
  line->setLineWidth(1);

  QVBoxLayout *total    = new QVBoxLayout();
  total->addWidget(label);
  total->addWidget(line);
  total->addLayout(hl);

  if (setting.metadata.writable) {
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(change_setting()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    total->addWidget(buttonBox);
  } else {
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    total->addWidget(buttonBox);
  }

  setLayout(total);
}

void BinaryChecklist::change_setting() {
  std::bitset<64> bs;
  for (int i=0; i < boxes_.size(); ++i) {
    if (boxes_[i]->isVisible())
      bs[i] = boxes_[i]->isChecked();
  }
  setting_.value_int = bs.to_ullong();

  for (int i=0; i < spins_.size(); ++i) {
    if (spins_[i]->isVisible() && spins_[i]->isEnabled()) {
      double dbl = (spins_[i]->value() - spins_[i]->minimum()) / spins_[i]->singleStep();
      //      PL_DBG << "converted dbl " << spins_[i]->value() << " to " << dbl;
      int64_t num = static_cast<int64_t>(dbl);
      num = num << i;
      setting_.value_int |= num;
    }
  }

  for (int i=0; i < menus_.size(); ++i) {
    if (menus_[i]->isVisible() && menus_[i]->isEnabled()) {
      int64_t num = menus_[i]->currentData().toInt();
      num = num << i;
      setting_.value_int |= num;
    }
  }

  accept();
}



