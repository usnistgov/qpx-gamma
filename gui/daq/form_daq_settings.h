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
 *      Module and channel settings UI
 *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QCloseEvent>
#include "project.h"
#include "special_delegate.h"
#include "tree_settings.h"


namespace Ui {
class FormDaqSettings;
}

class FormDaqSettings : public QDialog
{
  Q_OBJECT

public:
  explicit FormDaqSettings(Qpx::ProjectPtr project, QWidget *parent = 0);
  ~FormDaqSettings();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void selectionChanged(int);

private:
  Ui::FormDaqSettings *ui;

  std::vector<Qpx::Spill> spills_;

//  Qpx::ProjectPtr     project_;

//  Qpx::Setting        dev_settings_;
  TreeSettings        tree_settings_model_;
  QpxSpecialDelegate  tree_delegate_;

};
