#include "qp_appearance.h"

namespace QPlot
{

QPen Appearance::get_pen(QString theme) const
{
  if (themes.count(theme))
    return themes[theme];
  else
    return default_pen;
}

}


