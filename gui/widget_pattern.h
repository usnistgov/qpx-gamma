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
 *      QpxPattern - coincidence pattern with drawing and editing facilities
 *      QpxPatternEditor - widget that represents coincidence pattern
 *
 ******************************************************************************/

#ifndef QPX_PATTERN_H
#define QPX_PATTERN_H
#include <QObject>
#include <QMetaType>
#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QStyledItemDelegate>
#include <vector>
#include <bitset>
#include "detector.h"
#include "custom_logger.h"

class QpxPattern
{
public:

    explicit QpxPattern(QVector<int16_t> pattern = QVector<int16_t>({0,0,0,0}), bool tristate = false);
    explicit QpxPattern(std::bitset<16> pattern, bool tristate = false);

    void paint(QPainter *painter, const QRect &rect,
               const QPalette &palette, bool enabled = true) const;
    QSize sizeHint() const;
    int checkFlag(int count) const { if (count < maxCount()) return pattern_[count]; else return -1; }
    int maxCount() const { return pattern_.size(); }
    void setFlag(int count);
    QVector<int16_t> const& pattern() const {return pattern_;}

private:
    QRectF outer, inner;
    QVector<int16_t> pattern_;
    bool tristate_;
    int PaintingScaleFactor;
};

Q_DECLARE_METATYPE(QpxPattern)

Q_DECLARE_METATYPE(Pixie::Detector)

class QpxPatternEditor : public QWidget
{
    Q_OBJECT
public:
    QpxPatternEditor(QWidget *parent = 0);
    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setQpxPattern(const QpxPattern &qpxPattern) { myQpxPattern = qpxPattern; }
    QpxPattern qpxPattern() { return myQpxPattern; }

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    int flagAtPosition(int x);
    QpxPattern myQpxPattern;
};

#endif
