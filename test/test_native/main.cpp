#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>
#include <ranges>
#include <span>

#include "menu.h"

using namespace menu;

int main(int argc, char **argv) {
  doctest::Context context;
  context.setOption("success", true);
  context.setOption("no-exitcode", true);
  context.applyCommandLine(argc, argv);
  return context.run();
}
