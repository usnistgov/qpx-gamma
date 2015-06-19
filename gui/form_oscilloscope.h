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
 *      Oscilloscope
 *
 ******************************************************************************/

#ifndef FORM_OSCILlOSCOPE_H
#define FORM_OSCILlOSCOPE_H

#include <QWidget>
#include "thread_runner.h"
#include "widget_plot1d.h"

namespace Ui {
class FormOscilloscope;
}

class FormOscilloscope : public QWidget
{
  Q_OBJECT

public:
  explicit FormOscilloscope(ThreadRunner&, QWidget *parent = 0);
  ~FormOscilloscope();

signals:
  void toggleIO(bool);
  void statusText(QString);

private slots:
  void on_pushOscilRefresh_clicked();
  void on_pushOscilBaselines_clicked();
  void on_pushOscilOffsets_clicked();
  void oscil_complete(QList<QVector<double>>*, QString);

  void toggle_push(bool enable, Pixie::LiveStatus live);

  void on_doubleSpinXDT_editingFinished();

private:
  Ui::FormOscilloscope *ui;
  ThreadRunner &runner_thread_;
};

#endif // FORM_OSCILlOSCOPE_H
