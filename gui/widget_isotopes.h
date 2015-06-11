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
 *      Isotope DB browser
 *
 ******************************************************************************/

#ifndef WIDGET_ISOTOPES_H
#define WIDGET_ISOTOPES_H

#include <QWidget>
#include "isotope.h"

namespace Ui {
class WidgetIsotopes;
}

class WidgetIsotopes : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetIsotopes(QWidget *parent = 0);
  void setDir(QString filedir);
  ~WidgetIsotopes();
  std::vector<double> current_gammas() const;
  QString current_isotope() const;
  void set_current_isotope(QString);

signals:
  void energiesSelected();

private slots:
  void on_pushOpen_clicked();
  void isotopeChosen(QString);
  void energiesChosen();

  void on_pushSum_clicked();

  void on_pushRemove_clicked();

private:

  Ui::WidgetIsotopes *ui;
  QString root_dir_;

  XMLableDB<RadTypes::Isotope> isotopes_;

  std::vector<double> current_gammas_;

};

#endif // WIDGET_ISOTOPES_H
