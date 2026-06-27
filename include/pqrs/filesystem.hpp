#pragma once

// pqrs::filesystem v1.3.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <optional>
#include <string>
#include <sys/stat.h>

namespace pqrs::filesystem {

[[nodiscard]] inline std::optional<uid_t> uid(const std::string& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<uid_t> symlink_uid(const std::string& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_uid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> gid(const std::string& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<gid_t> symlink_gid(const std::string& path) noexcept {
  struct stat s;
  if (lstat(path.c_str(), &s) == 0) {
    return s.st_gid;
  }
  return std::nullopt;
}

[[nodiscard]] inline bool is_owned(const std::string& path, uid_t uid) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    return s.st_uid == uid;
  }
  return false;
}

[[nodiscard]] inline std::optional<mode_t> file_access_permissions(const std::string& path) noexcept {
  struct stat s;
  if (stat(path.c_str(), &s) != 0) {
    return std::nullopt;
  }
  return s.st_mode & ACCESSPERMS;
}

inline void copy(const std::string& from_file_path,
                 const std::string& to_file_path) {
  std::ifstream ifstream(from_file_path);
  if (!ifstream) {
    return;
  }
  std::ofstream ofstream(to_file_path);
  if (!ofstream) {
    return;
  }
  ofstream << ifstream.rdbuf();
}

} // namespace pqrs::filesystem
