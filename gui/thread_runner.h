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

#pragma once

#include <QThread>
#include <QMutex>
#include <QVector>
#include <vector>
#include <string>
#include <cstdint>
#include <boost/atomic.hpp>

#include "engine.h"
#include "project.h"

enum RunnerAction {kBoot, kShutdown, kPushSettings, kSetSetting, kSetDetector, kSetDetectors,
    kList, kMCA, kOscil, kInitialize, kSettingsRefresh, kOptimize, kTerminate, kNone
};

class ThreadRunner : public QThread
{
    Q_OBJECT
public:
    explicit ThreadRunner(QObject *parent = 0);
    ~ThreadRunner();

    void set_idle_refresh(bool);
    void set_idle_refresh_frequency(int);


    void do_initialize();
    void do_boot();
    void do_shutdown();
    void do_push_settings(const Qpx::Setting &tree);
    void do_set_setting(const Qpx::Setting &item, Qpx::Match match);
    void do_set_detector(int, Qpx::Detector);
    void do_set_detectors(std::map<int, Qpx::Detector>);

    void do_list(boost::atomic<bool>&, uint64_t timeout);
    void do_run(Qpx::ProjectPtr, boost::atomic<bool>&, uint64_t timeout);

    void do_optimize();
    void do_oscil();
    void do_refresh_settings();
    void terminate();
    bool terminating();
    bool running() {return running_.load();}

signals:
    void bootComplete();
    void runComplete();
    void listComplete(Qpx::ListData);
    void settingsUpdated(Qpx::Setting, std::vector<Qpx::Detector>, Qpx::ProducerStatus);
    void oscilReadOut(std::vector<Qpx::Hit>);

protected:
    void run();

private:
    Qpx::Engine &engine_;
    QMutex mutex_;
    RunnerAction action_;
    boost::atomic<bool> running_;
    boost::atomic<bool> idle_refresh_;
    boost::atomic<int> idle_refresh_frequency_;


    Qpx::ProjectPtr spectra_;
    boost::atomic<bool>* interruptor_;
    boost::atomic<bool> terminating_;

    uint64_t timeout_;

    std::map<int, Qpx::Detector> detectors_;
    Qpx::Detector det_;
    int chan_;
    Qpx::Setting tree_, one_setting_;
    Qpx::Match match_conditions_;

    bool flag_;

    Qpx::ProducerStatus recent_status_;
};
