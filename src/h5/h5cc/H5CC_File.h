#pragma once

#include "H5CC_Group.h"

namespace H5CC {

enum class Access { no_access,
                    r_existing,
                    rw_existing,
                    rw_new,
                    rw_truncate,
                    rw_require
                  };

class File : public Groupoid<H5::H5File>
{
public:
  File();
  File(std::string filename, Access access = Access::rw_require);

  bool open(std::string filename, Access access = Access::rw_require);
  void close();

  Access status() const noexcept { return status_; }
  bool is_open() const { return (status_ != Access::no_access); }

private:
  Access status_ { Access::no_access };

};

}
