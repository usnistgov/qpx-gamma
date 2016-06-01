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


#ifndef FORM_EXPERIMENT_SETUP_H
#define FORM_EXPERIMENT_SETUP_H

#include <QWidget>
#include <QItemSelection>
#include "experiment.h"
#include "tree_experiment.h"
#include "engine.h"

namespace Ui {
class FormExperimentSetup;
}

class FormExperimentSetup : public QWidget
{
  Q_OBJECT

public:
  explicit FormExperimentSetup(Qpx::ExperimentProject&, QWidget *parent = 0);
  ~FormExperimentSetup();

  void retro_push(Qpx::TrajectoryPtr);
  void update_exp_project();

  void toggle_push(bool);

signals:
  void prototypesChanged();
  void selectedProject(int64_t);
  void toggleIO();

public slots:
  void update_settings(const Qpx::Setting&, const std::vector<Qpx::Detector>&, Qpx::SourceStatus);

private slots:

  void toggle_push();

  void on_pushAddSubdomain_clicked();
  void on_pushEditDomain_clicked();
  void on_pushDeleteDomain_clicked();

  void item_selected_in_tree(QItemSelection,QItemSelection);

  void on_pushEditPrototypes_clicked();

private:
  Ui::FormExperimentSetup *ui;

  Qpx::ExperimentProject &exp_project_;

  TreeExperiment tree_experiment_model_;

  std::vector<Qpx::Detector> current_dets_;
  Qpx::Setting current_tree_;

  std::map<std::string, Qpx::Domain> all_domains_;

  void loadSettings();
  void saveSettings();

  void remake_domains();
};

#endif // FORM_CALIBRATION_H
