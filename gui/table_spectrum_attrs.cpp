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
      return QString::fromStdString((*generic_attributes)[row].name);
    case 1:
      if ((*generic_attributes)[row].setting_type == Gamma::SettingType::integer)
        if ((*generic_attributes)[row].unit == "binary")
          return QString::number(static_cast<uint32_t>((*generic_attributes)[row].value_int, 16)).toUpper();
        else
          return QVariant::fromValue((*generic_attributes)[row].value_int);
      else if ((*generic_attributes)[row].setting_type == Gamma::SettingType::floating)
        return QVariant::fromValue((*generic_attributes)[row].value);
      else if ((*generic_attributes)[row].setting_type == Gamma::SettingType::int_menu)
        return QString::fromStdString((*generic_attributes)[row].int_menu_items.at((*generic_attributes)[row].value_int));
      else if ((*generic_attributes)[row].setting_type == Gamma::SettingType::boolean)
        if ((*generic_attributes)[row].value_int)
          return "T";
        else
          return "F";
      else if ((*generic_attributes)[row].setting_type == Gamma::SettingType::text)
        return QString::fromStdString((*generic_attributes)[row].value_text);
      else if ((*generic_attributes)[row].setting_type == Gamma::SettingType::detector)
        return QString::fromStdString((*generic_attributes)[row].value_text);
      else
        return QVariant::fromValue((*generic_attributes)[row].value);
      return QVariant::fromValue((*generic_attributes)[row].value);
      return QVariant::fromValue((*generic_attributes)[row]);
    case 2:
      return QString::fromStdString((*generic_attributes)[row].unit);
    case 3:
      return QString::fromStdString((*generic_attributes)[row].description);
    }
  }
  else if (role == Qt::EditRole)
  {
    if (col == 1)
      return QVariant::fromValue((*generic_attributes)[row]);
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

void TableSpectrumAttrs::eat(std::vector<Gamma::Setting> *generic_attr) {
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
    if (col == 1)
      if ((((*generic_attributes)[row].setting_type == Gamma::SettingType::integer) || ((*generic_attributes)[row].setting_type == Gamma::SettingType::int_menu))
          && (value.type() == QVariant::Int))
        (*generic_attributes)[row].value_int = value.toInt();
      else if (((*generic_attributes)[row].setting_type == Gamma::SettingType::boolean)
          && (value.type() == QVariant::Bool))
        (*generic_attributes)[row].value_int = value.toBool();
      else if (((*generic_attributes)[row].setting_type == Gamma::SettingType::floating)
          && (value.type() == QVariant::Double))
        (*generic_attributes)[row].value = value.toDouble();
      else if (((*generic_attributes)[row].setting_type == Gamma::SettingType::text)
          && (value.type() == QVariant::String))
        (*generic_attributes)[row].value_text = value.toString().toStdString();
      else if (((*generic_attributes)[row].setting_type == Gamma::SettingType::int_menu)
          && (value.type() == QVariant::Int))
        (*generic_attributes)[row].value_int = value.toInt();
      else if (((*generic_attributes)[row].setting_type == Gamma::SettingType::detector)
          && (value.type() == QVariant::String))
        (*generic_attributes)[row].value_text = value.toString().toStdString();
      else
        return false;

  return true;
}

