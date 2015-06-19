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
 *      ThreadRunner - thread for issuing commands to Pixie outside of gui thread
 *
 ******************************************************************************/

#ifndef THREAD_RUNNER_H_
#define THREAD_RUNNER_H_

#include <QThread>
#include <QMutex>
#include <QVector>
#include <vector>
#include <string>
#include <cstdint>
#include <boost/atomic.hpp>

#include "wrapper.h"
#include "spectra_set.h"
#include "simulator.h"

Q_DECLARE_METATYPE(Pixie::RunType)

enum RunnerAction {
    kBoot, kList, kMCA, kSimulate,
    kOffsets, kBaselines, kOscil,
    kTau, kBLcut, kSettingsRefresh, kOptimize,
    kTerminate, kNone
};

class ThreadRunner : public QThread
{
    Q_OBJECT
public:
    explicit ThreadRunner(QObject *parent = 0);
    ~ThreadRunner();

    void do_boot(bool boot_keepcw, std::vector<std::string> boot_files, std::vector<uint8_t> boot_slots);
    void do_list(boost::atomic<bool>&, Pixie::RunType, uint64_t timeout, bool dblbuf);
    void do_run(Pixie::SpectraSet&, boost::atomic<bool>&, Pixie::RunType, uint64_t timeout, bool dblbuf);
    void do_fake(Pixie::SpectraSet&, boost::atomic<bool>&, QString file, std::array<int,2> chans, int source_res, int dest_res, int timeout);

    void do_offsets();
    void do_baselines();
    void do_optimize();
    void do_oscil(double xdt);
    void do_refresh_settings();
    void do_tau();
    void do_BLcut();
    void terminate();
    bool terminating();

signals:
    void runComplete();
    void listComplete(Pixie::ListData*);
    void bootComplete(bool, bool);
    void settingsUpdated();
    void oscilReadOut(QList<QVector<double>>*, QString);

protected:
    void run();

private:
    QMutex mutex_;
    RunnerAction action_;

    Pixie::SpectraSet* spectra_;
    boost::atomic<bool>* interruptor_;
    boost::atomic<bool> terminating_;

    //boot variables
    std::vector<std::string> boot_files_;
    std::vector<uint8_t> boot_slots_;
    bool boot_keepcw_;
    double mod_filter_, mod_wait_, xdt_;

    //run variables
    Pixie::RunType run_type_;
    bool dblbuf_;
    uint64_t timeout_;

    double value_;
    Pixie::Module module_;
    Pixie::Detector det_;
    std::vector<Pixie::Detector> dets_;
    std::string setting_;

    //simulation
    QString file_;
    int source_res_, dest_res_;
    std::array<int,2> fake_chans_;
};

#endif
