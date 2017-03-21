#include <H5Cpp.h>

namespace H5CC {

inline int exceptions_off()
{
  H5::Exception::dontPrint();
  return 1;
}

static const int initializer = exceptions_off();

}
