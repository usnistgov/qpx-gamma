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
#include <QDateTime>
#include "qt_util.h"

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
//  if ((channels_.size() > 0) && (!consolidated_list_.branches.empty()))
    return consolidated_list_.branches.size() + 1;
//  else
//    return 0;
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

  if (role == Qt::ForegroundRole)
  {
    if (row == 0) {
      if (col == 0) {
        QBrush brush(Qt::black);
        return brush;
      }
    } else if ((col <= static_cast<int>(channels_.size()+2)) && (!channels_.empty()) && (!consolidated_list_.branches.empty())) {
      Qpx::Setting item = consolidated_list_.branches.get(row-1);
      if (item.metadata.writable) {
        QBrush brush(Qt::black);
        return brush;
      } else {
        QBrush brush(Qt::darkGray);
        return brush;
      }
    }
    else
      return QVariant();
  }

  else if (role == Qt::FontRole)
  {
    QFont font;
    font.setPointSize(10);
    if ((col == 0) && (row != 0))
      font.setBold(true);
    else if (col > static_cast<int>(channels_.size()))
      font.setItalic(true);
    return font;
  }


  else if (role == Qt::DisplayRole)
  {
    if (row == 0) {
      if (col == 0) {
        return "<===detector===>";
      } else if (col <= static_cast<int>(channels_.size())) {
        Qpx::Setting det;
        det.metadata.setting_type = Qpx::SettingType::detector;
        det.value_text = channels_[col-1].name_;
        det.indices.insert(col-1);
        return QVariant::fromValue(det);
      } else
        return QVariant();
    } else {
      Qpx::Setting item = consolidated_list_.branches.get(row-1);
      if (col == 0)
        return QString::fromStdString(item.id_);
      else if ((col == static_cast<int>(channels_.size()+1)) && preferred_units_.count(item.id_))
        return QString::fromStdString(preferred_units_.at(item.id_));
      else if (col == static_cast<int>(channels_.size()+2))
        return QString::fromStdString(item.metadata.description);
      else if (col <= static_cast<int>(channels_.size())) {
        item = channels_[col-1].settings_.get_setting(item, Qpx::Match::id);
        if (item != Qpx::Setting()) {
          if (item.metadata.setting_type == Qpx::SettingType::floating) {
            double val = item.value_dbl;
            if (preferred_units_.count(item.id_) && (item.metadata.unit != preferred_units_.at(item.id_))) {
              UnitConverter uc;
              val = to_double( uc.convert_units(val, item.metadata.unit, preferred_units_.at(item.id_)) );
            }
            return QVariant::fromValue(val);
          } else
            return QVariant::fromValue(item);
        }
      } else
        return QVariant();
    }
  }

  else if (role == Qt::EditRole) {
    if ((row == 0) && (col > 0) && (col <= static_cast<int>(channels_.size()))) {
      Qpx::Setting det;
      det.metadata.setting_type = Qpx::SettingType::detector;
      det.value_text = channels_[col-1].name_;
      return QVariant::fromValue(det);
    } else if (row != 0) {
      Qpx::Setting item = consolidated_list_.branches.get(row-1);
      if ((col == static_cast<int>(channels_.size()+1)) && preferred_units_.count(item.id_)) {
        UnitConverter uc;
        Qpx::Setting st("unit");
        st.metadata.setting_type = Qpx::SettingType::int_menu;
        if (item.metadata.minimum)
          st.metadata.int_menu_items = uc.make_ordered_map(item.metadata.unit, item.metadata.minimum, item.metadata.maximum);
        else
          st.metadata.int_menu_items = uc.make_ordered_map(item.metadata.unit, item.metadata.step, item.metadata.maximum);
        for (auto &q : st.metadata.int_menu_items)
          if (q.second == preferred_units_.at(item.id_))
            st.value_int = q.first;
        return QVariant::fromValue(st);
      } else if (col <= static_cast<int>(channels_.size())) {
        item = channels_[col-1].settings_.get_setting(item, Qpx::Match::id);
        if (item == Qpx::Setting())
          return QVariant();
        if ((item.metadata.setting_type == Qpx::SettingType::floating)
            && preferred_units_.count(item.id_)
            && (item.metadata.unit != preferred_units_.at(item.id_))) {
          std::string to_units = preferred_units_.at(item.id_);
          UnitConverter uc;
          item.value_dbl = to_double( uc.convert_units(item.value_dbl, item.metadata.unit, to_units) );
          item.metadata.minimum = to_double( uc.convert_units(item.metadata.minimum, item.metadata.unit, to_units) );
          item.metadata.step = to_double( uc.convert_units(item.metadata.step, item.metadata.unit, to_units) );
          item.metadata.maximum = to_double( uc.convert_units(item.metadata.maximum, item.metadata.unit, to_units) );
        }
        return QVariant::fromValue(item);
      } else
        return QVariant();
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
      else if (section <= static_cast<int>(channels_.size()))
        return (QString("chan ") + QString::number(section-1));
      else if (section == static_cast<int>(channels_.size()+1))
        return "Units";
      else if (section == static_cast<int>(channels_.size()+2))
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

void TableChanSettings::update(const std::vector<Qpx::Detector> &settings) {
  channels_ = settings;
  if (!show_read_only_) {
    for (size_t i=0; i < settings.size(); ++i) {
      channels_[i].settings_.branches.clear();
      for (auto &q : settings[i].settings_.branches.my_data_) {
        if (q.metadata.writable)
          channels_[i].settings_.branches.add(q);
      }
    }
  }

  consolidated_list_ = Qpx::Setting();
  for (auto &q : channels_) {
    for (auto &p : q.settings_.branches.my_data_) {
      consolidated_list_.branches.add(p);
      if (!preferred_units_.count(p.id_) && (!p.metadata.unit.empty())) {
        //        DBG << "adding preferred unit for " << p.id_ << " as " << p.metadata.unit;
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

  if ((col > 0) && (col <= static_cast<int>(channels_.size() + 2)) && (!consolidated_list_.branches.empty())) {
    if ((row == 0) && (col <= static_cast<int>(channels_.size())))
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);

    Qpx::Setting item = consolidated_list_.branches.get(row-1);

    if (col == static_cast<int>(channels_.size() + 1)) {
      if (preferred_units_.count(item.id_) && scalable_units_.count(UnitConverter().strip_unit(item.metadata.unit)))
        return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
      else
        return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    } else if (col == static_cast<int>(channels_.size() + 2))
      return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);

    item = channels_[col-1].settings_.get_setting(item, Qpx::Match::id);

    if (item == Qpx::Setting())
      return QAbstractTableModel::flags(index);
    else if (item.metadata.writable)
      return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else if (item.metadata.setting_type == Qpx::SettingType::binary)
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
    if ((row > 0) && (col <= static_cast<int>(channels_.size()+2))) {
      Qpx::Setting item = consolidated_list_.branches.get(row-1);

      if (col == static_cast<int>(channels_.size()+1)) {
        if (preferred_units_.count(item.id_)) {
          int idx = value.toInt();
          std::string prefix;
          UnitConverter uc;
          if (idx < static_cast<int>(uc.prefix_values_indexed.size()))
            prefix = uc.prefix_values_indexed[idx];
          preferred_units_[item.id_] = prefix + uc.strip_unit(preferred_units_[item.id_]);
          QModelIndex start_ix = createIndex( row, 0 );
          QModelIndex end_ix = createIndex( row, columnCount() - 1);
          emit dataChanged( start_ix, end_ix );
          return true;
        } else
          return false;
      }

      item = channels_[col-1].settings_.get_setting(item, Qpx::Match::id);
      if (item == Qpx::Setting())
        return false;

      if (((item.metadata.setting_type == Qpx::SettingType::integer)
           || (item.metadata.setting_type == Qpx::SettingType::int_menu)
           || (item.metadata.setting_type == Qpx::SettingType::binary)
           || (item.metadata.setting_type == Qpx::SettingType::command))
          && (value.canConvert(QMetaType::LongLong)))
        item.value_int = value.toLongLong();
      else if ((item.metadata.setting_type == Qpx::SettingType::boolean)
               && (value.type() == QVariant::Bool))
        item.value_int = value.toBool();
      else if ((item.metadata.setting_type == Qpx::SettingType::floating)
               && (value.type() == QVariant::Double)) {
        double val = value.toDouble();
        if (preferred_units_.count(item.id_) && (item.metadata.unit != preferred_units_.at(item.id_))) {
          std::string to_units = preferred_units_.at(item.id_);
          UnitConverter uc;
          val = to_double( uc.convert_units(val, preferred_units_.at(item.id_), item.metadata.unit) );
        }
        item.value_dbl = val;
      }
      else if ((item.metadata.setting_type == Qpx::SettingType::floating_precise)
               && (value.type() == QVariant::Double)) {
        long double val = value.toDouble();
        if (preferred_units_.count(item.id_) && (item.metadata.unit != preferred_units_.at(item.id_))) {
          std::string to_units = preferred_units_.at(item.id_);
          UnitConverter uc;
          val = to_double( uc.convert_units(val, preferred_units_.at(item.id_), item.metadata.unit) );
        }
        item.value_precise = val;
      }
      else if (((item.metadata.setting_type == Qpx::SettingType::text)
                || (item.metadata.setting_type == Qpx::SettingType::file_path)
                || (item.metadata.setting_type == Qpx::SettingType::dir_path)
                || (item.metadata.setting_type == Qpx::SettingType::detector) )
               && (value.type() == QVariant::String))
        item.value_text = value.toString().toStdString();
      else if ((item.metadata.setting_type == Qpx::SettingType::time)
               && (value.type() == QVariant::DateTime))
        item.value_time = fromQDateTime(value.toDateTime());
      else if ((item.metadata.setting_type == Qpx::SettingType::time_duration)
               && (value.canConvert(QMetaType::LongLong)))
        item.value_duration = boost::posix_time::seconds(value.toLongLong());

      emit setting_changed(col-1, item);
      return true;
    } else {
      emit detector_chosen(col-1, value.toString().toStdString());
      return true;
    }
  return true;
}
