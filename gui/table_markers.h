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
 *      TableMarkers -
 *
 ******************************************************************************/

#ifndef TABLE_MARKERS_H
#define TABLE_MARKERS_H

#include <QAbstractTableModel>
#include <map>

class TableMarkers : public QAbstractTableModel
{
  Q_OBJECT

private:
  std::map<double, double>& markers_;
  std::vector<double> markers_chan_;
  std::vector<double> markers_energy_;

public:
  explicit TableMarkers(std::map<double, double>&, QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role);

  void update();

signals:

public slots:

};

#endif // TABLE_MARKERS_H
