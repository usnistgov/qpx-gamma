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

#include "gui/table_markers.h"

TableMarkers::TableMarkers(std::map<double, double>& markers, QObject *parent) :
  QAbstractTableModel(parent),
  markers_(markers)
{
}

int TableMarkers::rowCount(const QModelIndex & /*parent*/) const
{
  return markers_chan_.size();
}

int TableMarkers::columnCount(const QModelIndex & /*parent*/) const
{
  return 2;
}

QVariant TableMarkers::data(const QModelIndex &index, int role) const
{
  int col = index.column();
  int row = index.row();
  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    switch (col) {
    case 0:
      return QVariant::fromValue(markers_chan_[row]);
    case 1:
      return QVariant::fromValue(markers_energy_[row]);
    }
  }
  return QVariant();
}

QVariant TableMarkers::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("Channel");
      case 1:
        return QString("Energy");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableMarkers::update() {
  markers_chan_.clear();
  markers_energy_.clear();
  for (auto &q: markers_ ) {
    markers_chan_.push_back(q.first);
    markers_energy_.push_back(q.second);
  }
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( (rowCount() - 1), (columnCount() - 1) );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableMarkers::flags(const QModelIndex &index) const
{
  Qt::ItemFlags myflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index) & ~Qt::ItemIsSelectable;

  int col = index.column();

  if (col == 1)
    return Qt::ItemIsEditable | myflags;
  else
    return myflags;

  return myflags;
}

bool TableMarkers::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  QModelIndex ix = createIndex(row, 0);

  if (role == Qt::EditRole)
    markers_[data(ix, Qt::EditRole).toDouble()] = value.toDouble();

  update();
  return true;
}
