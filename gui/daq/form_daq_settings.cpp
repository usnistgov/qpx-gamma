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
 *      Module and channel settings
 *
 ******************************************************************************/

#include "form_daq_settings.h"
#include "ui_form_daq_settings.h"

FormDaqSettings::FormDaqSettings(Qpx::Setting tree, QWidget *parent) :
  QDialog(parent),
  dev_settings_(tree),
  ui(new Ui::FormDaqSettings)
{
  ui->setupUi(this);

  tree_settings_model_.update(dev_settings_);
  ui->treeSettings->setModel(&tree_settings_model_);
  ui->treeSettings->setItemDelegate(&tree_delegate_);
  ui->treeSettings->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeSettings->setEditTriggers(QAbstractItemView::NoEditTriggers);

}

FormDaqSettings::~FormDaqSettings()
{
  delete ui;
}

void FormDaqSettings::closeEvent(QCloseEvent *event) {
  event->accept();
}
