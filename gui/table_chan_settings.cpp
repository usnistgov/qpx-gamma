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
#include <boost/algorithm/string.hpp>

void TableChanSettings::set_show_read_only(bool show_ro) {
  show_read_only_ = show_ro;
}


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

  if (row == 0) {
    if (col == 0) {
      if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
        return "<===detector===>";
      else if (role == Qt::FontRole) {
        QFont regularfont;
        regularfont.setPointSize(10);
        return regularfont;
      } else if (role == Qt::ForegroundRole) {
        QBrush brush(Qt::black);
        return brush;
      }
    }
    else if (col <= channels_.size()) {
      if (role == Qt::EditRole) {
        Gamma::Setting det;
        det.setting_type = Gamma::SettingType::detector;
        det.value_text = channels_[col-1].name_;
        return QVariant::fromValue(det);
      } else if (role == Qt::DisplayRole)
        return QString::fromStdString(channels_[col-1].name_);
      else if (role == Qt::ForegroundRole) {
        QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta};
        QBrush brush(palette[(col-1) % palette.size()]);
        return brush;
      } else if (role == Qt::FontRole) {
        QFont boldfont;
        boldfont.setBold(true);
        boldfont.setPointSize(10);
        return boldfont;
      }
    }
  } else if ((col == 0) && (channels_.size() > 0)) {
    Gamma::Setting item = channels_.front().settings_.branches.get(row-1);
    if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
      std::vector<std::string> tokens;
      boost::algorithm::split(tokens, item.name, boost::algorithm::is_any_of("/"));
      std::string name;
      if (tokens.size() > 1)
        name += tokens[tokens.size() - 2] + "/" + tokens[tokens.size() - 1];
      else if (tokens.size() > 0)
        name += tokens[tokens.size() - 1];
      return QString::fromStdString(name);
    } else if (role == Qt::ForegroundRole) {
      if (item.writable) {
        QBrush brush(Qt::black);
        return brush;
      } else {
        QBrush brush(Qt::darkGray);
        return brush;
      }
    } else if (role == Qt::FontRole) {
      QFont boldfont;
      boldfont.setBold(true);
      boldfont.setPointSize(10);
      return boldfont;
    }
  } else if ((col > channels_.size()) && (!channels_.empty())) {
    Gamma::Setting item = channels_.front().settings_.branches.get(row-1);
    if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
      return QString::fromStdString(item.unit);
    else if (role == Qt::ForegroundRole) {
      if (item.writable) {
        QBrush brush(Qt::black);
        return brush;
      } else {
        QBrush brush(Qt::darkGray);
        return brush;
      }
    } else if (role == Qt::FontRole) {
      QFont italicfont;
      italicfont.setItalic(true);
      italicfont.setPointSize(10);
      return italicfont;
    }
  } else if (col <= channels_.size()) {
    Gamma::Setting item = channels_[col-1].settings_.branches.get(row-1);
    if (role == Qt::EditRole)
      return QVariant::fromValue(item);
    else if (role == Qt::ForegroundRole) {
      if (item.writable) {
        QBrush brush(Qt::black);
        return brush;
      } else {
        QBrush brush(Qt::darkGray);
        return brush;
      }
    } else if (role == Qt::FontRole) {
      QFont regularfont;
      regularfont.setPointSize(10);
      return regularfont;
    } else if (role == Qt::DisplayRole) {
      if (item.setting_type == Gamma::SettingType::integer)
        return QVariant::fromValue(item.value_int);
      else if (item.setting_type == Gamma::SettingType::binary) {
        QString hex = QString::number(item.value_int, 16).toUpper();
        int size = item.maximum;
        int tot = (size / 4);
        if ((size % 4) > 0)
          tot++;
        int dif = tot - hex.length();
        while (dif > 0) {
          hex = QString("0") + hex;
          dif--;
        }
        hex = QString("0x") + hex;
        return hex;
      } else if (item.setting_type == Gamma::SettingType::floating)
        return QVariant::fromValue(item.value);
      else if (item.setting_type == Gamma::SettingType::int_menu)
        return QString::fromStdString(item.int_menu_items.at(item.value_int));
      else if (item.setting_type == Gamma::SettingType::boolean)
        if (item.value_int)
          return "T";
        else
          return "F";
      else if (item.setting_type == Gamma::SettingType::text)
        return QString::fromStdString(item.value_text);
      else if (item.setting_type == Gamma::SettingType::file_path)
        return QString::fromStdString(item.value_text);
      else if (item.setting_type == Gamma::SettingType::detector)
        return QString::fromStdString(item.value_text);
      else
        return QVariant::fromValue(item.value);
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
  if (!show_read_only_) {
    for (int i=0; i < settings.size(); ++i) {
      channels_[i].settings_.branches.clear();
      for (auto &q : settings[i].settings_.branches.my_data_) {
        if (q.writable)
          channels_[i].settings_.branches.add(q);
      }
    }
  }
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
      Gamma::Setting item = channels_[col-1].settings_.branches.get(row-1);
      if (((item.setting_type == Gamma::SettingType::integer) || (item.setting_type == Gamma::SettingType::int_menu))
          && (value.type() == QVariant::Int))
        item.value_int = value.toInt();
      else if ((item.setting_type == Gamma::SettingType::binary)
          && (value.type() == QVariant::Int))
        item.value_int = value.toInt();
      else if ((item.setting_type == Gamma::SettingType::boolean)
          && (value.type() == QVariant::Bool))
        item.value_int = value.toBool();
      else if ((item.setting_type == Gamma::SettingType::floating)
          && (value.type() == QVariant::Double))
        item.value = value.toDouble();
      else if ((item.setting_type == Gamma::SettingType::text)
          && (value.type() == QVariant::String))
        item.value_text = value.toString().toStdString();
      else if ((item.setting_type == Gamma::SettingType::file_path)
          && (value.type() == QVariant::String))
        item.value_text = value.toString().toStdString();
      else if ((item.setting_type == Gamma::SettingType::int_menu)
          && (value.type() == QVariant::Int))
        item.value_int = value.toInt();
      else if ((item.setting_type == Gamma::SettingType::detector)
          && (value.type() == QVariant::String)) {
        item.value_text = value.toString().toStdString();
      }
      emit setting_changed(col-1, item);
    }
    else if (value.type() == QMetaType::QString) {
      PL_DBG << "table changing detector " << (col-1) << " to " << value.toString().toStdString();
      emit detector_chosen(col-1, value.toString().toStdString());
    }
  return true;
}
