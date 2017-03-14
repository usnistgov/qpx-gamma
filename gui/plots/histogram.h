#pragma once

#include <list>
#include <map>

struct c2d
{
  c2d(uint32_t xx, uint32_t yy)
    : x(xx), y(yy) {}
  uint32_t x, y;
};

inline bool operator<(const c2d& a, const c2d& b)
{
  return a.x < b.x || (!(b.x < a.x) && a.y < b.y);
}


struct p2d
{
  p2d(uint32_t xx, uint32_t yy, double vv)
    : x(xx), y(yy), v(vv) {}
  p2d(c2d c, double vv)
    : x(c.x), y(c.y), v(vv) {}
  uint32_t x, y;
  double v;
};

inline bool operator<(const p2d& a, const p2d& b)
{
  return a.x < b.x || (!(b.x < a.x) && a.y < b.y);
}


using HistMap2D = std::map<c2d, double>;
using HistList2D = std::list<p2d>;

inline HistList2D to_list(const HistMap2D &map)
{
  HistList2D ret;
  for (auto m : map)
    ret.push_back(p2d{m.first, m.second});
  return ret;
}

inline HistMap2D to_map(const HistList2D &list)
{
  HistMap2D ret;
  for (auto m : list)
    ret[c2d{m.x, m.y}] = m.v;
  return ret;
}

using HistMap1D = std::map<double,double>;
using HistList1D = std::list<std::pair<double,double>>;

inline HistList1D to_list(const HistMap1D &map)
{
  HistList1D ret;
  for (auto m : map)
    ret.push_back( {m.first, m.second} );
  return ret;
}

inline HistMap1D to_map(const HistList1D &list)
{
  HistMap1D ret;
  for (auto m : list)
    ret[m.first] = m.second;
  return ret;
}
