#ifndef QP_ENTRY2D_H
#define QP_ENTRY2D_H

#include <list>
#include <vector>
#include <map>
#include "precise_float.h"

namespace QPlot
{

using Entry = std::pair<std::vector<size_t>, PreciseFloat>;
using EntryList = std::list<Entry>;

}

#endif
