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
 *      DialogSpectrumTemplate - edit dialog for spectrum template
 *      TableSpectraTemplates - table of spectrum templates
 *      DialogSpectraTemplates - dialog for managing spectrum templates
 *
 ******************************************************************************/

#ifndef DIALOG_SPECTRA_TEMPLATES_H_
#define DIALOG_SPECTRA_TEMPLATES_H_

#include <QDialog>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include "spectrum.h"
#include "special_delegate.h"
#include "table_spectrum_attrs.h"
#include "qt_util.h"

namespace Ui {
class DialogSpectraTemplates;
class DialogSpectrumTemplate;
}


class TableSpectraTemplates : public QAbstractTableModel
{
  Q_OBJECT
private:
  XMLableDB<Qpx::Spectrum::Template> &templates_;

public:
  TableSpectraTemplates(XMLableDB<Qpx::Spectrum::Template>& templates, QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;

  void update();
};


class DialogSpectrumTemplate : public QDialog
{
  Q_OBJECT
public:
  explicit DialogSpectrumTemplate(Qpx::Spectrum::Template, bool, QWidget *parent = 0);
  ~DialogSpectrumTemplate();

signals:
  void templateReady(Qpx::Spectrum::Template);

private slots:
  void on_buttonBox_accepted();
  void on_lineName_editingFinished();
  void on_checkBox_clicked();
  void on_spinBits_valueChanged(int arg1);
  void colorChanged(const QColor &col);

  void on_buttonBox_rejected();

  void on_pushRandColor_clicked();

  void on_comboType_activated(const QString &arg1);

  void on_spinDets_valueChanged(int arg1);

private:
  void updateData();

  Ui::DialogSpectrumTemplate *ui;
  Qpx::Spectrum::Template myTemplate;

  QpxSpecialDelegate      special_delegate_;
  TableSpectrumAttrs         table_model_;
};

class DialogSpectraTemplates : public QDialog
{
  Q_OBJECT

public:
  explicit DialogSpectraTemplates(XMLableDB<Qpx::Spectrum::Template> &newdb, QString savedir, QWidget *parent = 0);
  ~DialogSpectraTemplates();

private:
  Ui::DialogSpectraTemplates *ui;

  XMLableDB<Qpx::Spectrum::Template> &templates_;

  QpxSpecialDelegate      special_delegate_;
  TableSpectraTemplates         table_model_;
  QItemSelectionModel selection_model_;

  QString root_dir_;

private slots:
  void add_template(Qpx::Spectrum::Template);
  void change_template(Qpx::Spectrum::Template);

  void on_pushImport_clicked();
  void on_pushExport_clicked();
  void on_pushNew_clicked();
  void on_pushEdit_clicked();
  void on_pushDelete_clicked();

  void on_pushSetDefault_clicked();
  void on_pushUseDefault_clicked();

  void selection_changed(QItemSelection,QItemSelection);
  void selection_double_clicked(QModelIndex);

  void toggle_push();
  void on_pushClear_clicked();
  void on_pushUp_clicked();
  void on_pushDown_clicked();
};

#endif // DIALOGSPECTRATEMPLATES_H
