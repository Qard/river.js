#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <libplatform/libplatform.h>
#include <v8.h>
#include <uv.h>

#include <uvv8/uvv8.hh>
#include "module-loader.hh"

struct River {
  void main(int argc, char** argv);
};
