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
#include "widget_selector.h"

namespace Ui {
class FormExperimentSetup;
}

class FormExperimentSetup : public QWidget
{
  Q_OBJECT

public:
  explicit FormExperimentSetup(XMLableDB<Qpx::Detector>&, Qpx::ExperimentProject&, QWidget *parent = 0);
  ~FormExperimentSetup();

  void update_settings();
  void retro_push(Qpx::TrajectoryPtr);

  void update_exp_project();

signals:
  void selectedProject(int64_t);
  void selectedSink(int64_t);

private slots:

  void toggle_push();

  void on_pushAddSubdomain_clicked();
  void on_pushEditDomain_clicked();
  void on_pushDeleteDomain_clicked();

  void item_selected_in_tree(QItemSelection,QItemSelection);

  void on_pushEditPrototypes_clicked();

  void spectrumDoubleclicked(SelectorItem item);
  void choose_spectrum(SelectorItem item);

private:
  Ui::FormExperimentSetup *ui;

  Qpx::ExperimentProject &exp_project_;

  TreeExperiment tree_experiment_model_;

  std::map<std::string, Qpx::Domain> all_domains_;

  int64_t current_spectrum_;

//  Qpx::Metadata sink_prototype_;

  //from parent
  QString data_directory_;
  QString settings_directory_;

  XMLableDB<Qpx::Detector> &detectors_; //unused!!!
  std::vector<Qpx::Detector> current_dets_;

  void loadSettings();
  void saveSettings();

  void remake_domains();
  void populate_selector();
};

#endif // FORM_CALIBRATION_H
