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
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Global functions for Qt application:
 *         validate file for output or input
 *         generate bright translucent color
 *
 ******************************************************************************/

#include "qt_util.h"
#include <stdlib.h>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include "custom_logger.h"

bool validateFile(QWidget* parent, QString name, bool write) {
    QFile file(name);
    if (name.isEmpty())
        return false;

    if (!write) {
        if (!file.exists()) {
            QMessageBox::warning(parent, "Failed", "File does not exist.");
            return false;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(parent, "Failed", "Could not open file for reading.");
            return false;
        }
    } else {
        if (file.exists() && !file.remove()) {
            QMessageBox::warning(parent, "Failed", "Could not delete file.");
            return false;
        }
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(parent, "Failed", "Could not open file for writing.");
            return false;
        }
    }
    file.close();
    return true;
}


QColor generateColor() {
  int H = rand() % 340;
  int S = rand() % 64 + 192;
  int V = rand() % 60 + 160;
  int A = 128;
  return QColor::fromHsv(H, S, V, A);
}

QDateTime fromBoostPtime(boost::posix_time::ptime bpt) {
    std::string bpt_iso = boost::posix_time::to_iso_extended_string(bpt);
    std::replace(bpt_iso.begin(), bpt_iso.end(), '-', ' ');
    std::replace(bpt_iso.begin(), bpt_iso.end(), 'T', ' ');
    std::replace(bpt_iso.begin(), bpt_iso.end(), ':', ' ');
    std::replace(bpt_iso.begin(), bpt_iso.end(), '.', ' ');

    std::stringstream iss;
    iss.str(bpt_iso);

    int year, month, day, hour, minute, second;
    double ms = 0;
    iss >> year >> month >> day >> hour >> minute >> second >> ms;

    while (ms > 999)
        ms = ms / 10;
    ms = round(ms);

    QDate date;
    date.setDate(year, month, day);

    QTime time;
    time.setHMS(hour, minute, second, static_cast<int>(ms));

    QDateTime ret;
    ret.setDate(date);
    ret.setTime(time);

    return ret;
}

boost::posix_time::ptime fromQDateTime(QDateTime qdt) {
    std::string dts = qdt.toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
    boost::posix_time::ptime bpt = boost::posix_time::time_from_string(dts);
    return bpt;
}

QString catExtensions(std::list<std::string> exts) {
  QString ret;
  for (auto &p : exts) {
    ret += "*." + QString::fromStdString(p);
    if (p != exts.back())
      ret += " ";
  }
  return ret;
}

QString catFileTypes(QStringList types) {
  QString ret;
  for (auto &q : types) {
    if (q != types.front())
      ret += ";;";
    ret += q;
  }
  return ret;
}
