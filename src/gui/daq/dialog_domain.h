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
 *      QpxSaveTypesWidget - widget for selecting file types for saving spectra
 *      DialogDomain  - dialog for chosing location and types
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <unordered_map>
#include <QDialog>
#include "domain.h"


namespace Ui {
class DialogDomain;
}

class DialogDomain : public QDialog
{
  Q_OBJECT

public:
  explicit DialogDomain(Qpx::Domain &domain,
                        const std::map<std::string, Qpx::Domain> &ext_domains,
                        QWidget *parent = 0);
  ~DialogDomain();

private slots:
  void on_buttonBox_accepted();
  void on_buttonBox_rejected();

  void on_comboSetting_activated(const QString &arg1);
  void on_comboUntil_activated(const QString &arg1);

  void on_doubleSpinStart_editingFinished();
  void on_doubleSpinEnd_editingFinished();

  void on_pushAddCustom_clicked();
  void on_pushEditCustom_clicked();
  void on_pushDeleteCustom_clicked();

  void durationChanged();

private:
  Ui::DialogDomain *ui;

  Qpx::Domain domain_;
  Qpx::Domain &external_domain_;

  std::map<std::string, Qpx::Setting> manual_settings_;
  std::map<std::string, Qpx::Domain>  external_domains_;

  std::map<std::string, Qpx::Domain> all_domains_;

  void loadSettings();
  void saveSettings();
  void remake_domains();
  double displayETA();
};
