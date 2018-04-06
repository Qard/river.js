#pragma once

#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <uv.h>

#include "uvv8/util/check.hh"

bool IsAbsolutePath(const std::string& path);

std::string GetWorkingDirectory();

// Returns the directory part of path, without the trailing '/'.
std::string DirName(const std::string& path);

// Resolves path to an absolute path if necessary, and does some
// normalization (eliding references to the current directory
// and replacing backslashes with slashes).
std::string NormalizePath(const std::string& path,
                          const std::string& dir_name);

const std::string& ReadFile(const std::string& path);

// const std::string ResolveRelative(const std::string& path,
//                            const std::string& base,
//                            std::vector<std::string> extensions = { "js" });
