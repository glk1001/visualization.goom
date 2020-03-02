// Compile with
//
// g++ -std=c++17 test_goom_logging.cpp -o test_goom_logging -I ../lib/goom/src -lgoom -L ../build/visualization.goom-prefix/src/visualization.goom-build/lib/goom

extern "C" {
  #include "goom/goom.h"
  #include "goom/goom_config.h"
  #include "goom/goom_logging.h"
}
#include <iostream>
#include <ostream>


static void my_logger(int lvl, const int line_num, const char *func_name, const char *msg)
{
  std::cout << "LOG: line " << line_num << ", " << func_name << ": " << msg << std::endl;
}

int main(int argc, const char* argv[])
{
  std::cout << "Goom logging test." << std::endl << std::endl;

  goom_logger = my_logger;
  GOOM_LOG_INFO("Hello %s number %d", "World", 10);
}
