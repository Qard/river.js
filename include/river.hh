#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <libplatform/libplatform.h>
#include <v8.h>
#include <uv.h>

#include "util/check.hh"
#include "util/convert.hh"
#include "util/exceptions.hh"
#include "module-loader.hh"
#include "async-task.hh"
#include "uv.hh"

#include "uvv8/fs.hh"

struct River {
  void main(int argc, char** argv);
};
