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

Q_DECLARE_METATYPE(Pixie::Detector)
Q_DECLARE_METATYPE(Pixie::Setting)


/*

QVariant TableChanSettings::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
  {
    if (row == 0) {
      if (col == 0)
        return "<===detector===>";
      else if (col <= Pixie::kNumChans)
        return QString::fromStdString(my_settings_.get_detector(Pixie::Channel(col-1)).name_);
    }
  } else if (role == Qt::EditRole) {
    if (col <= Pixie::kNumChans) {
      if (row == 0)
        return QVariant::fromValue(my_settings_.get_detector(Pixie::Channel(col-1)));
    }
  } else if (role == Qt::ForegroundRole) {
    if (row == 0) {
      QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta};
      QBrush brush(palette[col % palette.size()]);
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
    else if (col == Pixie::kNumChans + 1) {
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
      else if (section <= Pixie::kNumChans)
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

bool TableChanSettings::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::EditRole)
    if (value.canConvert<Pixie::Detector>()) {
      my_settings_.set_detector(Pixie::Channel(col - 1), qvariant_cast<Pixie::Detector>(value));
      emit detectors_changed();
    }

  return true;
}
*/




TreeItem::TreeItem(const Pixie::Setting &data, TreeItem *parent)
{
  parentItem = parent;
  itemData = data;

  if (itemData.node_type == Pixie::NodeType::stem) {
    childItems.resize(itemData.branches.size());
    for (int i=0; i < itemData.branches.size(); ++i)
      childItems[i] = new TreeItem(itemData.branches.get(i), this);
  }
}

void TreeItem::eat_data(const Pixie::Setting &data) {
  itemData = data;

  if (itemData.node_type == Pixie::NodeType::stem) {
    if (itemData.branches.size() != childItems.size()) {
      PL_ERR << "Setting branch size mismatch";
      return;
    }
    for (int i=0; i < itemData.branches.size(); ++i)
      childItems[i]->eat_data(itemData.branches.get(i));
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
  if (parentItem)
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
  else if ((column == 1) && (itemData.node_type == Pixie::NodeType::setting))
  {
    if (itemData.setting_type == Pixie::SettingType::integer)
      if (itemData.unit == "binary")
        return QString::number(static_cast<uint32_t>(itemData.value_int, 16)).toUpper();
      else
        return QVariant::fromValue(itemData.value_int);
    else if (itemData.setting_type == Pixie::SettingType::floating)
      return QVariant::fromValue(itemData.value);
    else if (itemData.setting_type == Pixie::SettingType::boolean)
      if (itemData.value_int)
        return "T";
      else
        return "F";
    else if (itemData.setting_type == Pixie::SettingType::text)
      return QString::fromStdString(itemData.value_text);
    else
      return QVariant::fromValue(itemData.value);
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
  return ((column == 1) && (itemData.writable));
}




/*bool TreeItem::insertChildren(int position, int count, int columns)
{
  if (position < 0 || position > childItems.size())
    return false;

  std::string name = itemData.name;
  int start = itemData.branches.size();
  Pixie::SettingType type = Pixie::SettingType::integer;
  if (start) {
    name = itemData.branches.get(0).name;
    type = itemData.branches.get(0).setting_type;
  }
  for (int row = start; row < (start + count); ++row) {
    Pixie::Setting newsetting;
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

Pixie::Setting TreeItem::rebuild() {
  Pixie::Setting root = itemData;
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

  if ((itemData.setting_type == Pixie::SettingType::integer)
      && (value.type() == QVariant::Int))
    itemData.value_int = value.toInt();
  else if ((itemData.setting_type == Pixie::SettingType::boolean)
      && (value.type() == QVariant::Bool))
    itemData.value_int = value.toBool();
  else if ((itemData.setting_type == Pixie::SettingType::floating)
      && (value.type() == QVariant::Double))
    itemData.value = value.toDouble();
  else if ((itemData.setting_type == Pixie::SettingType::text)
      && (value.type() == QVariant::String))
    itemData.value_text = value.toString().toStdString();
  else
    return false;

/*    else if (value.canConvert<Pixie::Detector>()) {
      my_settings_.set_detector(Pixie::Channel(col - 1), qvariant_cast<Pixie::Detector>(value));
      emit detectors_changed();
    }*/

  //itemData[column] = value;
  return true;
}






TreeSettings::TreeSettings(const Pixie::Setting &data, QObject *parent)
  : QAbstractItemModel(parent)
{
  rootItem = new TreeItem(data);
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
    /*if (row == 0) {
      QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta};
      QBrush brush(palette[col % palette.size()]);
      return brush;
    } else */ if ((col == 1) && !item->is_editable(col)) {
      QBrush brush(Qt::darkGray);
      return brush;
    } else {
      QBrush brush(Qt::black);
      return brush;
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
      return "description";

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

  return parentItem->childCount();
}

bool TreeSettings::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (role != Qt::EditRole)
    return false;

  TreeItem *item = getItem(index);
  bool result = item->setData(index.column(), value);

  if (result) {
    data_ = rootItem->rebuild();

    emit dataChanged(index, index);
    emit tree_changed();
  }
  return result;
}

const Pixie::Setting & TreeSettings::get_tree() {
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


void TreeSettings::update(const Pixie::Setting &data) {
  data_ = data;
  rootItem->eat_data(data_);

  emit layoutChanged();
}
