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

QString CustomSaveFileDialog(QWidget *parent,
                           const QString &title,
                           const QString &directory,
                           const QString &filter) {
#if defined(Q_WS_WIN) || defined(Q_WS_MAC)
  return QFileDialog::getSaveFileName(parent,
                                      title,
                                      directory,
                                      filter);
#else
  QFileDialog dialog(parent, title, directory, filter);
  if (parent) {
    dialog.setWindowModality(Qt::WindowModal);
  }
  QRegExp filter_regex(QLatin1String("(?:^\\*\\.(?!.*\\()|\\(\\*\\.)(\\w+)"));
  QStringList filters = filter.split(QLatin1String(";;"));
  if (!filters.isEmpty()) {
    dialog.setNameFilter(filters.first());
    if (filter_regex.indexIn(filters.first()) != -1) {
      dialog.setDefaultSuffix(filter_regex.cap(1));
    }
  }
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  if (dialog.exec() == QDialog::Accepted) {
    QString file_name = dialog.selectedFiles().first();
    QFileInfo info(file_name);
    if (info.suffix().isEmpty() && !dialog.selectedNameFilter().isEmpty()) {
      if (filter_regex.indexIn(dialog.selectedNameFilter()) != -1) {
        QString extension = filter_regex.cap(1);
        file_name += QLatin1String(".") + extension;
      }
    }

    QFile file(file_name);
    if (file.exists()) {
        QMessageBox msgBox;
        msgBox.setText("Replace?");
        msgBox.setInformativeText("File \'" + file_name + "\' already exists. Replace?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (msgBox.exec() != QMessageBox::Yes)
          return QString();
    }

    return file_name;
  } else {
    return QString();
  }
#endif  // Q_WS_MAC || Q_WS_WIN
}

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
  int H = rand() % 359;
  int S = rand() % 64 + 191;
  int V = rand() % 54 + 181;
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

void add_to_table(QTableWidget *table,
                  int row, int col, std::string data,
                  QVariant value, QBrush background)
{
  QTableWidgetItem * item = new QTableWidgetItem(QString::fromStdString(data));
//  item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  item->setFlags(item->flags() ^ Qt::ItemIsEditable);
  item->setData(Qt::UserRole, value);
  item->setData(Qt::BackgroundRole, background);
  table->setItem(row, col, item);
//  DBG << "added " << data << " and " << value.toDouble();
}
