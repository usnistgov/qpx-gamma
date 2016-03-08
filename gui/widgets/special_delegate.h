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
 *      QpxSpecialDelegate - displays colors, patterns, allows chosing of
 *      detectors.
 *
 ******************************************************************************/


#ifndef QPX_SPECIAL_DELEGATE_H_
#define QPX_SPECIAL_DELEGATE_H_

#include <QStyledItemDelegate>
#include <QDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "widget_pattern.h"
#include "detector.h"
#include "qtcolorpicker.h"

Q_DECLARE_METATYPE(Qpx::Detector)
Q_DECLARE_METATYPE(Qpx::Setting)

class QpxSpecialDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    QpxSpecialDelegate(QObject *parent = 0): QStyledItemDelegate(parent), detectors_("Detectors") {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const Q_DECL_OVERRIDE;
    void eat_detectors(const XMLableDB<Qpx::Detector>&);
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const Q_DECL_OVERRIDE;

private slots:
    void get_color_from_dialog(QtColorPicker *);

signals:
    void begin_editing() const;
    void ask_execute(Qpx::Setting command, QModelIndex index) const;
    void ask_binary(Qpx::Setting command, QModelIndex index) const;

private:
    XMLableDB<Qpx::Detector> detectors_;
};

#endif
