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
 *      ThreadPlotPrep - thread for preparation of spectra to be plotted
 *
 ******************************************************************************/

#ifndef THREAD_PLOT_SIGNAL_H_
#define THREAD_PLOT_SIGNAL_H_

#include <QThread>
#include "spectra_set.h"

class ThreadPlotSignal : public QThread
{
    Q_OBJECT
public:
    explicit ThreadPlotSignal(Pixie::SpectraSet& spectra, QObject *parent = 0) :
        QThread(parent), spectra_(&spectra) {}

    void begin() {
      if (!isRunning())
        start(HighPriority);
    }

signals:
    void plot_ready();

protected:
    void run() {
      while (spectra_->wait_ready()) {
        emit plot_ready();
        QThread::sleep(1);
      }
    }

private:
    Pixie::SpectraSet* spectra_;
};

#endif
