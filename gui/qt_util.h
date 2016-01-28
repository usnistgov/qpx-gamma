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

#ifndef QT_UTIL_H_
#define QT_UTIL_H_
#include <QWidget>
#include <QFileDialog>
#include <boost/date_time.hpp>


//  Modified FileDialog by Dave Mateer
//  http://stackoverflow.com/users/183339/dave-mateer
QString CustomSaveFileDialog(QWidget *parent,
                           const QString &title,
                           const QString &directory,
                           const QString &filter);

bool validateFile(QWidget* parent, QString, bool);
QColor generateColor();
QDateTime fromBoostPtime(boost::posix_time::ptime);
boost::posix_time::ptime fromQDateTime(QDateTime);
QString catExtensions(std::list<std::string> exts);
QString catFileTypes(QStringList types);

#endif
