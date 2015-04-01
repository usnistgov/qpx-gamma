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
 *      QpxSaveTypesWidget - widget for selecting file types for saving spectra
 *      DialogSaveSpectra  - dialog for chosing location and types
 *
 ******************************************************************************/


#ifndef DIALOGSAVESPECTRA_H_
#define DIALOGSAVESPECTRA_H_

#include <vector>
#include <unordered_map>
#include <QDialog>
#include "spectra_set.h"


class WidgetSaveTypes : public QWidget
{
    Q_OBJECT

public:
    WidgetSaveTypes(QWidget *parent = 0);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    void initialize(std::vector<std::string> types);

    std::vector<std::string> spectrum_types;
    std::vector<std::vector<std::string>> file_formats;
    std::vector<std::vector<bool>> selections;

signals:
    void stateChanged();

protected:
    int w_, h_;
    int max_formats_;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

};

namespace Ui {
class DialogSaveSpectra;
}

class DialogSaveSpectra : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSaveSpectra(Pixie::SpectraSet&, QString, QWidget *parent = 0);
    ~DialogSaveSpectra();

private slots:
    void on_lineName_textChanged(const QString &arg1);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::DialogSaveSpectra *ui;
    QString root_dir_;
    std::string total_dir_;
    Pixie::SpectraSet *my_set_;
};

#endif // DIALOGSAVESPECTRA_H
