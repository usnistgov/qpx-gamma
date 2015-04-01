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
 *      QpxSpectra - displays boxes for multiple spectra with name and color
 *      QpxSpectraWidget - widget that allows selection of spectra
 *
 ******************************************************************************/

#ifndef QPX_SPECTRA_WIDGET
#define QPX_SPECTRA_WIDGET
#include <QObject>
#include <QMetaType>
#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QStaticText>
#include <vector>
#include <list>
#include "spectra_set.h"
#include "custom_logger.h"

struct SpectrumAvatar {
    QString name;
    QColor color;
    bool selected;
};

class QpxSpectraWidget : public QWidget
{
    Q_OBJECT

public:
    QpxSpectraWidget(QWidget *parent = 0);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setQpxSpectra(Pixie::SpectraSet &newset, int dim = 0, int res = 0);
    void update_looks();
    QString selected();

signals:
    void stateChanged();
    void contextRequested();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    Pixie::SpectraSet *all_spectra_;
    QVector<SpectrumAvatar> my_spectra_;

    int rect_w_, rect_h_, border;
    int width_last, height_total, max_wide;
    int selected_;

    QRectF inner, outer, text;

    void recalcDim(int, int);

    int flagAt(int, int);

};

#endif
