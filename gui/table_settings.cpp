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
 *      TableChanSettings - table for displaying and manipulating
 *      channel settings and chosing detectors.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "table_settings.h"
#include "special_delegate.h"
#include <boost/algorithm/string.hpp>

TableChanSettings::TableChanSettings(QObject *parent) {
  show_read_only_ = true;
  scalable_units_.insert("V");
  scalable_units_.insert("A");
  scalable_units_.insert("s");
  scalable_units_.insert("Hz");
}

void TableChanSettings::set_show_read_only(bool show_ro) {
  show_read_only_ = show_ro;
}


int TableChanSettings::rowCount(const QModelIndex & /*parent*/) const
{
  int num = 0;
   if ((channels_.size() > 0) && (!consolidated_list_.branches.empty()))
     num = consolidated_list_.branches.size();
  if (num)
    return num + 1;
  else
    return 0;
}

int TableChanSettings::columnCount(const QModelIndex & /*parent*/) const
{
  int num = 3 + channels_.size();
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
        det.metadata.setting_type = Gamma::SettingType::detector;
        det.value_text = channels_[col-1].name_;
        return QVariant::fromValue(det);
      } else if (role == Qt::DisplayRole)
        return QString::fromStdString(channels_[col-1].name_);
      else if (role == Qt::ForegroundRole) {
        QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};
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
    Gamma::Setting item = consolidated_list_.branches.get(row-1);
    if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
      std::vector<std::string> tokens;
      boost::algorithm::split(tokens, item.id_, boost::algorithm::is_any_of("/"));
      std::string name;
//      if (tokens.size() > 2)
//        name = tokens[0] + "/../" + tokens[tokens.size() - 1];
//      else
        name = item.id_;
      return QString::fromStdString(name);
    } else if (role == Qt::ForegroundRole) {
      if (item.metadata.writable) {
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
  } else if ((col == (channels_.size()+1)) && (!channels_.empty()) && (!consolidated_list_.branches.empty())) {
    Gamma::Setting item = consolidated_list_.branches.get(row-1);
    if ((role == Qt::DisplayRole) && preferred_units_.count(item.id_))
      return QString::fromStdString(preferred_units_.at(item.id_));
    else if  ((role == Qt::EditRole) && preferred_units_.count(item.id_)) {
      UnitConverter uc;
      Gamma::Setting st("unit");
      st.metadata.setting_type = Gamma::SettingType::int_menu;
      if (item.metadata.minimum)
        st.metadata.int_menu_items = uc.make_ordered_map(item.metadata.unit, item.metadata.minimum, item.metadata.maximum);
      else
        st.metadata.int_menu_items = uc.make_ordered_map(item.metadata.unit, item.metadata.step, item.metadata.maximum);
      for (auto &q : st.metadata.int_menu_items)
        if (q.second == preferred_units_.at(item.id_))
          st.value_int = q.first;
      return QVariant::fromValue(st);
    }
    else if (role == Qt::ForegroundRole) {
      if (item.metadata.writable) {
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
  } else if ((col == (channels_.size()+2)) && (!channels_.empty()) && (!consolidated_list_.branches.empty())) {
    Gamma::Setting item = consolidated_list_.branches.get(row-1);
    if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
      return QString::fromStdString(item.metadata.description);
    } else if (role == Qt::ForegroundRole) {
      if (item.metadata.writable) {
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
    Gamma::Setting item = consolidated_list_.branches.get(row-1);
    item = channels_[col-1].settings_.get_setting(item, Gamma::Match::id);
    //item.index = col - 1;
    //item.indices.clear();
    //item.indices.insert(item.index);
    if (item == Gamma::Setting())
      return QVariant();
    if (role == Qt::EditRole) {
      if ((item.metadata.setting_type == Gamma::SettingType::floating)
          && preferred_units_.count(item.id_)
          && (item.metadata.unit != preferred_units_.at(item.id_))) {
            std::string to_units = preferred_units_.at(item.id_);
            UnitConverter uc;
            item.value_dbl = uc.convert_units(item.value_dbl, item.metadata.unit, to_units).convert_to<double>();
            item.metadata.minimum = uc.convert_units(item.metadata.minimum, item.metadata.unit, to_units).convert_to<double>();
            item.metadata.step = uc.convert_units(item.metadata.step, item.metadata.unit, to_units).convert_to<double>();
            item.metadata.maximum = uc.convert_units(item.metadata.maximum, item.metadata.unit, to_units).convert_to<double>();
          }
      return QVariant::fromValue(item);
    }
    else if (role == Qt::ForegroundRole) {
      if (item.metadata.setting_type == Gamma::SettingType::indicator) {
        QBrush brush(Qt::white);
        return brush;
      } else if (item.metadata.writable) {
        QBrush brush(Qt::black);
        return brush;
      } else {
        QBrush brush(Qt::darkGray);
        return brush;
      }
    } else if (role == Qt::BackgroundColorRole) {
      if (item.metadata.setting_type == Gamma::SettingType::indicator)
        return QColor::fromRgba(item.get_setting(Gamma::Setting(item.metadata.int_menu_items.at(item.value_int)), Gamma::Match::id).metadata.address);
      else
        return QVariant();
    } else if (role == Qt::FontRole) {
      if (item.metadata.setting_type == Gamma::SettingType::indicator) {
        QFont boldfont;
        boldfont.setBold(true);
        boldfont.setPointSize(11);
        return boldfont;
      } else {
        QFont regularfont;
        regularfont.setPointSize(10);
        return regularfont;
      }
    } else if (role == Qt::DisplayRole) {
      if (item.metadata.setting_type == Gamma::SettingType::integer)
        return QVariant::fromValue(item.value_int);
      else if (item.metadata.setting_type == Gamma::SettingType::binary) {
        QString hex = QString::number(item.value_int, 16).toUpper();
        int size = item.metadata.maximum;
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
      } else if (item.metadata.setting_type == Gamma::SettingType::floating) {
        double val = item.value_dbl;
        if (preferred_units_.count(item.id_) && (item.metadata.unit != preferred_units_.at(item.id_))) {
          UnitConverter uc;
          val = uc.convert_units(val, item.metadata.unit, preferred_units_.at(item.id_)).convert_to<double>();
        }
        return QVariant::fromValue(val);
      } else if (item.metadata.setting_type == Gamma::SettingType::int_menu)
        if (item.metadata.int_menu_items.count(item.value_int) > 0)
          return QString::fromStdString(item.metadata.int_menu_items.at(item.value_int));
        else
          return QVariant();
      else if (item.metadata.setting_type == Gamma::SettingType::indicator) {
              if (item.metadata.int_menu_items.count(item.value_int) > 0)
                return QString::fromStdString(
                      item.get_setting(Gamma::Setting(item.metadata.int_menu_items.at(item.value_int)), Gamma::Match::id).metadata.name
                      );
              else
                return QVariant();
      }
      else if (item.metadata.setting_type == Gamma::SettingType::boolean)
        if (item.value_int)
          return "T";
        else
          return "F";
      else if (item.metadata.setting_type == Gamma::SettingType::text)
        return QString::fromStdString(item.value_text);
      else if (item.metadata.setting_type == Gamma::SettingType::command)
        return QVariant::fromValue(item);
      else if (item.metadata.setting_type == Gamma::SettingType::file_path)
        return QString::fromStdString(item.value_text);
      else if (item.metadata.setting_type == Gamma::SettingType::dir_path)
        return QString::fromStdString(item.value_text);
      else if (item.metadata.setting_type == Gamma::SettingType::detector)
        return QString::fromStdString(item.value_text);
      else
        return QVariant::fromValue(item.value_dbl);
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
      else if (section == (channels_.size()+1))
        return "Units";
      else if (section == (channels_.size()+2))
        return "Description";
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
        if (q.metadata.writable)
          channels_[i].settings_.branches.add(q);
      }
    }
  }

  consolidated_list_ = Gamma::Setting();
  for (auto &q : channels_) {
    for (auto &p : q.settings_.branches.my_data_) {
      //p.index = 0;
      consolidated_list_.branches.add(p);
      if (!preferred_units_.count(p.id_) && (!p.metadata.unit.empty())) {
//        PL_DBG << "adding preferred unit for " << p.id_ << " as " << p.metadata.unit;
        preferred_units_[p.id_] = p.metadata.unit;
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

  if ((col > 0) && (col <= (channels_.size() + 2)) && (!consolidated_list_.branches.empty())) {
    if (row == 0)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    Gamma::Setting item = consolidated_list_.branches.get(row-1);

    if (col == (channels_.size() + 1)) {
       if (preferred_units_.count(item.id_) && scalable_units_.count(UnitConverter().strip_unit(item.metadata.unit)))
         return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
       else
         return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    } else if (col == (channels_.size() + 2))
      return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);

    //item.index = col -1;
    item = channels_[col-1].settings_.get_setting(item, Gamma::Match::id);

    if (item == Gamma::Setting())
      return QAbstractTableModel::flags(index);
    else if (item.metadata.writable)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else if (item.metadata.setting_type == Gamma::SettingType::binary)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else
      return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
  }
}

bool TableChanSettings::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::EditRole)
    if ((row > 0) && (col <= (channels_.size()+2))) {
      Gamma::Setting item = consolidated_list_.branches.get(row-1);

      if (col == (channels_.size()+1)) {
        if (preferred_units_.count(item.id_)) {
          int idx = value.toInt();
          std::string prefix;
          UnitConverter uc;
          if (idx < uc.prefix_values_indexed.size())
            prefix = uc.prefix_values_indexed[idx];
          preferred_units_[item.id_] = prefix + uc.strip_unit(preferred_units_[item.id_]);
          QModelIndex start_ix = createIndex( row, 0 );
          QModelIndex end_ix = createIndex( row, columnCount() - 1);
          emit dataChanged( start_ix, end_ix );
          return true;
        } else
          return false;
      }

      //item.index = col -1;
      item = channels_[col-1].settings_.get_setting(item, Gamma::Match::id);
      if (item == Gamma::Setting())
        return false;

      if (((item.metadata.setting_type == Gamma::SettingType::integer) || (item.metadata.setting_type == Gamma::SettingType::int_menu) || (item.metadata.setting_type == Gamma::SettingType::binary))
          && (value.canConvert(QMetaType::LongLong)))
        item.value_int = value.toLongLong();
      else if ((item.metadata.setting_type == Gamma::SettingType::boolean)
          && (value.type() == QVariant::Bool))
        item.value_int = value.toBool();
      else if ((item.metadata.setting_type == Gamma::SettingType::floating)
          && (value.type() == QVariant::Double)) {
        double val = value.toDouble();
        if (preferred_units_.count(item.id_) && (item.metadata.unit != preferred_units_.at(item.id_))) {
              std::string to_units = preferred_units_.at(item.id_);
              UnitConverter uc;
              val = uc.convert_units(val, preferred_units_.at(item.id_), item.metadata.unit).convert_to<double>();
            }
        item.value_dbl = val;
      }
      else if (((item.metadata.setting_type == Gamma::SettingType::text)
                || (item.metadata.setting_type == Gamma::SettingType::file_path)
                || (item.metadata.setting_type == Gamma::SettingType::dir_path)
                || (item.metadata.setting_type == Gamma::SettingType::detector) )
          && (value.type() == QVariant::String))
        item.value_text = value.toString().toStdString();
      emit setting_changed(col-1, item);
      return true;
    } else {
      emit detector_chosen(col-1, value.toString().toStdString());
      return true;
    }
  return true;
}
