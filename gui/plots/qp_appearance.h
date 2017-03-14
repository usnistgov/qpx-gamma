#pragma once

#include <QPen>
#include <QMap>

namespace QPlot
{

struct Appearance
{
  QMap<QString, QPen> themes;
  QPen default_pen {Qt::gray, 1, Qt::SolidLine};

  QPen get_pen(QString theme) const;
};

}
