#pragma once

#include <exception>
#include <system_error>
#include <future>
#include <iostream>
#include <ios>

template <typename T>
void processCodeException(const T& e)
{
  auto c = e.code();
  std::cerr << "- category:         " << c.category().name() << std::endl;
  std::cerr << "- value:            " << c.value() << std::endl;
  std::cerr << "- message:          " << c.message() << std::endl;
  std::cerr << "- def category:     " << c.default_error_condition().category().name() << std::endl;
  std::cerr << "- def value:        " << c.default_error_condition().value() << std::endl;
  std::cerr << "- def message:      " << c.default_error_condition().message() << std::endl;
}

inline void printException()
{
  try
  {
    throw;
  }
  catch (const std::ios_base::failure& e)
  {
    std::cerr << "I/O EXCEPTION: " << e.what() << std::endl;
//    processCodeException(e);
  }
  catch (const std::system_error& e)
  {
    std::cerr << "SYSTEM EXCEPTION: " << e.what() << std::endl;
    processCodeException(e);
  }
  catch (const std::future_error& e)
  {
    std::cerr << "FUTURE EXCEPTION: " << e.what() << std::endl;
    processCodeException(e);
  }
  catch (const std::bad_alloc& e)
  {
    std::cerr << "BAD ALLOC EXCEPTION: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "EXCEPTION (unknown)" << std::endl;
  }
}
