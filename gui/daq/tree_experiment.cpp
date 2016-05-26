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

#include "tree_experiment.h"
#include "widget_pattern.h"
#include "qpx_util.h"
#include "qt_util.h"
#include <QDateTime>

Q_DECLARE_METATYPE(Qpx::TrajectoryNode)

TreeExperiment::TreeExperiment(QObject *parent)
  : QAbstractItemModel(parent)
{
  root = std::shared_ptr<Qpx::TrajectoryNode>( new Qpx::TrajectoryNode() );
}


int TreeExperiment::columnCount(const QModelIndex & /* parent */) const
{
  return 4;
}

QVariant TreeExperiment::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if(!index.isValid())
    return QVariant();
  ItemPtr item = static_cast<ItemPtr>(index.internalPointer());
  if(item)
  {
    switch(role)
    {
    case Qt::DisplayRole: {
      if (col == 0)
        return QString::fromStdString(item->type());
      else if (col == 1)
        return QString::fromStdString(item->to_string());
      else if (col == 2)
        return QString::fromStdString(item->crit_to_string());
      else if ((col == 3) && (item->data_idx >= 0))
        return "(" + QString::number(item->data_idx) + ")";
      break;
    }
    case Qt::EditRole: {
      return QVariant::fromValue(*item);
    }
      //      case Qt::DecorationRole:
      //          {
      //              auto it = type2icon.find(item->type_id());
      //              if(it != type2icon.end())
      //                  return it->second;
      //          }
    }
  }
  return QVariant();
}

Qt::ItemFlags TreeExperiment::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return QAbstractItemModel::flags(index);

  ItemPtr item = static_cast<ItemPtr>(index.internalPointer());

  return QAbstractItemModel::flags(index);
}

QVariant TreeExperiment::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    if (section == 0)
      return "type";
    else if (section == 1)
      return "values";
    else if (section == 2)
      return "time";
    else if (section == 3)
      return "idx";

  return QVariant();
}

QModelIndex TreeExperiment::index(int row, int column, const QModelIndex &parent) const
{
  if(!hasIndex(row, column, parent))
    return QModelIndex();

  ItemPtr item = root.get();
  if(parent.isValid())
    item = static_cast<ItemPtr>(parent.internalPointer());

  auto child = item->getChild(row);
  if(child)
    return createIndex(row,column,(void*)child.get());
  return QModelIndex();
}

QModelIndex TreeExperiment::parent(const QModelIndex &child) const
{
  if(!child.isValid())
    return QModelIndex();
  ItemPtr c = static_cast<ItemPtr>(child.internalPointer());
  auto p = c->getParent().get();
  if(p == root.get())
    return QModelIndex();
  return createIndex(p->row(),0,(void*)p);
}

QModelIndex TreeExperiment::indexOf(Qpx::TrajectoryPtr goal) const
{
  std::list<int> trace;
  Qpx::TrajectoryPtr copy = goal;
  while (copy && copy->getParent())
  {
    trace.push_back(copy->row());
    copy = copy->getParent();
  }

  if (trace.empty())
    return QModelIndex();

  QModelIndex idx;
  while (!trace.empty() && this->rowCount(idx))
  {
    idx = this->index(trace.back(), 0, idx);
    trace.pop_back();
  }

  if (idx.isValid() && (static_cast<ItemPtr>(idx.internalPointer()) == goal.get()))
    return idx;
  return QModelIndex();
}

int TreeExperiment::rowCount(const QModelIndex &parent) const
{
  if(!parent.isValid())
    return root->childCount();
  if(parent.column()>0)
    return 0;
  ItemPtr p =static_cast<ItemPtr>(parent.internalPointer());
  return p->childCount();
}

bool TreeExperiment::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant &value, int role)
{
  //  if (role != Qt::EditRole || orientation != Qt::Horizontal)
  //    return false;

  //  bool result = rootItem->setData(section, value);

  //  if (result) {
  //    emit headerDataChanged(orientation, section, section);
  //    //emit push settings to device
  //  }

  //  return result;
}


void TreeExperiment::set_root(std::shared_ptr<Qpx::TrajectoryNode> r)
{
  beginResetModel();
  root = r;
  endResetModel();

  emit layoutChanged();
}

void TreeExperiment::emplace_back(QModelIndex &index, Qpx::TrajectoryNode&& t)
{
  if(!index.isValid())
    return;
  ItemPtr item = static_cast<ItemPtr>(index.internalPointer());
  if(!item)
    return;
  beginInsertRows(index,item->childCount(),item->childCount());
  item->emplace_back(std::forward<Qpx::TrajectoryNode>(t));
  endInsertRows();
}

bool TreeExperiment::retro_push(Qpx::TrajectoryPtr node)
{
  if (!node)
    return false;
  QModelIndex parent = indexOf(node->getParent());

  if (!parent.isValid())
    return false;

  ItemPtr item = static_cast<ItemPtr>(parent.internalPointer());
  if(!item)
    return false;

  beginInsertRows(parent,node->row(),node->row());
  endInsertRows();
  return true;
}


bool TreeExperiment::push_back(QModelIndex &index, const Qpx::TrajectoryNode& t)
{
  if(!index.isValid())
    return false;
  ItemPtr item = static_cast<ItemPtr>(index.internalPointer());
  if(!item)
    return false;
  beginInsertRows(index,item->childCount(),item->childCount());
  item->push_back(t);
  endInsertRows();
  return true;
}

bool TreeExperiment::remove_row(QModelIndex &index)
{
  if(!index.isValid())
    return false;
  QModelIndex paridx = parent(index);
  if (!paridx.isValid())
    return false;
  ItemPtr item = static_cast<ItemPtr>(paridx.internalPointer());
  if (!item)
    return false;
  beginRemoveRows(paridx, index.row(), index.row());
  item->remove_child(index.row());
  endRemoveRows();
  return true;
}

bool TreeExperiment::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (role != Qt::EditRole)
    return false;

  if(!index.isValid())
    return false;
  QModelIndex paridx = parent(index);
  if (!paridx.isValid())
    return false;
  ItemPtr item = static_cast<ItemPtr>(paridx.internalPointer());
  if (!item)
    return false;

  if (!value.canConvert<Qpx::TrajectoryNode>())
    return false;
  Qpx::TrajectoryNode tn = qvariant_cast<Qpx::TrajectoryNode>(value);


  beginInsertRows(paridx, index.row(), index.row());
  item->replace_child(index.row(), tn);

  QModelIndex parparidx = parent(paridx);
  QVector<int> roles;
  roles << Qt::DisplayRole;
  QModelIndex lindex = this->index(index.row(), 0, paridx);
  QModelIndex rindex = this->index(index.row(), columnCount()-1, paridx);
  emit dataChanged(lindex, rindex);

  endInsertRows();

  return true;
}

/*bool TreeExperiment::insertRows(int position, int rows, const QModelIndex &parent)
{
  Qpx::TrajectoryNode *parentItem = getItem(parent);
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success = parentItem->insertChildren(position, rows, rootItem->columnCount());
  endInsertRows();

  return success;
}*/

/*bool TreeExperiment::removeRows(int position, int rows, const QModelIndex &parent)
{
  Qpx::TrajectoryNode *parentItem = getItem(parent);
  bool success = true;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}*/
