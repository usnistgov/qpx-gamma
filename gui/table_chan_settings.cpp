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
 * Description:
 *      TableChanSettings - table for displaying and manipulating Pixie's
 *      channel settings and chosing detectors.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "table_chan_settings.h"
#include "special_delegate.h"

int TableChanSettings::rowCount(const QModelIndex & /*parent*/) const
{
  int num = my_settings_.chan_param_num();
  if (num)
    return num + 1;
  else
    return 0;
}

int TableChanSettings::columnCount(const QModelIndex & /*parent*/) const
{
  return 6;
}

QVariant TableChanSettings::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
  {
    if (row == 0) {
      if (col == 0)
        return "<===detector===>";
      else if (col < 5)
        return QString::fromStdString(my_settings_.get_detector(Pixie::Channel(col-1)).name_);
    } else {
      switch (col) {
      case 0:
        return QString::fromStdString(chan_meta_[row-1].name);
      case 1:
      case 2:
      case 3:
      case 4:
        if (chan_meta_[row-1].unit == "binary")
          return QString::number(static_cast<uint32_t>(my_settings_.get_chan(row - 1, Pixie::Channel(col-1))), 16).toUpper();
        else
          return QVariant::fromValue(my_settings_.get_chan(row - 1, Pixie::Channel(col-1)));
      case 5:
        return QString::fromStdString(chan_meta_[row-1].unit);
      }
    }
  }
  else if (role == Qt::ForegroundRole) {
    if (row == 0) {
      QVector<QColor> palette {Qt::black, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta};
      QBrush brush(palette[col]);
      return brush;
    } else if ((col > 0) && (col < 5) && ((!chan_meta_[row-1].writable)
                                          || (chan_meta_[row-1].unit == "binary"))) {
      QBrush brush(Qt::darkGray);
      return brush;
    } else {
      QBrush brush(Qt::black);
      return brush;
    }
  }
  else if (role == Qt::FontRole) {
    if ((col == 0) || (row == 0)) {
      QFont boldfont;
      boldfont.setBold(true);
      boldfont.setPointSize(10);
      return boldfont;
    }
    else if (col == 5) {
      QFont italicfont;
      italicfont.setItalic(true);
      italicfont.setPointSize(10);
      return italicfont;
    }
    else {
      QFont regularfont;
      regularfont.setPointSize(10);
      return regularfont;
    }
  }
  return QVariant();
}

QVariant TableChanSettings::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("Setting name");
      case 1:
      case 2:
      case 3:
      case 4:
        return (QString("chan ") + QString::number(section-1));
      case 5:
        return "Units";
      }
    } else if (orientation == Qt::Vertical) {
      if (section)
        return QString::number(section-1);
      else
        return "D";
    }
  }
  else if (role == Qt::FontRole) {
    QFont boldfont;
    boldfont.setPointSize(10);
    boldfont.setCapitalization(QFont::AllUppercase);
    return boldfont;
  }
  return QVariant();
}

void TableChanSettings::update() {
  chan_meta_ = my_settings_.channel_meta();
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() , 5 );
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableChanSettings::flags(const QModelIndex &index) const
{
  int row = index.row();
  int col = index.column();

  if ((col > 0) && (col < 5))
    if (row == 0)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else if ((chan_meta_[row-1].writable) &&
             (chan_meta_[row-1].unit != "binary"))
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else
      return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}

bool TableChanSettings::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::EditRole)
    if (row)
      my_settings_.set_chan(row - 1, value.toDouble(), Pixie::Channel(col - 1));
    else if (value.canConvert<Pixie::Detector>()) {
      my_settings_.set_detector(Pixie::Channel(col - 1), qvariant_cast<Pixie::Detector>(value));
      emit detectors_changed();
    }

  return true;
}
