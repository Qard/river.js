#include "uvv8/util/path.hh"

bool starts_with(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size()
    && str.compare(0, suffix.size(), suffix) == 0;
}

bool IsAbsolutePath(const std::string& path) {
  #if defined(_WIN32) || defined(_WIN64)
    // TODO(adamk): This is an incorrect approximation, but should
    // work for all our test-running cases.
    return path.find(':') != std::string::npos;
  #else
    return path[0] == '/';
  #endif
}

std::string GetWorkingDirectory() {
  std::string cwd_string;
  size_t size = 1024;
  char buf[size];
  if (uv_cwd(buf, &size) == UV_ENOBUFS) {
    char buf2[size];
    uv_cwd(buf2, &size);
    cwd_string = buf2;
  } else {
    cwd_string = buf;
  }
  return cwd_string;
}

// Returns the directory part of path, without the trailing '/'.
std::string DirName(const std::string& path) {
  DCHECK(IsAbsolutePath(path));
  size_t last_slash = path.find_last_of('/');
  DCHECK(last_slash != std::string::npos);
  return path.substr(0, last_slash);
}

// Resolves path to an absolute path if necessary, and does some
// normalization (eliding references to the current directory
// and replacing backslashes with slashes).
std::string NormalizePath(const std::string& path,
                          const std::string& dir_name) {

  std::cout << "normalizing path: " << path << ", dir: " << dir_name << std::endl;
  std::string result;
  if (IsAbsolutePath(path)) {
    result = path;
  } else {
    result = dir_name + '/' + path;
  }
  std::replace(result.begin(), result.end(), '\\', '/');
  size_t i;
  while ((i = result.find("/./")) != std::string::npos) {
    result.erase(i, 2);
  }
  return result;
}

// inline bool exists(const std::string& path) {
//   return access(path.c_str(), F_OK) != -1;
// }
//
// std::string get_extension(const std::string& path) {
//   auto position = path.find_last_of(".");
//   return position != std::string::npos
//     ? path.substr(position + 1)
//     : "";
// }
//
// bool has_extension(const std::string& path, std::vector<std::string> extensions) {
//   for (auto extension : extensions) {
//     if (get_extension(path) == extension) {
//       return true;
//     }
//   }
//   return false;
// }
//
// const std::string& resolve_full(const std::string& path, const std::string& base) {
//   if (exists(full_path)) {
//     return full_path;
//   }
// }
//
// std::vector<std::string> split(const std::string &str, const char &delimiter) {
//   auto head = str.begin();
//   std::vector<std::string> parts;
//
//   while (head != str.end()) {
//     auto temp = find(head, str.end(), delimiter);
//     if (head != str.end()) {
//       parts.push_back(std::string(head, temp));
//     }
//
//     head = temp;
//     while ((head != str.end()) && (*head == delimiter)) {
//       head++;
//     }
//   }
//
//   return parts;
// }
//
// const std::string Resolve(const std::string& path) {
//   std::vector<std::string> parts = split(path, '/');
//   std::vector<std::string> buffer;
//
//   for (auto part : parts) {
//     // Dots and empty segments are collapsed
//     if (part == "." || part == "") {
//       continue;
//
//     // Double dot steps back a segment
//     } else if (part == "..") {
//       // Stepped back too far
//       if (!buffer.size()) {
//         return nullptr;
//       }
//       // Drop the last segment
//       buffer.pop_back();
//
//     // Otherwise, add a segment
//     } else {
//       buffer.push_back(part);
//     }
//   }
//
//   // Join the final segment buffer
//   std::ostringstream result;
//   for (auto part : buffer) {
//     result << "/" << part;
//   }
//
//   return result.str();
// }
//
// const std::string& Join(const std::string& path, const std::string& base) {
//
// }
//
// const std::string ResolveRelative(const std::string& path, const std::string& base, std::vector<std::string> extensions) {
//   std::string resolved;
//
//   // If there is no extension, try the extensions list.
//   if (get_extension(path) != "") {
//     std::string current = Resolve(base + "/" + path);
//     return exists(current)
//       ? current
//       : ResolveRelative(path, base + "/..", extensions);
//   } else {
//     std::string full_path;
//     for (auto extension : extensions) {
//       full_path = ResolveRelative(path + "." + extension, base, extensions);
//       if (!full_path.empty()) {
//         return full_path;
//       }
//     }
//   }
//
//   return nullptr;
// }

const std::string& ReadFile(const std::string& path) {
  std::ifstream it(path.c_str());
  const std::string* contents = new std::string(
    (std::istreambuf_iterator<char>(it)),
    std::istreambuf_iterator<char>()
  );
  return *contents;
}
