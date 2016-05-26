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
 *      TableChanSettings - tree for displaying and manipulating
 *      channel settings and chosing detectors.
 *
 ******************************************************************************/


#ifndef TREE_EXPERIMENT_H_
#define TREE_EXPERIMENT_H_

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QFont>
#include <QBrush>
#include "domain.h"

class TreeExperiment : public QAbstractItemModel
{
  Q_OBJECT

private:
  std::shared_ptr<Qpx::TrajectoryNode> root;

  //boost::container::flat_map<size_t,QIcon> type2icon;

//  std::shared_ptr<Qpx::TrajectoryNode> getItem(const QModelIndex &index) const;
public:
  using ItemPtr = Qpx::TrajectoryNode*;
  using constItemPtr = const Qpx::TrajectoryNode*;

  explicit TreeExperiment(QObject *parent = 0);

  QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
  Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
  int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

//  bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
//  bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

//  void update(const Qpx::Setting &data);

  QModelIndex indexOf(Qpx::TrajectoryPtr) const;
  bool retro_push(Qpx::TrajectoryPtr);

  std::shared_ptr<Qpx::TrajectoryNode> getRoot()const{return root;}
  void emplace_back(QModelIndex &index, Qpx::TrajectoryNode && t);
  bool push_back(QModelIndex &index, const Qpx::TrajectoryNode &t);
  bool remove_row(QModelIndex &index);
  void set_root(std::shared_ptr<Qpx::TrajectoryNode> r);
//  void insertIcon(size_t type, QIcon icon){type2icon[type]=icon;}

signals:
  void tree_changed();

};


#endif
