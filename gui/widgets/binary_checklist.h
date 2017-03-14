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
 *      QpxSpecialDelegate - displays colors, patterns, allows chosing of
 *      detectors.
 *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "generic_setting.h"

class BinaryChecklist : public QDialog {
  Q_OBJECT

public:
  explicit BinaryChecklist(Qpx::Setting setting, QWidget *parent = 0);
  Qpx::Setting get_setting() {return setting_;}

private slots:
  void change_setting();

private:
//  Ui::FormDaqSettings *ui;

  Qpx::Setting      setting_;
  std::vector<QCheckBox*> boxes_;
  std::vector<QDoubleSpinBox*>  spins_;
  std::vector<QComboBox*> menus_;

};
