#include "H5CC_File.h"
#include "H5CC_Exception.h"

namespace H5CC {

File::File()
  : Groupoid<H5::H5File>()
{}

File::File(std::string filename, Access access)
  : File()
{
  open(filename, access);
}

bool File::open(std::string filename, Access access)
{
  if (is_open())
    close();

  name_.clear();

  try {
  if (access == Access::r_existing)
    Location<H5::H5File>::location_ = H5::H5File(filename, H5F_ACC_RDONLY);
  else if (access == Access::rw_existing)
    Location<H5::H5File>::location_ = H5::H5File(filename, H5F_ACC_RDWR);
  else if (access == Access::rw_new)
    Location<H5::H5File>::location_ = H5::H5File(filename, H5F_ACC_EXCL);
  else if (access == Access::rw_truncate)
    Location<H5::H5File>::location_ = H5::H5File(filename, H5F_ACC_TRUNC);
  }
  catch(...)
  {
    Exception::rethrow();
  }

  if (access == Access::rw_require)
  {
    try
    {
      open(filename, Access::rw_existing);
    }
    catch (...)
    {
      open(filename, Access::rw_new);
      if (!is_open())
        Exception::rethrow();
    }
  }

  status_ = access;
  name_ = filename;

  return (status_ != Access::no_access);
}

void File::close()
{
  try
  {
    Location<H5::H5File>::location_.close();
  }
  catch (...)
  {
    Exception::rethrow();
  }
  status_ = Access::no_access;
  name_.clear();
}

}
