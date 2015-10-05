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
 *
 ******************************************************************************/

#include "table_spectrum_attrs.h"

Q_DECLARE_METATYPE(Gamma::Setting)

TableSpectrumAttrs::TableSpectrumAttrs(QObject *parent)
  : QAbstractTableModel(parent)
{
}

int TableSpectrumAttrs::rowCount(const QModelIndex & /*parent*/) const
{
  return generic_attributes->size();
}

int TableSpectrumAttrs::columnCount(const QModelIndex & /*parent*/) const
{
  return 4;
}

QVariant TableSpectrumAttrs::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(generic_attributes->get(row).id_);
    case 1:
    {
      Gamma::SettingType type = generic_attributes->get(row).metadata.setting_type;

      if (type == Gamma::SettingType::integer)
        return QVariant::fromValue(generic_attributes->get(row).value_int);
      else if (type == Gamma::SettingType::binary) {
        QString hex = QString::number(generic_attributes->get(row).value_int, 16).toUpper();
        int size = generic_attributes->get(row).metadata.maximum;
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
      }
      else if (type == Gamma::SettingType::floating)
        return QVariant::fromValue(generic_attributes->get(row).value_dbl);
      else if (type == Gamma::SettingType::int_menu) {
        if (generic_attributes->get(row).metadata.int_menu_items.count(generic_attributes->get(row).value_int) > 0)
          return QString::fromStdString(generic_attributes->get(row).metadata.int_menu_items.at(generic_attributes->get(row).value_int));
        else
          return QVariant();
      }
      else if (type == Gamma::SettingType::boolean)
        if (generic_attributes->get(row).value_int)
          return "T";
        else
          return "F";
      else if ((type == Gamma::SettingType::text)
               || (type == Gamma::SettingType::file_path)
               || (type == Gamma::SettingType::dir_path)
               || (type == Gamma::SettingType::detector))
        return QString::fromStdString(generic_attributes->get(row).value_text);
      else
        return QVariant::fromValue(generic_attributes->get(row));
    }
    case 2:
      return QString::fromStdString(generic_attributes->get(row).metadata.unit);
    case 3:
      return QString::fromStdString(generic_attributes->get(row).metadata.description);
    default:
      return QVariant();
    }
  }
  else if (role == Qt::EditRole)
  {
    if (col == 1)
      return QVariant::fromValue(generic_attributes->get(row));
  }
  return QVariant();
}

QVariant TableSpectrumAttrs::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("name");
      case 1:
        return QString("value");
      case 2:
        return QString("units");
      case 3:
        return QString("description");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableSpectrumAttrs::eat(XMLable2DB<Gamma::Setting> *generic_attr) {
  generic_attributes = generic_attr;
}

void TableSpectrumAttrs::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex(generic_attributes->size(), columnCount());
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableSpectrumAttrs::flags(const QModelIndex &index) const
{
  int col = index.column();

  if (col == 1)
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
  else
    return Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
}

bool TableSpectrumAttrs::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::EditRole)
    if (col == 1) {
      Gamma::Setting setting = generic_attributes->get(row);
      Gamma::SettingType type = setting.metadata.setting_type;

      if (((type == Gamma::SettingType::integer) || (type == Gamma::SettingType::int_menu) || (type == Gamma::SettingType::binary))
          && (value.canConvert(QMetaType::LongLong)))
        setting.value_int = value.toLongLong();
      else if ((type == Gamma::SettingType::boolean)
          && (value.type() == QVariant::Bool))
        setting.value_int = value.toBool();
      else if ((type == Gamma::SettingType::floating)
          && (value.type() == QVariant::Double))
        setting.value_dbl = value.toDouble();
      else if (((type == Gamma::SettingType::text)
                || (type == Gamma::SettingType::detector)
                || (type == Gamma::SettingType::dir_path)
                || (type == Gamma::SettingType::file_path))
          && (value.type() == QVariant::String))
        setting.value_text = value.toString().toStdString();
      else
        return false;
      generic_attributes->replace(setting);
      return true;
    }
  return false;
}

