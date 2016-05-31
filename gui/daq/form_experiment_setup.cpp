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
 *      FormExperimentSetup -
 *
 ******************************************************************************/

#include "form_experiment_setup.h"
#include "widget_detectors.h"
#include "ui_form_experiment_setup.h"
#include "qt_util.h"
#include "dialog_domain.h"
#include <QSettings>

#include "engine.h"
#include "daq_sink_factory.h"
#include "dialog_spectra_templates.h"

Q_DECLARE_METATYPE(Qpx::TrajectoryNode)

FormExperimentSetup::FormExperimentSetup(Qpx::ExperimentProject &project, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormExperimentSetup),
  exp_project_(project),
  tree_experiment_model_(this)
{
  ui->setupUi(this);

  ui->treeViewExperiment->setModel(&tree_experiment_model_);
  ui->treeViewExperiment->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->treeViewExperiment->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->treeViewExperiment->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->treeViewExperiment->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(ui->treeViewExperiment->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(item_selected_in_tree(QItemSelection,QItemSelection)));

  loadSettings();

  update_settings();
}

FormExperimentSetup::~FormExperimentSetup()
{
  delete ui;
}

void FormExperimentSetup::retro_push(Qpx::TrajectoryPtr node)
{
  if (exp_project_.push_next_setting(node))
    tree_experiment_model_.retro_push(node);
//  emit toggleIO();
}

void FormExperimentSetup::loadSettings() {
  QSettings settings_;

}

void FormExperimentSetup::saveSettings()
{
  QSettings settings_;
}

void FormExperimentSetup::update_exp_project()
{
  tree_experiment_model_.set_root(exp_project_.get_trajectories());
  item_selected_in_tree(QItemSelection(),QItemSelection());
  remake_domains();
  toggle_push();
}


void FormExperimentSetup::toggle_push() {
  Qpx::Setting time;
  time.metadata.setting_type = Qpx::SettingType::time_duration;
  time.value_duration = boost::posix_time::milliseconds(exp_project_.estimate_total_time() * 1000.0);

  ui->labelTime->setText(
        "Estimated total run time = " + QString::fromStdString(time.val_to_pretty_string())
        );

  bool hasresults = exp_project_.has_results();

  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  ui->pushAddSubdomain->setEnabled(!idx.empty() && !hasresults);
  ui->pushDeleteDomain->setEnabled(!idx.empty());
  ui->pushEditPrototypes->setEnabled(!hasresults);
  ui->pushEditDomain->setEnabled(false);

  if (idx.empty() || !idx.front().isValid())
    return;

  auto &i = idx.front();

  QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
  if (!data.canConvert<Qpx::TrajectoryNode>())
    return;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

  ui->pushEditDomain->setEnabled(!idx.empty() && !hasresults && (tn.data_idx < 0));
}

void FormExperimentSetup::update_settings() {
  //should come from other thread?
  current_dets_ = Qpx::Engine::getInstance().get_detectors();
  remake_domains();
}

void FormExperimentSetup::on_pushAddSubdomain_clicked()
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  if (idx.empty())
    return;
  QModelIndex i = idx.front();
  if (!i.isValid())
    return;

  QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
  if (!data.canConvert<Qpx::TrajectoryNode>())
    return;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

  if (tn.data_idx >= 0)
    return;

  Qpx::Domain newdomain;
  DialogDomain diag(newdomain, all_domains_, this);
  int ret = diag.exec();
  if (!ret || (newdomain.type == Qpx::DomainType::none))
    return;

  tn = Qpx::TrajectoryNode();
  tn.domain = newdomain;

  tree_experiment_model_.push_back(i, tn);
  exp_project_.notity_tree_change();

  emit toggleIO();
}

void FormExperimentSetup::on_pushEditDomain_clicked()
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  if (idx.empty())
    return;
  QModelIndex i = idx.front();
  if (!i.isValid())
    return;

  QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
  if (!data.canConvert<Qpx::TrajectoryNode>())
    return;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

  if ((tn.data_idx >= 0) || (tn.domain_value != Qpx::Setting()) || (tn.domain.verbose == "root"))
    return;

  DialogDomain diag(tn.domain, all_domains_, this);
  int ret = diag.exec();
  if (!ret || (tn.domain.type == Qpx::DomainType::none))
    return;

  tree_experiment_model_.setData(i, QVariant::fromValue(tn), Qt::EditRole);
  exp_project_.notity_tree_change();

  emit toggleIO();
}

void FormExperimentSetup::on_pushDeleteDomain_clicked()
{
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  if (idx.empty())
    return;
  QModelIndex i = idx.front();
  if (!i.isValid())
    return;

  QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
  if (!data.canConvert<Qpx::TrajectoryNode>())
    return;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

  if (tree_experiment_model_.remove_row(i))
  {
    if (tn.data_idx >=0)
      exp_project_.delete_data(tn.data_idx);
    else
      exp_project_.notity_tree_change();
    emit toggleIO();
  }
}


void FormExperimentSetup::remake_domains()
{
  XMLableDB<Qpx::Metadata> tpt = exp_project_.get_prototypes();

  std::map<std::string, Qpx::Setting> source_settings;
  for (uint16_t i=0; i < current_dets_.size(); ++i) {
    bool relevant = false;
    for (auto &p : tpt.my_data_)
      if (p.chan_relevant(i))
        relevant = true;
    if (!relevant)
      continue;
    for (auto &q : current_dets_[i].settings_.branches.my_data_)
      source_settings["[DAQ] " + q.id_
          + " (" + std::to_string(i) + ":" + current_dets_[i].name_ + ")"] = q;
  }


  std::map<std::string, Qpx::Setting> sink_settings;
  for (auto &p : tpt.my_data_) {
    Qpx::Setting allset, set;
    allset = p.attributes;
    allset.cull_invisible();
    set.branches.my_data_ = allset.find_all(set, Qpx::Match::indices);
    for (auto &q : set.branches.my_data_)
      sink_settings["[" + p.type() + "] " + q.id_] = q;
    for (uint16_t i=0; i < current_dets_.size(); ++i) {
      if (!p.chan_relevant(i))
        continue;
      set.indices.clear();
      set.indices.insert(i);
      set.branches.my_data_ = p.attributes.find_all(set, Qpx::Match::indices);
      for (auto &q : set.branches.my_data_)
        sink_settings["[" + p.type() + "] " + q.id_
            + " (" + std::to_string(i) + ":" + current_dets_[i].name_ + ")"] = q;
    }
  }


  all_domains_.clear();

  for (auto &s : source_settings)
    if (s.second.metadata.writable
        && s.second.metadata.visible
        && s.second.is_numeric()) {
      Qpx::Domain domain;
      domain.verbose = s.first;
      domain.type = Qpx::DomainType::source;
      domain.value_range = s.second;
      all_domains_[s.first] = domain;
    }

  for (auto &s : sink_settings)
    if (s.second.metadata.writable
        && s.second.metadata.visible
        && s.second.is_numeric())
    {
      Qpx::Domain domain;
      domain.verbose = s.first;
      domain.type = Qpx::DomainType::sink;
      domain.value_range = s.second;
      all_domains_[s.first] = domain;
    }
}

void FormExperimentSetup::item_selected_in_tree(QItemSelection,QItemSelection)
{
  toggle_push();
  auto idx = ui->treeViewExperiment->selectionModel()->selectedIndexes();
  if (idx.empty() || !idx.front().isValid())
  {
    emit selectedProject(-1);
//    choose_spectrum(SelectorItem());
    return;
  }

  auto &i = idx.front();

  QVariant data = tree_experiment_model_.data(i, Qt::EditRole);
  if (!data.canConvert<Qpx::TrajectoryNode>())
    return;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(data);

  emit selectedProject(tn.data_idx);
//  choose_spectrum(SelectorItem());
}

void FormExperimentSetup::on_pushEditPrototypes_clicked()
{
  QSettings settings_;

  settings_.beginGroup("Program");
  QString profile_directory_ = settings_.value("profile_directory", QDir::homePath() + "/qpx/settings").toString();

  XMLableDB<Qpx::Metadata> ptp = exp_project_.get_prototypes();
  DialogSpectraTemplates* newDialog = new DialogSpectraTemplates(ptp, current_dets_,
                                                                 profile_directory_, this);
  newDialog->exec();

  exp_project_.set_prototypes(ptp);
  remake_domains();
  emit prototypesChanged();
  emit toggleIO();
}

void FormExperimentSetup::toggle_push(bool enable)
{
  ui->widgetButtons->setEnabled(enable);
  toggle_push();
}
