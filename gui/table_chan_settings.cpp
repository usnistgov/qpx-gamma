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
  int num = 0;
   if (channels_.size() > 0)
     num = channels_.front().settings_.branches.size();
  if (num)
    return num + 1;
  else
    return 0;
}

int TableChanSettings::columnCount(const QModelIndex & /*parent*/) const
{
  int num = 2 + channels_.size();
  return num;
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
      else if (col <= channels_.size()) {
        if (role == Qt::EditRole) {
          Gamma::Setting det;
          det.setting_type = Gamma::SettingType::detector;
          det.value_text = channels_[col-1].name_;
          return QVariant::fromValue(det);
        } else if (role == Qt::DisplayRole)
          return QString::fromStdString(channels_[col-1].name_);
      }
    } else {
      if ((col == 0) && (channels_.size() > 0))
        return QString::fromStdString(channels_.front().settings_.branches.get(row-1).name);
      else if (col <= channels_.size())
      {
        Gamma::Setting itemData = channels_[col-1].settings_.branches.get(row-1);
        if (itemData.setting_type == Gamma::SettingType::integer)
          return QVariant::fromValue(itemData.value_int);
        else if (itemData.setting_type == Gamma::SettingType::binary)
          return QVariant::fromValue(itemData.value_int);
        else if (itemData.setting_type == Gamma::SettingType::floating)
          return QVariant::fromValue(itemData.value);
        else if (itemData.setting_type == Gamma::SettingType::int_menu)
          return QString::fromStdString(itemData.int_menu_items.at(itemData.value_int));
        else if (itemData.setting_type == Gamma::SettingType::boolean)
          if (itemData.value_int)
            return "T";
          else
            return "F";
        else if (itemData.setting_type == Gamma::SettingType::text)
          return QString::fromStdString(itemData.value_text);
        else if (itemData.setting_type == Gamma::SettingType::file_path)
          return QString::fromStdString(itemData.value_text);
        else if (itemData.setting_type == Gamma::SettingType::detector)
          return QString::fromStdString(itemData.value_text);
        else
          return QVariant::fromValue(itemData.value);
      }
      else
        return QString::fromStdString(channels_.front().settings_.branches.get(row-1).unit);
    }
  }
  else if (role == Qt::ForegroundRole) {
    if (row == 0) {
      QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta};
      QBrush brush(palette[col % palette.size()]);
      return brush;
    } else if ((col > 0) && (col <= channels_.size()) && (!channels_.front().settings_.branches.get(row-1).writable)) {
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
    else if (col == channels_.size() + 1) {
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
      if (section == 0)
        return QString("Setting name");
      else if (section <= channels_.size())
        return (QString("chan ") + QString::number(section-1));
      else
        return "Units";
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

void TableChanSettings::update(const std::vector<Gamma::Detector> &settings) {
  channels_ = settings;
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex( rowCount() - 1, columnCount() - 1);
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableChanSettings::flags(const QModelIndex &index) const
{
  int row = index.row();
  int col = index.column();

  if ((col > 0) && (col <= channels_.size()))
    if (row == 0)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else if (channels_.front().settings_.branches.get(row-1).writable)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else
      return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}

bool TableChanSettings::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::EditRole)
    if (row > 0) {
      Gamma::Setting itemData = channels_[col-1].settings_.branches.get(row-1);
      if (((itemData.setting_type == Gamma::SettingType::integer) || (itemData.setting_type == Gamma::SettingType::int_menu))
          && (value.type() == QVariant::Int))
        itemData.value_int = value.toInt();
      else if ((itemData.setting_type == Gamma::SettingType::binary)
          && (value.type() == QVariant::Int))
        itemData.value_int = value.toInt();
      else if ((itemData.setting_type == Gamma::SettingType::boolean)
          && (value.type() == QVariant::Bool))
        itemData.value_int = value.toBool();
      else if ((itemData.setting_type == Gamma::SettingType::floating)
          && (value.type() == QVariant::Double))
        itemData.value = value.toDouble();
      else if ((itemData.setting_type == Gamma::SettingType::text)
          && (value.type() == QVariant::String))
        itemData.value_text = value.toString().toStdString();
      else if ((itemData.setting_type == Gamma::SettingType::file_path)
          && (value.type() == QVariant::String))
        itemData.value_text = value.toString().toStdString();
      else if ((itemData.setting_type == Gamma::SettingType::int_menu)
          && (value.type() == QVariant::Int))
        itemData.value_int = value.toInt();
      else if ((itemData.setting_type == Gamma::SettingType::detector)
          && (value.type() == QVariant::String)) {
        itemData.value_text = value.toString().toStdString();
      }
      emit setting_changed(col-1, itemData);
    }
    else if (value.type() == QMetaType::QString) {
      PL_DBG << "table changing detector " << (col-1) << " to " << value.toString().toStdString();
      emit detector_chosen(col-1, value.toString().toStdString());
    }
  return true;
}
