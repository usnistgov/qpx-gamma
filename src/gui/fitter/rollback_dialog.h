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

#pragma once

#include <QDialog>
#include <QRadioButton>
#include "roi.h"

class RollbackDialog : public QDialog {
  Q_OBJECT

public:
  explicit RollbackDialog(Qpx::ROI setting, QWidget *parent = 0);
  int get_choice();

private:
  Qpx::ROI      roi_;
  std::vector<QRadioButton*> radios_;
};
