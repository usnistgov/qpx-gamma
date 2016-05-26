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
 *      DialogDomain  - dialog for chosing location and types
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/


#include "dialog_domain.h"
#include "ui_dialog_domain.h"
#include "custom_logger.h"
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>

using namespace Qpx;


DialogDomain::DialogDomain(Qpx::Domain &domain,
                           const std::map<std::string, Qpx::Domain> &ext_domains,
                           QWidget *parent)
  : QDialog(parent)
  , external_domains_(ext_domains)
  , external_domain_(domain)
  , domain_(domain)
  , ui(new Ui::DialogDomain)
{
  ui->setupUi(this);

  connect(ui->timeDuration, SIGNAL(valueChanged()), this, SLOT(durationChanged()));

  ui->timeDuration->set_us_enabled(false);
  ui->timeDuration->setVisible(false);
  ui->doubleCriterion->setValue(false);

  loadSettings();
  remake_domains();

  ui->comboUntil->addItem("Real time >");
  ui->comboUntil->addItem("Live time >");
  ui->comboUntil->addItem("Total count >");

  if (all_domains_.count(domain.verbose))
    ui->comboSetting->setCurrentText(QString::fromStdString(domain.verbose));
  on_comboSetting_activated("");

  ui->doubleCriterion->setValue(domain.criterion);
  ui->timeDuration->set_total_seconds(domain.criterion);

  ui->comboUntil->setCurrentText("Real time >");
  if (domain.criterion_type == Qpx::DomainAdvanceCriterion::realtime)
    ui->comboUntil->setCurrentText("Real time >");
  else if (domain.criterion_type == Qpx::DomainAdvanceCriterion::livetime)
    ui->comboUntil->setCurrentText("Live time >");
  else if (domain.criterion_type == Qpx::DomainAdvanceCriterion::totalcount)
    ui->comboUntil->setCurrentText("Total count >");

  ui->doubleSpinStart->setValue(domain.value_range.metadata.minimum);
  on_doubleSpinStart_editingFinished();
  ui->doubleSpinEnd->setValue(domain.value_range.metadata.maximum);
  on_doubleSpinEnd_editingFinished();
  ui->doubleSpinDelta->setValue(domain.value_range.metadata.step);

  on_comboUntil_activated("");
}

DialogDomain::~DialogDomain()
{
  delete ui;
}


void DialogDomain::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    Qpx::Setting manset("manual_optimization_settings");
    manset.metadata.setting_type = Qpx::SettingType::stem;

    std::string path = profile_directory.toStdString() + "/manual_settings.set";
    pugi::xml_document doc;
    if (doc.load_file(path.c_str())) {
      pugi::xml_node root = doc.child("Optimizer");
      if (root.child(manset.xml_element_name().c_str()))
        manset.from_xml(root.child(manset.xml_element_name().c_str()));
    }

    manual_settings_.clear();
    for (auto &s : manset.branches.my_data_)
      manual_settings_["[manual] " + s.id_] = s;
  }

}

void DialogDomain::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();
  settings_.endGroup();

  if (!profile_directory.isEmpty()) {
    std::string path = profile_directory.toStdString() + "/manual_settings.set";
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    pugi::xml_node node = root.append_child("Optimizer");
    if (!manual_settings_.empty()) {
      Qpx::Setting manset("manual_optimization_settings");
      manset.metadata.setting_type = Qpx::SettingType::stem;
      for (auto &q : manual_settings_)
        manset.branches.add_a(q.second);
      manset.to_xml(node);
    }
    doc.save_file(path.c_str());
  }
}


void DialogDomain::on_buttonBox_accepted()
{
  double numpoints = (ui->doubleSpinEnd->value() - ui->doubleSpinStart->value()) / ui->doubleSpinDelta->value();
  if ((numpoints > 100) &&
      (QMessageBox::Yes != QMessageBox::question(this, "Big experiment", "This will acquire over 100 spectra. Are you sure?")))
    return;

  domain_.value_range.metadata.minimum = ui->doubleSpinStart->value();
  domain_.value_range.metadata.step    = ui->doubleSpinDelta->value();
  domain_.value_range.metadata.maximum = ui->doubleSpinEnd->value();
  domain_.criterion = ui->timeDuration->total_seconds();

  if (ui->comboUntil->currentText() == "Real time >")
    domain_.criterion_type = Qpx::DomainAdvanceCriterion::realtime;
  else if (ui->comboUntil->currentText() == "Live time >")
    domain_.criterion_type = Qpx::DomainAdvanceCriterion::livetime;
  if (ui->comboUntil->currentText() == "Total count >")
    domain_.criterion_type = Qpx::DomainAdvanceCriterion::totalcount;

  if ((displayETA() > (12 * 60 * 60)) &&
      (QMessageBox::Yes != QMessageBox::question(this, "Big experiment", "This will acquire over over 12 hourse. Are you sure?")))
    return;

  external_domain_ = domain_;

  accept();
}

void DialogDomain::on_buttonBox_rejected()
{
  reject();
}

void DialogDomain::on_comboSetting_activated(const QString &)
{
  domain_ = Qpx::Domain();
  if (all_domains_.count(ui->comboSetting->currentText().toStdString()))
    domain_ = all_domains_.at( ui->comboSetting->currentText().toStdString() );

  ui->doubleSpinStart->setRange(domain_.value_range.metadata.minimum, domain_.value_range.metadata.maximum);
  ui->doubleSpinStart->setSingleStep(domain_.value_range.metadata.step);
  ui->doubleSpinStart->setValue(domain_.value_range.metadata.minimum);
  ui->doubleSpinStart->setSuffix(" " + QString::fromStdString(domain_.value_range.metadata.unit));

  ui->doubleSpinEnd->setRange(domain_.value_range.metadata.minimum, domain_.value_range.metadata.maximum);
  ui->doubleSpinEnd->setSingleStep(domain_.value_range.metadata.step);
  ui->doubleSpinEnd->setValue(domain_.value_range.metadata.maximum);
  ui->doubleSpinEnd->setSuffix(" " + QString::fromStdString(domain_.value_range.metadata.unit));

  ui->doubleSpinDelta->setRange(domain_.value_range.metadata.step, domain_.value_range.metadata.maximum - domain_.value_range.metadata.minimum);
  ui->doubleSpinDelta->setSingleStep(domain_.value_range.metadata.step);
  ui->doubleSpinDelta->setValue(domain_.value_range.metadata.step);
  ui->doubleSpinDelta->setSuffix(" " + QString::fromStdString(domain_.value_range.metadata.unit));

  ui->pushEditCustom->setVisible(domain_.type == Qpx::DomainType::manual);
  ui->pushDeleteCustom->setVisible(domain_.type == Qpx::DomainType::manual);
}

void DialogDomain::on_comboUntil_activated(const QString &)
{
  QString arg1 = ui->comboUntil->currentText();
  if ((arg1 == "Real time >") || (arg1 == "Live time >"))
  {
    ui->doubleCriterion->setVisible(false);
    ui->timeDuration->setVisible(true);
  }
  else if (arg1 == "Total count >")
  {
    ui->doubleCriterion->setVisible(true);
    ui->timeDuration->setVisible(false);
    ui->doubleCriterion->setRange(1, 10000000);
    ui->doubleCriterion->setSingleStep(100);
    ui->doubleCriterion->setDecimals(0);
  }
  else
  {
    ui->doubleCriterion->setVisible(false);
    ui->timeDuration->setVisible(false);
  }
  displayETA();
}

void DialogDomain::durationChanged()
{
  displayETA();
}

double DialogDomain::displayETA()
{
  QString arg1 = ui->comboUntil->currentText();
  bool show = ((arg1 == "Real time >") || (arg1 == "Live time >"));
  ui->labelEstimate->setVisible(show);
  ui->labelETA->setVisible(show);

  if (!show)
    return 0;

  double numpoints = (ui->doubleSpinEnd->value() - ui->doubleSpinStart->value()) / ui->doubleSpinDelta->value();
  double duration = numpoints * ui->timeDuration->total_seconds();

  Qpx::Setting st;
  st.metadata.setting_type = Qpx::SettingType::time_duration;
  st.value_duration = boost::posix_time::seconds(duration);

  ui->labelETA->setText(QString::fromStdString(st.val_to_pretty_string()));
  return duration;
}

void DialogDomain::on_doubleSpinStart_editingFinished()
{
  ui->doubleSpinEnd->setMinimum(ui->doubleSpinStart->value());
  ui->doubleSpinDelta->setMaximum(domain_.value_range.metadata.maximum - ui->doubleSpinStart->value());
}

void DialogDomain::on_doubleSpinEnd_editingFinished()
{
  ui->doubleSpinStart->setMaximum(ui->doubleSpinEnd->value());
  ui->doubleSpinDelta->setMaximum(ui->doubleSpinEnd->value() - domain_.value_range.metadata.minimum);
}

void DialogDomain::remake_domains()
{
  all_domains_ = external_domains_;

  for (auto &s : manual_settings_)
    if (s.second.is_numeric())
    {
      Qpx::Domain domain;
      domain.verbose = s.first;
      domain.type = Qpx::DomainType::manual;
      domain.value_range = s.second;
      all_domains_[s.first] = domain;
    }

  ui->comboSetting->clear();
  for (auto &q : all_domains_)
    ui->comboSetting->addItem(QString::fromStdString(q.first));
}


void DialogDomain::on_pushAddCustom_clicked()
{
  bool ok;
  QString name = QInputDialog::getText(this, "New custom variable",
                                       "Name:", QLineEdit::Normal, "", &ok);
  if (ok) {
    for (auto &s : manual_settings_)
      if (s.second.id_ == name.toStdString())
      {
        QMessageBox::information(this, "Already exists",
                                 "Manual setting \'" + name + "\'' already exists");
        return;
      }

    Qpx::Setting newset(name.toStdString());
    newset.metadata.name = name.toStdString();
    newset.metadata.setting_type = Qpx::SettingType::floating;
    newset.metadata.flags.insert("manual");

    double d = 0;

    d = QInputDialog::getDouble(this, "Minimum",
                                "Minimum value for " + name + " = ",
                                0,
                                std::numeric_limits<double>::min(),
                                std::numeric_limits<double>::max(),
                                0.001,
                                &ok);
    if (ok)
      newset.metadata.minimum = d;
    else
      return;

    d = QInputDialog::getDouble(this, "Maximum",
                                "Maximum value for " + name + " = ",
                                std::max(100.0, newset.metadata.minimum),
                                newset.metadata.minimum,
                                std::numeric_limits<double>::max(),
                                0.001,
                                &ok);
    if (ok)
      newset.metadata.maximum = d;
    else
      return;

    d = QInputDialog::getDouble(this, "Step",
                                "Step value for " + name + " = ",
                                1,
                                0,
                                newset.metadata.maximum - newset.metadata.minimum,
                                0.001,
                                &ok);
    if (ok)
      newset.metadata.step = d;
    else
      return;


    manual_settings_["[manual] " + newset.id_] = newset;
    remake_domains();
    saveSettings();
  }
}

void DialogDomain::on_pushEditCustom_clicked()
{
  QString name = ui->comboSetting->currentText();
  if (!manual_settings_.count(name.toStdString()))
    return;
  Qpx::Setting newset = manual_settings_.at(name.toStdString());

  double d = 0;
  bool ok;

  d = QInputDialog::getDouble(this, "Minimum",
                              "Minimum value for " + name + " = ",
                              newset.metadata.minimum,
                              std::numeric_limits<double>::min(),
                              std::numeric_limits<double>::max(),
                              0.001,
                              &ok);
  if (ok)
    newset.metadata.minimum = d;
  else
    return;

  d = QInputDialog::getDouble(this, "Maximum",
                              "Maximum value for " + name + " = ",
                              std::max(newset.metadata.maximum, newset.metadata.minimum),
                              newset.metadata.minimum,
                              std::numeric_limits<double>::max(),
                              0.001,
                              &ok);
  if (ok)
    newset.metadata.maximum = d;
  else
    return;

  d = QInputDialog::getDouble(this, "Step",
                              "Step value for " + name + " = ",
                              newset.metadata.step,
                              0,
                              newset.metadata.maximum - newset.metadata.minimum,
                              0.001,
                              &ok);
  if (ok)
    newset.metadata.step = d;
  else
    return;

  manual_settings_[name.toStdString()] = newset;
  remake_domains();
  ui->comboSetting->setCurrentText(name);
  on_comboSetting_activated("");
  saveSettings();
}

void DialogDomain::on_pushDeleteCustom_clicked()
{
  QString name = ui->comboSetting->currentText();
  if (!manual_settings_.count(name.toStdString()))
    return;

  int ret = QMessageBox::question(this,
                      "Delete setting?",
                      "Are you sure you want to delete custom manual setting \'"
                      + name + "\'?");

  if (!ret)
      return;

  manual_settings_.erase(name.toStdString());
  remake_domains();
  on_comboSetting_activated("");
  saveSettings();
}
