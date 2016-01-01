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
 *      ThreadRunner - thread for issuing commands to daq outside of gui thread
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

#include "engine.h"
#include "spectra_set.h"
#include "simulator.h"

enum RunnerAction {
    kExecuteCommand, kBoot, kShutdown, kPushSettings, kSetSetting, kSetDetector, kSetDetectors,
    kList, kMCA, kSimulate, kOscil, kInitialize,
    kSettingsRefresh, kOptimize, kTerminate, kNone
};

class ThreadRunner : public QThread
{
    Q_OBJECT
public:
    explicit ThreadRunner(QObject *parent = 0);
    ~ThreadRunner();

    void do_initialize(QString);
    void do_boot();
    void do_shutdown();
    void do_execute_command(const Gamma::Setting &tree);
    void do_push_settings(const Gamma::Setting &tree);
    void do_set_setting(const Gamma::Setting &item, Gamma::Match match);
    void do_set_detector(int, Gamma::Detector);
    void do_set_detectors(std::map<int, Gamma::Detector>);

    void do_list(boost::atomic<bool>&, uint64_t timeout);
    void do_run(Qpx::SpectraSet&, boost::atomic<bool>&, uint64_t timeout);
    void do_fake(Qpx::SpectraSet&, boost::atomic<bool>&, QString file, std::array<int,2> chans, int source_res, int dest_res, int timeout);

    void do_optimize();
    void do_oscil(double xdt);
    void do_refresh_settings();
    void terminate();
    bool terminating();
    bool running() {return running_.load();}

signals:
    void bootComplete();
    void runComplete();
    void listComplete(Qpx::ListData*);
    void settingsUpdated(Gamma::Setting, std::vector<Gamma::Detector>, Qpx::DeviceStatus);
    void oscilReadOut(std::vector<Qpx::Trace>);

protected:
    void run();

private:
    Qpx::Engine &engine_;

    QMutex mutex_;
    RunnerAction action_;

    Qpx::SpectraSet* spectra_;
    boost::atomic<bool>* interruptor_;
    boost::atomic<bool> terminating_;
    boost::atomic<bool> running_;

    double xdt_;

    uint64_t timeout_;

    std::map<int, Gamma::Detector> detectors_;
    Gamma::Detector det_;
    int chan_;
    Gamma::Setting tree_;
    Gamma::Match match_conditions_;

    //simulation
    QString file_;
    int source_res_, dest_res_;
    std::array<int,2> fake_chans_;
};

#endif
