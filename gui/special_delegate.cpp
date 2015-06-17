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
 *      QpxSpecialDelegate - displays colors, patterns, allows chosing of
 *      detectors.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "special_delegate.h"
#include "generic_setting.h"
#include <QComboBox>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>

void QpxSpecialDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
  if (index.data().canConvert<QpxPattern>()) {
    QpxPattern qpxPattern = qvariant_cast<QpxPattern>(index.data());
    if (option.state & QStyle::State_Selected)
      painter->fillRect(option.rect, option.palette.highlight());
    qpxPattern.paint(painter, option.rect, option.palette);
  } else if (index.data().type() == QVariant::Color) {
    QColor thisColor = qvariant_cast<QColor>(index.data());
    painter->fillRect(option.rect, thisColor);
  }
  else
    QStyledItemDelegate::paint(painter, option, index);

}

QSize QpxSpecialDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
  if (index.data().canConvert<QpxPattern>()) {
    QpxPattern qpxPattern = qvariant_cast<QpxPattern>(index.data());
    return qpxPattern.sizeHint();
  } else {
    return QStyledItemDelegate::sizeHint(option, index);
  }
}

QWidget *QpxSpecialDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const

{
  if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
    Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
    if (set.setting_type == Pixie::SettingType::floating) {
      QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
      return editor;
    } else if (set.setting_type == Pixie::SettingType::integer) {
      QSpinBox *editor = new QSpinBox(parent);
      return editor;
    } else if (set.setting_type == Pixie::SettingType::text) {
      QLineEdit *editor = new QLineEdit(parent);
      return editor;
    } else if (set.setting_type == Pixie::SettingType::detector) {
      QComboBox *editor = new QComboBox(parent);
      editor->addItem(QString("none"), QString("none"));
      for (int i=0; i < detectors_.size(); i++) {
        QString name = QString::fromStdString(detectors_.get(i).name_);
        editor->addItem(name, name);
      }
      return editor;
    } else if (set.setting_type == Pixie::SettingType::boolean) {
      QCheckBox *editor = new QCheckBox(parent);
      return editor;
    } else if (set.setting_type == Pixie::SettingType::int_menu) {
      QComboBox *editor = new QComboBox(parent);
      for (auto &q : set.int_menu_items)
        editor->addItem(QString::fromStdString(q.second), QVariant::fromValue(q.first));
      return editor;
    }
  } else if (index.data(Qt::EditRole).type() == QVariant::Double) {
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setDecimals(6);
    editor->setRange(std::numeric_limits<double>::min(),std::numeric_limits<double>::max());
    return editor;
  } else {
    return QStyledItemDelegate::createEditor(parent, option, index);
  }
}

void QpxSpecialDelegate::setEditorData ( QWidget *editor, const QModelIndex &index ) const
{
  if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      if (set.setting_type == Pixie::SettingType::detector) {
        int cbIndex = cb->findText(QString::fromStdString(set.name));
        if(cbIndex >= 0)
          cb->setCurrentIndex(cbIndex);
      } else if (set.int_menu_items.count(set.value_int)) {
        int cbIndex = cb->findText(QString::fromStdString(set.int_menu_items[set.value_int]));
        if(cbIndex >= 0)
          cb->setCurrentIndex(cbIndex);
      }
    }
  } else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      sb->setDecimals(6); //generalize
      sb->setRange(set.minimum, set.maximum);
      sb->setSingleStep(set.step);
      sb->setValue(set.value);
    } else
      sb->setValue(index.data(Qt::EditRole).toDouble());
  } else if (QSpinBox *sb = qobject_cast<QSpinBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      sb->setRange(static_cast<int64_t>(set.minimum), static_cast<int64_t>(set.maximum));
      sb->setSingleStep(static_cast<int64_t>(set.step));
      sb->setValue(static_cast<int64_t>(set.value_int));
    } else
      sb->setValue(index.data(Qt::EditRole).toInt());
  } else if (QLineEdit *le = qobject_cast<QLineEdit *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      le->setText(QString::fromStdString(set.value_text));
    } else
      le->setText(index.data(Qt::EditRole).toString());
  } else if (QCheckBox *cb = qobject_cast<QCheckBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      cb->setChecked(set.value_int);
    } else
      cb->setChecked(index.data(Qt::EditRole).toBool());
  } else {
    QStyledItemDelegate::setEditorData(editor, index);
  }
}

void QpxSpecialDelegate::setModelData ( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Pixie::Setting>()) {
      Pixie::Setting set = qvariant_cast<Pixie::Setting>(index.data(Qt::EditRole));
      if (cb->currentData().type() == QMetaType::Int)
        model->setData(index, QVariant::fromValue(cb->currentData().toInt()), Qt::EditRole);
      else if (cb->currentData().type() == QMetaType::Double)
        model->setData(index, QVariant::fromValue(cb->currentData().toDouble()), Qt::EditRole);
      else if (cb->currentData().type() == QMetaType::QString) {
        QString word = cb->currentData().toString();
        if (set.setting_type == Pixie::SettingType::detector)
          model->setData(index, QVariant::fromValue(detectors_.get(word.toStdString())), Qt::EditRole);
        else
          model->setData(index, QVariant::fromValue(word), Qt::EditRole);
      }
    }
  } else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox *>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QSpinBox *sb = qobject_cast<QSpinBox *>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QLineEdit *le = qobject_cast<QLineEdit *>(editor))
    model->setData(index, le->text(), Qt::EditRole);
  else if (QCheckBox *cb = qobject_cast<QCheckBox *>(editor))
    model->setData(index, QVariant::fromValue(cb->isChecked()), Qt::EditRole);
  else
    QStyledItemDelegate::setModelData(editor, model, index);
}

void QpxSpecialDelegate::eat_detectors(const XMLableDB<Pixie::Detector> &detectors) {
  detectors_ = detectors;
}
