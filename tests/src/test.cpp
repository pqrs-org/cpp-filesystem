#include <boost/ut.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <pqrs/filesystem.hpp>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace {
struct temporary_directory final {
  std::string path;

  temporary_directory() {
    auto pattern = (std::filesystem::temp_directory_path() / "cpp-filesystem-XXXXXX").string();
    auto directory = ::mkdtemp(pattern.data());
    if (!directory) {
      throw std::runtime_error("mkdtemp failed");
    }
    path = directory;
  }

  ~temporary_directory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }

  std::string file(const std::string& name) const {
    return path + "/" + name;
  }

  void write(const std::string& name,
             const std::vector<uint8_t>& contents) const {
    std::ofstream stream(file(name),
                         std::ios::binary);
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    if (!contents.empty()) {
      stream.write(reinterpret_cast<const char*>(contents.data()),
                   contents.size());
    }
    stream.close();
  }
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "uid"_test = [] {
    // Return owner uid for an existing path and nullopt for a missing path.
    expect(pqrs::filesystem::uid("/") == 0);

    std::ofstream("data/owned_file.tmp").close();
    expect(pqrs::filesystem::uid("data/owned_file.tmp") == getuid());
    unlink("data/owned_file.tmp");

    expect(pqrs::filesystem::uid("data/not_found") == std::nullopt);
  };

  "symlink_uid"_test = [] {
    // Return symlink owner uid without following the symlink.
    expect(pqrs::filesystem::symlink_uid("data/bin-ls-symlink") == getuid());
    expect(pqrs::filesystem::uid("data/bin-ls-symlink") == 0);
    expect(pqrs::filesystem::symlink_uid("data/not_found") == std::nullopt);
  };

  "gid"_test = [] {
    // Return group id for an existing path and nullopt for a missing path.
    expect(pqrs::filesystem::gid("/bin/ls") == 0);

    std::ofstream("data/owned_file.tmp").close();
    expect(pqrs::filesystem::gid("data/owned_file.tmp") == getgid());
    unlink("data/owned_file.tmp");

    expect(pqrs::filesystem::gid("data/not_found") == std::nullopt);
  };

  "symlink_gid"_test = [] {
    // Return symlink group id without following the symlink.
    expect(pqrs::filesystem::symlink_gid("data/bin-ls-symlink") == getgid());
    expect(pqrs::filesystem::gid("data/bin-ls-symlink") == 0);
    expect(pqrs::filesystem::symlink_gid("data/not_found") == std::nullopt);
  };

  "is_owned"_test = [] {
    // Follow symlinks and compare the target owner with the supplied uid.
    expect(!pqrs::filesystem::is_owned("/bin/ls", getuid()));
    expect(pqrs::filesystem::is_owned("data/file", getuid()));
    expect(!pqrs::filesystem::is_owned("data/not_found", getuid()));
    // Follow symlink.
    // The link target is /bin/ls, while the symlink itself is owned by the current user.
    expect(pqrs::filesystem::symlink_uid("data/bin-ls-symlink") == getuid());
    expect(pqrs::filesystem::is_owned("data/bin-ls-symlink", 0));
  };

  "is_symlink_owned"_test = [] {
    // Compare symlink owner without following the symlink.
    expect(pqrs::filesystem::is_symlink_owned("data/file", getuid()));
    expect(!pqrs::filesystem::is_symlink_owned("data/not_found", getuid()));
    expect(pqrs::filesystem::is_symlink_owned("data/bin-ls-symlink", getuid()));
    expect(!pqrs::filesystem::is_symlink_owned("data/bin-ls-symlink", 0));
  };

  "read_file contents"_test = [] {
    temporary_directory directory;

    // Include NUL and high-bit bytes, and span multiple read buffers.
    std::vector<uint8_t> contents(20000);
    for (size_t i = 0; i < contents.size(); ++i) {
      contents[i] = static_cast<uint8_t>(i);
    }
    directory.write("binary", contents);

    auto result = pqrs::filesystem::read_file(directory.file("binary"));
    expect(result.has_value());
    if (result) {
      expect(*result == contents);
    }

    directory.write("empty", {});
    auto empty = pqrs::filesystem::read_file(directory.file("empty"));
    expect(empty.has_value());
    if (empty) {
      expect(empty->empty());
    }
  };

  "read_file allowed owners"_test = [] {
    temporary_directory directory;

    const std::vector<uint8_t> contents{0, 128, 255};
    directory.write("file", contents);

    auto other_uid = getuid() == 0 ? uid_t{1} : uid_t{0};
    pqrs::filesystem::read_file_options options {
      .allowed_owners = std::vector<uid_t>{
          other_uid,
          getuid(),
      },
    };

    auto allowed = pqrs::filesystem::read_file(directory.file("file"),
                                               options);
    expect(allowed.has_value());
    if (allowed) {
      expect(*allowed == contents);
    }

    for (const auto& owners : {
             std::vector<uid_t>{other_uid},
             std::vector<uid_t>{},
         }) {
      options.allowed_owners = owners;
      auto denied = pqrs::filesystem::read_file(directory.file("file"),
                                                options);
      expect(!denied.has_value());
      if (!denied) {
        expect(denied.error().type == pqrs::filesystem::read_file_error::reason::invalid_owner);
        expect(!denied.error().code);
      }
    }
  };

  "read_file size limit"_test = [] {
    temporary_directory directory;

    const std::vector<uint8_t> contents{0, 1, 2};
    directory.write("file", contents);

    pqrs::filesystem::read_file_options options;

    for (auto limit : {
             size_t{3},
             size_t{4},
         }) {
      options.max_size = limit;
      auto result = pqrs::filesystem::read_file(directory.file("file"),
                                                options);
      expect(result.has_value());
      if (result) {
        expect(*result == contents);
      }
    }

    for (auto limit : {
             size_t{0},
             size_t{2},
         }) {
      options.max_size = limit;
      auto result = pqrs::filesystem::read_file(directory.file("file"),
                                                options);
      expect(!result.has_value());
      if (!result) {
        expect(result.error().type == pqrs::filesystem::read_file_error::reason::size_limit_exceeded);
        expect(!result.error().code);
      }
    }

    directory.write("empty", {});

    options.max_size = 0;
    auto empty = pqrs::filesystem::read_file(directory.file("empty"), options);
    expect(empty.has_value());
    if (empty) {
      expect(empty->empty());
    }
  };

  "read_file missing file"_test = [] {
    temporary_directory directory;

    auto result = pqrs::filesystem::read_file(directory.file("missing"));
    expect(!result.has_value());
    if (!result) {
      expect(result.error().type == pqrs::filesystem::read_file_error::reason::open_failed);
      expect(result.error().code == std::error_code(ENOENT, std::generic_category()));
      expect(result.error().message() == "open failed: " + result.error().code.message());
    }
  };

  "read_file non-regular files"_test = [] {
    temporary_directory directory;

    expect(::mkfifo(directory.file("fifo").c_str(), 0600) == 0);

    // A FIFO without a writer must be rejected without waiting for one.
    for (const auto& path : {
             directory.path,
             directory.file("fifo"),
         }) {
      auto result = pqrs::filesystem::read_file(path);
      expect(!result.has_value());
      if (!result) {
        expect(result.error().type == pqrs::filesystem::read_file_error::reason::not_regular_file);
        expect(!result.error().code);
      }
    }
  };

  "read_file symlink"_test = [] {
    temporary_directory directory;

    const std::vector<uint8_t> contents{42, 0, 255};
    directory.write("target", contents);

    expect(::symlink("target", directory.file("link").c_str()) == 0);

    pqrs::filesystem::read_file_options options {
      .allowed_owners = std::vector<uid_t>{getuid()},
    };

    auto result = pqrs::filesystem::read_file(directory.file("link"),
                                              options);
    expect(result.has_value());
    if (result) {
      expect(*result == contents);
    }
  };

  return 0;
}
