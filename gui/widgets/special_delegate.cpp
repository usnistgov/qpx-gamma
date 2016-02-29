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
#include "qt_util.h"
#include <QComboBox>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include "time_duration_widget.h"

#include <QApplication>

void QpxSpecialDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{

  if ((option.state & QStyle::State_Selected) && !(option.state & QStyle::State_HasFocus)) {
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }

  /*if (index.data().canConvert<QpxPattern>()) {
    QpxPattern qpxPattern = qvariant_cast<QpxPattern>(index.data());
    if (option.state & QStyle::State_Selected)
      painter->fillRect(option.rect, option.palette.highlight());
    qpxPattern.paint(painter, option.rect, option.palette);
  } else*/ if (index.data().type() == QVariant::Color) {
    QColor thisColor = qvariant_cast<QColor>(index.data());
    painter->fillRect(option.rect, thisColor);
  } else if (index.data().canConvert<Qpx::Setting>()) {
    Qpx::Setting itemData = qvariant_cast<Qpx::Setting>(index.data());
    if (itemData.metadata.setting_type == Qpx::SettingType::command) {
      QStyleOptionButton button;
      button.rect = option.rect;
      button.text = QString::fromStdString(itemData.metadata.name);
      if (itemData.metadata.writable)
        button.state = QStyle::State_Enabled;
      QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter);
    } else if (itemData.metadata.setting_type == Qpx::SettingType::pattern) {
      QpxPatternEditor pat(itemData.value_pattern, 20, 8);
      pat.setEnabled(itemData.metadata.writable);
      if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
      pat.paint(painter, option.rect, option.palette);
    } else if ((itemData.metadata.setting_type == Qpx::SettingType::dir_path) ||
               (itemData.metadata.setting_type == Qpx::SettingType::text) ||
               (itemData.metadata.setting_type == Qpx::SettingType::integer) ||
               (itemData.metadata.setting_type == Qpx::SettingType::int_menu) ||
               (itemData.metadata.setting_type == Qpx::SettingType::binary) ||
               (itemData.metadata.setting_type == Qpx::SettingType::floating) ||
               (itemData.metadata.setting_type == Qpx::SettingType::detector) ||
               (itemData.metadata.setting_type == Qpx::SettingType::indicator) ||
               (itemData.metadata.setting_type == Qpx::SettingType::time) ||
               (itemData.metadata.setting_type == Qpx::SettingType::time_duration) ||
               (itemData.metadata.setting_type == Qpx::SettingType::floating_precise) ||
               (itemData.metadata.setting_type == Qpx::SettingType::boolean) ||
               (itemData.metadata.setting_type == Qpx::SettingType::file_path)) {

      int flags = Qt::TextWordWrap | Qt::AlignVCenter;

      std::string raw_txt = itemData.val_to_pretty_string();
      if (raw_txt.size() > 32)
        raw_txt = raw_txt.substr(0,32) + "...";
      QString text = QString::fromStdString(raw_txt);

      painter->save();

      if (itemData.metadata.setting_type == Qpx::SettingType::indicator) {
        QColor bkgCol = QColor::fromRgba(
              itemData.get_setting(Qpx::Setting(
                                     itemData.metadata.int_menu_items.at(itemData.value_int)), Qpx::Match::id)
              .metadata.address
              );
        painter->fillRect(option.rect, bkgCol);
        painter->setPen(QPen(Qt::white, 3));
        QFont f = painter->font();
        f.setBold(true);
        painter->setFont(f);
        flags |= Qt::AlignCenter;
      } else if (itemData.metadata.setting_type == Qpx::SettingType::detector) {
        QVector<QColor> palette {Qt::darkCyan, Qt::darkBlue, Qt::darkGreen, Qt::darkRed, Qt::darkYellow, Qt::darkMagenta, Qt::red, Qt::blue};
        uint16_t index = 0;
        if (itemData.indices.size())
          index = *itemData.indices.begin();
        painter->setPen(QPen(palette[index % palette.size()], 2));
        QFont f = painter->font();
        f.setBold(true);
        painter->setFont(f);
      } else {
        if (option.state & QStyle::State_Selected) {
          painter->fillRect(option.rect, option.palette.highlight());
          painter->setPen(option.palette.highlightedText().color());
        } else {
          if (!itemData.metadata.writable)
            painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
          else
            painter->setPen(option.palette.color(QPalette::Active, QPalette::Text));
        }
      }

      painter->drawText(option.rect, flags, text);
      painter->restore();
    }
  }
  else
    QStyledItemDelegate::paint(painter, option, index);

}

QSize QpxSpecialDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{

  /*if (index.data().canConvert<QpxPattern>()) {
    QpxPattern qpxPattern = qvariant_cast<QpxPattern>(index.data());
    return qpxPattern.sizeHint();
  } else*/ if (index.data().canConvert<Qpx::Setting>()) {
    Qpx::Setting itemData = qvariant_cast<Qpx::Setting>(index.data());
    if (itemData.metadata.setting_type == Qpx::SettingType::command) {
      QPushButton button;
      button.setText(QString::fromStdString(itemData.metadata.name));
      return button.sizeHint();
    } else if (itemData.metadata.setting_type == Qpx::SettingType::time) {
      QDateTimeEdit editor;
      editor.setCalendarPopup(true);
      editor.setTimeSpec(Qt::UTC);
      editor.setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
      QSize sz = editor.sizeHint();
      sz.setWidth(sz.width() + 20);
      return sz;
    } else if (itemData.metadata.setting_type == Qpx::SettingType::pattern) {
      QpxPatternEditor pattern(itemData.value_pattern, 20, 8);
      return pattern.sizeHint();
    } else if (itemData.metadata.setting_type == Qpx::SettingType::time_duration) {
      TimeDurationWidget editor;
      return editor.sizeHint();
    } else if ((itemData.metadata.setting_type == Qpx::SettingType::dir_path) ||
               (itemData.metadata.setting_type == Qpx::SettingType::text) ||
               (itemData.metadata.setting_type == Qpx::SettingType::integer) ||
               (itemData.metadata.setting_type == Qpx::SettingType::int_menu) ||
               (itemData.metadata.setting_type == Qpx::SettingType::binary) ||
               (itemData.metadata.setting_type == Qpx::SettingType::floating) ||
               (itemData.metadata.setting_type == Qpx::SettingType::detector) ||
               (itemData.metadata.setting_type == Qpx::SettingType::command) ||
               (itemData.metadata.setting_type == Qpx::SettingType::time) ||
               (itemData.metadata.setting_type == Qpx::SettingType::time_duration) ||
               (itemData.metadata.setting_type == Qpx::SettingType::floating_precise) ||
               (itemData.metadata.setting_type == Qpx::SettingType::indicator) ||
               (itemData.metadata.setting_type == Qpx::SettingType::boolean) ||
               (itemData.metadata.setting_type == Qpx::SettingType::file_path)) {

      std::string raw_txt = itemData.val_to_pretty_string();
      if (raw_txt.size() > 32)
        raw_txt = raw_txt.substr(0,32) + "...";
      QString text = QString::fromStdString(raw_txt);

      QRect r = option.rect;
      QFontMetrics fm(QApplication::font());
      QRect qr = fm.boundingRect(r, Qt::AlignLeft | Qt::AlignVCenter, text);
      QSize size(qr.size());
      return size;
    } else
      return QStyledItemDelegate::sizeHint(option, index);
  } else {
    return QStyledItemDelegate::sizeHint(option, index);
  }
}

QWidget *QpxSpecialDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const

{
  emit begin_editing();

  if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
    Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
    if ((set.metadata.setting_type == Qpx::SettingType::floating)
        || (set.metadata.setting_type == Qpx::SettingType::floating_precise))
    {
      QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::integer) {
      QSpinBox *editor = new QSpinBox(parent);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::binary) {
      emit ask_binary(set, index);
      return nullptr;
    } else if ((set.metadata.setting_type == Qpx::SettingType::command) && (set.metadata.writable))  {
      emit ask_execute(set, index);
      return nullptr;
    } else if (set.metadata.setting_type == Qpx::SettingType::text) {
      QLineEdit *editor = new QLineEdit(parent);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::time) {
      QDateTimeEdit *editor = new QDateTimeEdit(parent);
      editor->setCalendarPopup(true);
      editor->setTimeSpec(Qt::UTC);
      editor->setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::time_duration) {
      TimeDurationWidget *editor = new TimeDurationWidget(parent);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::pattern) {
      QpxPatternEditor *editor = new QpxPatternEditor(parent);
      editor->set_pattern(set.value_pattern, 20, 8);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::detector) {
      QComboBox *editor = new QComboBox(parent);
      editor->addItem(QString("none"), QString("none"));
      for (int i=0; i < detectors_.size(); i++) {
        QString name = QString::fromStdString(detectors_.get(i).name_);
        editor->addItem(name, name);
      }
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::boolean) {
      QCheckBox *editor = new QCheckBox(parent);
      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::file_path) {
      QFileDialog *editor = new QFileDialog(parent, QString("Chose File"),
                                            QFileInfo(QString::fromStdString(set.value_text)).dir().absolutePath(),
                                            QString::fromStdString(set.metadata.unit));
      editor->setFileMode(QFileDialog::ExistingFile);

      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::dir_path) {
      QFileDialog *editor = new QFileDialog(parent, QString("Chose Directory"),
                                            QFileInfo(QString::fromStdString(set.value_text)).dir().absolutePath());
      editor->setFileMode(QFileDialog::Directory);

      return editor;
    } else if (set.metadata.setting_type == Qpx::SettingType::int_menu) {
      QComboBox *editor = new QComboBox(parent);
      for (auto &q : set.metadata.int_menu_items)
        editor->addItem(QString::fromStdString(q.second), QVariant::fromValue(q.first));
      return editor;
    }
  } else if (index.data(Qt::EditRole).canConvert(QMetaType::QVariantList)) {
    QLineEdit *editor = new QLineEdit(parent);
    return editor;
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
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      if (set.metadata.setting_type == Qpx::SettingType::detector) {
        int cbIndex = cb->findText(QString::fromStdString(set.value_text));
        if(cbIndex >= 0)
          cb->setCurrentIndex(cbIndex);
      } else if (set.metadata.int_menu_items.count(set.value_int)) {
        int cbIndex = cb->findText(QString::fromStdString(set.metadata.int_menu_items[set.value_int]));
        if(cbIndex >= 0)
          cb->setCurrentIndex(cbIndex);
      }
    }
  } else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      sb->setRange(set.metadata.minimum, set.metadata.maximum);
      sb->setSingleStep(set.metadata.step);
      sb->setDecimals(6); //generalize
      if (set.metadata.setting_type == Qpx::SettingType::floating_precise)
        sb->setValue(set.value_precise.convert_to<long double>());
      else
        sb->setValue(set.value_dbl);
    } else
      sb->setValue(index.data(Qt::EditRole).toDouble());
  } else if (QSpinBox *sb = qobject_cast<QSpinBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      sb->setRange(static_cast<int64_t>(set.metadata.minimum), static_cast<int64_t>(set.metadata.maximum));
      sb->setSingleStep(static_cast<int64_t>(set.metadata.step));
      sb->setValue(static_cast<int64_t>(set.value_int));
    } else
      sb->setValue(index.data(Qt::EditRole).toInt());
  } else if (QDateTimeEdit *dte = qobject_cast<QDateTimeEdit *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      dte->setDateTime(fromBoostPtime(set.value_time));
    }
  } else if (TimeDurationWidget *dte = qobject_cast<TimeDurationWidget *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      dte->set_total_seconds(set.value_duration.total_seconds());
    }
  } else if (QLineEdit *le = qobject_cast<QLineEdit *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      le->setText(QString::fromStdString(set.value_text));
    } else if (index.data(Qt::EditRole).canConvert(QMetaType::QVariantList)) {
      QVariantList qlist = index.data(Qt::EditRole).toList();
      if (!qlist.isEmpty())
        qlist.pop_front(); //number of indices allowed;
      QString text;
      foreach (QVariant var, qlist) {
        text += QString::number(var.toInt()) + " ";
      }
      le->setText(text.trimmed());
    } else
      le->setText(index.data(Qt::EditRole).toString());
  } else if (QCheckBox *cb = qobject_cast<QCheckBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      cb->setChecked(set.value_int);
    } else
      cb->setChecked(index.data(Qt::EditRole).toBool());
  } else {
    QStyledItemDelegate::setEditorData(editor, index);
  }
}

void QpxSpecialDelegate::setModelData ( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  if (QpxPatternEditor *pe = qobject_cast<QpxPatternEditor *>(editor)) {
    model->setData(index, QVariant::fromValue(pe->pattern()), Qt::EditRole);
  } else if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
    if (index.data(Qt::EditRole).canConvert<Qpx::Setting>()) {
      Qpx::Setting set = qvariant_cast<Qpx::Setting>(index.data(Qt::EditRole));
      if (cb->currentData().type() == QMetaType::Int)
        model->setData(index, QVariant::fromValue(cb->currentData().toInt()), Qt::EditRole);
      else if (cb->currentData().type() == QMetaType::Double)
        model->setData(index, QVariant::fromValue(cb->currentData().toDouble()), Qt::EditRole);
      else if (cb->currentData().type() == QMetaType::QString)
        model->setData(index, cb->currentData().toString(), Qt::EditRole);
    }
  } else if (QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox *>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QSpinBox *sb = qobject_cast<QSpinBox *>(editor))
    model->setData(index, QVariant::fromValue(sb->value()), Qt::EditRole);
  else if (QLineEdit *le = qobject_cast<QLineEdit *>(editor))
    model->setData(index, le->text(), Qt::EditRole);
  else if (QDateTimeEdit *dte = qobject_cast<QDateTimeEdit *>(editor))
    model->setData(index, dte->dateTime(), Qt::EditRole);
  else if (TimeDurationWidget *dte = qobject_cast<TimeDurationWidget *>(editor))
    model->setData(index, QVariant::fromValue(dte->total_seconds()), Qt::EditRole);
  else if (QCheckBox *cb = qobject_cast<QCheckBox *>(editor))
    model->setData(index, QVariant::fromValue(cb->isChecked()), Qt::EditRole);
  else if (QFileDialog *fd = qobject_cast<QFileDialog *>(editor)) {
    if ((!fd->selectedFiles().isEmpty()) /*&& (validateFile(parent, fd->selectedFiles().front(), false))*/)
      model->setData(index, QVariant::fromValue(fd->selectedFiles().front()), Qt::EditRole);
  }
  else
    QStyledItemDelegate::setModelData(editor, model, index);
}

void QpxSpecialDelegate::eat_detectors(const XMLableDB<Qpx::Detector> &detectors) {
  detectors_ = detectors;
}

void QpxSpecialDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}
