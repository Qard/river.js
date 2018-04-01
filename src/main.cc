#include "river.hh"

int main(int argc, char** argv) {
  auto river = new River();
  river->main(argc, argv);
  delete river;
  return 0;
}
