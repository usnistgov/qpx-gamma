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
 *      TableChanSettings - tree for displaying and manipulating
 *      channel settings and chosing detectors.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "tree_settings.h"
#include "widget_pattern.h"
#include "qpx_util.h"
#include "qt_util.h"
#include <QDateTime>

Q_DECLARE_METATYPE(Qpx::Setting)
Q_DECLARE_METATYPE(boost::posix_time::time_duration)

TreeItem::TreeItem(const Qpx::Setting &data, TreeItem *parent)
{
  parentItem = parent;
  itemData = data;

  if (itemData.metadata.setting_type == Qpx::SettingType::stem) {
    childItems.resize(itemData.branches.size());
    for (size_t i=0; i < itemData.branches.size(); ++i)
      childItems[i] = new TreeItem(itemData.branches.get(i), this);
  }
}

bool TreeItem::eat_data(const Qpx::Setting &data) {
  itemData = data;

  if (itemData.metadata.setting_type == Qpx::SettingType::stem) {
    if (static_cast<int>(itemData.branches.size()) != childItems.size())
      return false;
    for (size_t i=0; i < itemData.branches.size(); ++i) {
      Qpx::Setting s = itemData.branches.get(i);
      if (childItems[i]->itemData.id_ != s.id_)
        return false;
      if (!childItems[i]->eat_data(itemData.branches.get(i)))
        return false;
    }
  }
  return (itemData.metadata.setting_type != Qpx::SettingType::none);

  /*if (itemData.branches.size() != childItems.size()) {
      WARN << "Setting branch size mismatch " << itemData.id_;
//      for (int i=0; i <  childItems.size(); ++i)
//        delete childItems.takeAt(i);

      qDeleteAll(childItems);
      childItems.resize(itemData.branches.size());
      for (int i=0; i < itemData.branches.size(); ++i)
        childItems[i] = new TreeItem(itemData.branches.get(i), this);
    } else {
      for (int i=0; i < itemData.branches.size(); ++i) {
        Qpx::Setting s = itemData.branches.get(i);
        if (childItems[i]->itemData.id_ == s.id_)
          childItems[i]->eat_data(itemData.branches.get(i));
        else {
          delete childItems.takeAt(i);
          childItems[i] = new TreeItem(itemData.branches.get(i), this);
        }
      }
    }*/
}

TreeItem::~TreeItem()
{
  parentItem == nullptr;
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
  return 6; //name, indices, value, units, address, description
}

QVariant TreeItem::display_data(int column) const
{
  if (column == 0) {
    QString name;
    if (!itemData.metadata.name.empty())
      name = QString::fromStdString(itemData.metadata.name);
    else
      name = QString::fromStdString(itemData.id_);
    return name;
  }
  else if (column == 1) {
    QString ret = QString::fromStdString(itemData.indices_to_string(true));
    if (ret.size())
      return "  " + ret + "  ";
    else
      return QVariant();
  }
  else if ((column == 2) && (itemData.metadata.setting_type != Qpx::SettingType::none) && (itemData.metadata.setting_type != Qpx::SettingType::stem))
  {
    if (itemData.metadata.setting_type == Qpx::SettingType::color)
      return QColor(QString::fromStdString(itemData.value_text));
    else
      return QVariant::fromValue(itemData);
  }
  else if (column == 3)
    return QString::fromStdString(itemData.metadata.unit);
  else if (column == 4) {
    QString text;
    if (itemData.metadata.address >= 0)
      text +=  "0x" + QString::fromStdString(itohex32(itemData.metadata.address));
    else if (itemData.metadata.address != -1)
      text += QString::number(itemData.metadata.address);
    return text;
  }
  else if (column == 5)
    return QString::fromStdString(itemData.metadata.description);
  else
    return QVariant();
}


QVariant TreeItem::edit_data(int column) const
{
  if (column == 2)
    return QVariant::fromValue(itemData);
  else if ((column == 1) && (itemData.metadata.max_indices > 0)) {
    QList<QVariant> qlist;
    qlist.push_back(QVariant::fromValue(itemData.metadata.max_indices));
    for (auto &q : itemData.indices) {
      qlist.push_back(q);
    }
    return qlist;
  } else
    return QVariant();
}

bool TreeItem::is_editable(int column) const
{
  if ((column == 1)  && (itemData.metadata.max_indices > 0))
    return true;
  else if ((column != 2)  || (itemData.metadata.setting_type == Qpx::SettingType::stem))
    return false;
  else if (itemData.metadata.setting_type == Qpx::SettingType::binary)
    return true;
  else
    return ((column == 2) && (itemData.metadata.writable));
}




/*bool TreeItem::insertChildren(int position, int count, int columns)
{
  if (position < 0 || position > childItems.size())
    return false;

  std::string name = itemData.name;
  int start = itemData.branches.size();
  Qpx::SettingType type = Qpx::SettingType::integer;
  if (start) {
    name = itemData.branches.get(0).name;
    type = itemData.branches.get(0).metadata.setting_type;
  }
  for (int row = start; row < (start + count); ++row) {
    Qpx::Setting newsetting;
    newsetting.name = name;
    newsetting.metadata.setting_type = type;
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

Qpx::Setting TreeItem::rebuild() {
  Qpx::Setting root = itemData;

  root.branches.clear();
  for (int i=0; i < childCount(); i++)
    root.branches.add_a(child(i)->rebuild());
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
  if ((column == 1)  && (itemData.metadata.max_indices > 0)) {
    QString val = value.toString();
    QStringList ilist = val.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    if (ilist.isEmpty()) {
      itemData.indices.clear();
      return true;
    }

    std::set<int32_t> new_indices;
    foreach (QString idx, ilist) {
      bool ok;
      int i = idx.toInt(&ok);
      if (ok && (i >= 0) && (static_cast<int>(new_indices.size()) < itemData.metadata.max_indices))
        new_indices.insert(i);
    }

    bool diff = (new_indices.size() != itemData.indices.size());
    for (auto &q : new_indices)
      if (!itemData.indices.count(q))
        diff = true;

    if (!diff)
      return false;

    itemData.indices = new_indices;
    return true;
  }

  if (column != 2)
    return false;


  if (((itemData.metadata.setting_type == Qpx::SettingType::integer) ||
       (itemData.metadata.setting_type == Qpx::SettingType::binary) ||
       (itemData.metadata.setting_type == Qpx::SettingType::int_menu) ||
       (itemData.metadata.setting_type == Qpx::SettingType::command))
      && (value.canConvert(QMetaType::LongLong))) {
    //DBG << "int = " << value.toLongLong();
    itemData.value_int = value.toLongLong();
  }
  else if ((itemData.metadata.setting_type == Qpx::SettingType::boolean)
      && (value.type() == QVariant::Bool))
    itemData.value_int = value.toBool();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::floating)
      && (value.type() == QVariant::Double))
    itemData.value_dbl = value.toDouble();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::floating_precise)
      && (value.type() == QVariant::Double))
    itemData.value_precise = value.toDouble();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::text)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::color)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::file_path)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::pattern)
      && (value.canConvert<Qpx::Pattern>())) {
    Qpx::Pattern qpxPattern = qvariant_cast<Qpx::Pattern>(value);
    itemData.value_pattern = qpxPattern;
  }
  else if ((itemData.metadata.setting_type == Qpx::SettingType::dir_path)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::detector)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else if ((itemData.metadata.setting_type == Qpx::SettingType::time)
      && (value.type() == QVariant::DateTime))
    itemData.value_time = fromQDateTime(value.toDateTime());
  else if ((itemData.metadata.setting_type == Qpx::SettingType::time_duration)
      && (value.canConvert<boost::posix_time::time_duration>()))
    itemData.value_duration = qvariant_cast<boost::posix_time::time_duration>(value);
  else
    return false;

  return true;
}






TreeSettings::TreeSettings(QObject *parent)
  : QAbstractItemModel(parent), show_address_(true)
{
  rootItem = new TreeItem(Qpx::Setting());
  show_read_only_ = true;
  edit_read_only_ = false;
}

TreeSettings::~TreeSettings()
{
  delete rootItem;
}

void TreeSettings::set_edit_read_only(bool edit_ro) {
  edit_read_only_ = edit_ro;
  emit layoutChanged();
}

void TreeSettings::set_show_read_only(bool show_ro) {
  show_read_only_ = show_ro;
}

void TreeSettings::set_show_address_(bool show_ad) {
  show_address_ = show_ad;
  emit layoutChanged();
}

int TreeSettings::columnCount(const QModelIndex & /* parent */) const
{
  if (show_address_)
    return rootItem->columnCount();
  else
    return rootItem->columnCount() - 1;
}

QVariant TreeSettings::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  TreeItem *item = getItem(index);

  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole) {
    if (!show_address_ && (col == 4))
      col = 5;
    return item->display_data(col);
  }
  else if (role == Qt::EditRole)
    return item->edit_data(col);
  else if (role == Qt::ForegroundRole) {
    if (item->edit_data(2).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(item->edit_data(2));
      if ((col == 1) && (set.metadata.max_indices < 1)) {
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
  if (edit_read_only_ && (index.column() == 2))
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
  else if (item->is_editable(index.column()))
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
      return "indices";
    else if (section == 2)
      return "value";
    else if (section == 3)
      return "units";
    else if (section == 4)
      if (show_address_)
        return "address";
      else
        return "notes";
    else if (section == 5)
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

  if (!childItem)
    return QModelIndex();

  if (childItem == 0x0)
    return QModelIndex();

  TreeItem *parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  if (!parentItem)
    return QModelIndex();

  if (parentItem == 0x0)
    return QModelIndex();

  //DBG << parentItem;

  //DBG << "Index r" << index.row() << " c" << index.column() << " d=";
//  if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
//    Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
//    DBG << "id=" << set.id_;
//  }

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
    if (index.column() == 2) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      data_ = rootItem->rebuild();

      emit dataChanged(index, index);
      if (set.metadata.setting_type == Qpx::SettingType::detector)
        emit detector_chosen(index.row() - 1, set.value_text);
      else
        emit tree_changed();
    } else if (index.column() == 1) {
      data_ = rootItem->rebuild();
      emit dataChanged(index, index);
      emit tree_changed();
    }
  }
  return result;
}

const Qpx::Setting & TreeSettings::get_tree() {
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


void TreeSettings::update(const Qpx::Setting &data) {
  data_ = data;
  data_.cull_invisible();
  if (!show_read_only_) {
    //DBG << "Culling read only";
    data_.cull_readonly();
  }

  if (!rootItem->eat_data(data_)) {
//    DBG << "deleting root node";
    beginResetModel();
    delete rootItem;
    rootItem = new TreeItem(data_);
    endResetModel();
  }

  emit layoutChanged();
}
