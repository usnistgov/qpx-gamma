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

#include "tree_settings.h"
#include "widget_pattern.h"

Q_DECLARE_METATYPE(Gamma::Setting)

TreeItem::TreeItem(const Gamma::Setting &data, TreeItem *parent)
{
  parentItem = parent;
  itemData = data;

  if (itemData.setting_type == Gamma::SettingType::stem) {
    childItems.resize(itemData.branches.size());
    for (int i=0; i < itemData.branches.size(); ++i)
      childItems[i] = new TreeItem(itemData.branches.get(i), this);
  }
}

void TreeItem::eat_data(const Gamma::Setting &data) {
  itemData = data;

  if (itemData.setting_type == Gamma::SettingType::stem) {
    if (itemData.branches.size() != childItems.size()) {
      PL_WARN << "Setting branch size mismatch";
      for (int i=0; i <  childItems.size(); ++i)
        delete childItems.takeAt(i);

      //qDeleteAll(childItems);
      childItems.resize(itemData.branches.size());
      for (int i=0; i < itemData.branches.size(); ++i)
        childItems[i] = new TreeItem(itemData.branches.get(i), this);
    } else {
      for (int i=0; i < itemData.branches.size(); ++i)
        childItems[i]->eat_data(itemData.branches.get(i));
    }
  }
}

TreeItem::~TreeItem()
{
  qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
  return childItems.value(number);
}

int TreeItem::childCount() const
{
  return childItems.count();
}

int TreeItem::childNumber() const
{
  if (parentItem && !parentItem->childItems.empty())
    return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

  return 0;
}

int TreeItem::columnCount() const
{
  return 4; //name, value, units, description
}

QVariant TreeItem::display_data(int column) const
{
  if (column == 0)
    return QString::fromStdString(itemData.name);
  else if ((column == 1) && (itemData.setting_type != Gamma::SettingType::none) && (itemData.setting_type != Gamma::SettingType::stem))
  {
    if (itemData.setting_type == Gamma::SettingType::integer)
      return QVariant::fromValue(itemData.value_int);
    else if (itemData.setting_type == Gamma::SettingType::binary) {
      QString hex = QString::number(itemData.value_int, 16).toUpper();
      int size = itemData.maximum;
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
    } else if (itemData.setting_type == Gamma::SettingType::floating)
      return QVariant::fromValue(itemData.value);
    else if (itemData.setting_type == Gamma::SettingType::int_menu) {
      if (itemData.int_menu_items.count(itemData.value_int) > 0)
        return QString::fromStdString(itemData.int_menu_items.at(itemData.value_int));
      else
        return QVariant();
    }
    else if (itemData.setting_type == Gamma::SettingType::boolean)
      if (itemData.value_int)
        return "T";
      else
        return "F";
    else if (itemData.setting_type == Gamma::SettingType::pattern) {
      std::bitset<64> set(itemData.value_int);
      QVector<int16_t> pat(itemData.maximum);
      for (int i=0; i < itemData.maximum; ++i)
          pat[i] = set[i];
      QpxPattern pattern(pat, 20, false, 8);
      return QVariant::fromValue(pattern);
    }
    else if (itemData.setting_type == Gamma::SettingType::text)
      return QString::fromStdString(itemData.value_text);
    else if (itemData.setting_type == Gamma::SettingType::file_path)
      return QString::fromStdString(itemData.value_text);
    else if (itemData.setting_type == Gamma::SettingType::detector)
      return QString::fromStdString(itemData.value_text);
    else if (itemData.setting_type == Gamma::SettingType::command) {
      if (itemData.value_int == 0)
        return QString("RESERVED");
      else if (itemData.value_int == 1)
        return QString("NOT NOW");
      else if (itemData.value_int > 1)
        return QString("EXECUTE");
    }
    else
      return QVariant::fromValue(itemData.value);
  }
  else if (column == 2)
    return QString::fromStdString(itemData.unit);
  else if (column == 3)
    return QString::fromStdString(itemData.description);
  else
    return QVariant();
}


QVariant TreeItem::edit_data(int column) const
{
  if (column == 1)
    return QVariant::fromValue(itemData);
  else
    return QVariant();
}

bool TreeItem::is_editable(int column) const
{
  if ((column == 1)  && (itemData.setting_type == Gamma::SettingType::command) && (itemData.value_int < 2))
    return false;
  else
    return ((column == 1) && (itemData.writable));
}




/*bool TreeItem::insertChildren(int position, int count, int columns)
{
  if (position < 0 || position > childItems.size())
    return false;

  std::string name = itemData.name;
  int start = itemData.branches.size();
  Gamma::SettingType type = Gamma::SettingType::integer;
  if (start) {
    name = itemData.branches.get(0).name;
    type = itemData.branches.get(0).setting_type;
  }
  for (int row = start; row < (start + count); ++row) {
    Gamma::Setting newsetting;
    newsetting.index = row;
    newsetting.name = name;
    newsetting.setting_type = type;
    itemData.branches.add(newsetting);
    TreeItem *item = new TreeItem(newsetting, this);
    childItems.insert(position, item);
  }

  return true;
}

*/

TreeItem *TreeItem::parent()
{
  return parentItem;
}

Gamma::Setting TreeItem::rebuild() {
  Gamma::Setting root = itemData;
  root.branches.clear();
  for (int i=0; i < childCount(); i++)
    root.branches.add(child(i)->rebuild());
  return root;
}

/*bool TreeItem::removeChildren(int position, int count)
{
  if (position < 0 || position + count > childItems.size())
    return false;

  for (int row = 0; row < count; ++row)
    delete childItems.takeAt(position);

  return true;
}

*/

bool TreeItem::setData(int column, const QVariant &value)
{
  if (column != 1)
    return false;


  if (((itemData.setting_type == Gamma::SettingType::integer) ||
       (itemData.setting_type == Gamma::SettingType::binary) ||
       (itemData.setting_type == Gamma::SettingType::int_menu))
      && (value.canConvert(QMetaType::LongLong)))
    itemData.value_int = value.toLongLong();
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
  else if (itemData.setting_type == Gamma::SettingType::command)
    itemData.value = 1; //flag it for execution
  else if ((itemData.setting_type == Gamma::SettingType::detector)
      && (value.type() == QVariant::String)) {
    itemData.value_text = value.toString().toStdString();
  }
  else
    return false;

  return true;
}






TreeSettings::TreeSettings(QObject *parent)
  : QAbstractItemModel(parent)
{
  rootItem = new TreeItem(Gamma::Setting());
}

TreeSettings::~TreeSettings()
{
  delete rootItem;
}

int TreeSettings::columnCount(const QModelIndex & /* parent */) const
{
  return rootItem->columnCount();
}

QVariant TreeSettings::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  TreeItem *item = getItem(index);

  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
    return item->display_data(col);
  else if (role == Qt::EditRole)
    return item->edit_data(col);
  else if (role == Qt::ForegroundRole) {
    if (item->edit_data(col).canConvert<Gamma::Setting>()) {
      Gamma::Setting set = qvariant_cast<Gamma::Setting>(item->edit_data(col));
      if (set.setting_type == Gamma::SettingType::detector) {
        QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};
        QBrush brush(palette[set.index % palette.size()]);
        return brush;
      } else if (set.setting_type == Gamma::SettingType::command) {
        if (set.value_int == 0) {
          QBrush brush(Qt::darkGray);
          return brush;
        } else if (set.value_int == 1) {
          QBrush brush(Qt::darkRed);
          return brush;
        } else if (set.value_int == 2) {
          QBrush brush(Qt::darkGreen);
          return brush;
        }
      } else if ((col == 1) && (!set.writable)) {
        QBrush brush(Qt::darkGray);
        return brush;
      } else {
      QBrush brush(Qt::black);
      return brush;
      }
    }
  }
  else
    return QVariant();
}

Qt::ItemFlags TreeSettings::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;

  TreeItem *item = getItem(index);
  if (item->is_editable(index.column()))
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
  else
    return QAbstractItemModel::flags(index);
}

TreeItem *TreeSettings::getItem(const QModelIndex &index) const
{
  if (index.isValid()) {
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if (item)
      return item;
  }
  return rootItem;
}

QVariant TreeSettings::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    if (section == 0)
      return "setting";
    else if (section == 1)
      return "value";
    else if (section == 2)
      return "units";
    else if (section == 3)
      return "notes";

  return QVariant();
}

QModelIndex TreeSettings::index(int row, int column, const QModelIndex &parent) const
{
  if (parent.isValid() && parent.column() != 0)
    return QModelIndex();

  TreeItem *parentItem = getItem(parent);

  TreeItem *childItem = parentItem->child(row);
  if (childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

/*bool TreeSettings::insertRows(int position, int rows, const QModelIndex &parent)
{
  TreeItem *parentItem = getItem(parent);
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success = parentItem->insertChildren(position, rows, rootItem->columnCount());
  endInsertRows();

  return success;
}*/

QModelIndex TreeSettings::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  TreeItem *childItem = getItem(index);
  TreeItem *parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  return createIndex(parentItem->childNumber(), 0, parentItem);
}

/*bool TreeSettings::removeRows(int position, int rows, const QModelIndex &parent)
{
  TreeItem *parentItem = getItem(parent);
  bool success = true;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}*/

int TreeSettings::rowCount(const QModelIndex &parent) const
{
  TreeItem *parentItem = getItem(parent);

  if (parentItem != nullptr)
    return parentItem->childCount();
  else
    return 0;
}

bool TreeSettings::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (role != Qt::EditRole)
    return false;

  TreeItem *item = getItem(index);
  bool result = item->setData(index.column(), value);

  if (result) {
    Gamma::Setting set = qvariant_cast<Gamma::Setting>(index.data(Qt::EditRole));
    data_ = rootItem->rebuild();

    emit dataChanged(index, index);
    if (set.setting_type == Gamma::SettingType::detector)
      emit detector_chosen(set.index, set.value_text);
    if (set.setting_type == Gamma::SettingType::command)
      emit execute_command();
    else
      emit tree_changed();
  }
  return result;
}

const Gamma::Setting & TreeSettings::get_tree() {
  return data_;
}

bool TreeSettings::setHeaderData(int section, Qt::Orientation orientation,
                                 const QVariant &value, int role)
{
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
    return false;

  bool result = rootItem->setData(section, value);

  if (result) {
    emit headerDataChanged(orientation, section, section);
    //emit push settings to device
  }

  return result;
}


void TreeSettings::update(const Gamma::Setting &data) {
  data_ = data;
  rootItem->eat_data(data_);

  emit layoutChanged();
}
