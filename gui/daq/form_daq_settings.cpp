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
//#include "qpx_util.h"

FormDaqSettings::FormDaqSettings(Qpx::ProjectPtr project, QWidget *parent) :
  QDialog(parent),
//  project_(project),
  ui(new Ui::FormDaqSettings)
{
  ui->setupUi(this);

  if (project)
  {
    for (auto &s : project->spills())
    {
      spills_.push_back(s);
      ui->listSpills->addItem(QString::fromStdString(boost::posix_time::to_iso_extended_string(s.time)));
    }
  }

  connect(ui->listSpills, SIGNAL(currentRowChanged(int)), this, SLOT(selectionChanged(int)));

//  tree_settings_model_.update(dev_settings_);
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

void FormDaqSettings::selectionChanged(int row)
{
  if ((row >= 0) && (row < static_cast<int>(spills_.size())))
    tree_settings_model_.update(spills_.at(row).state);
  else
    tree_settings_model_.update(Qpx::Setting());
}

