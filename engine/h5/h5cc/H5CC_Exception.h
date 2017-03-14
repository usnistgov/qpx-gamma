#pragma once

#include "H5Exception.h"
#include <stdexcept>

namespace H5CC
{

class Exception
{
public:
  inline static void rethrow()
  {
    using namespace std;
    try
    {
      throw;
    }
    catch(H5::Exception error)
    {
      std::string ret = error.getDetailMsg() + " (" + error.getFuncName() + ")";
      throw std::runtime_error(ret.c_str());
    }
    catch (...)
    {
      throw;
    }
  }

};

}

