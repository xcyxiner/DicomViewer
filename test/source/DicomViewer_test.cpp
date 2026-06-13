#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "DicomViewer" ? 0 : 1;
}
