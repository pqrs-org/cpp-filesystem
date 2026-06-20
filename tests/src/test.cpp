#include <boost/ut.hpp>
#include <fstream>
#include <ostream>
#include <pqrs/filesystem.hpp>
#include <sstream>
#include <unistd.h>

namespace {
// file size of `data/file`.
constexpr off_t data_file_size = 2230;

std::string read_file(const std::string& path) {
  std::ifstream stream(path);
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

class scoped_current_directory final {
public:
  // Keep tests that use "." scoped to a disposable directory.
  explicit scoped_current_directory(const std::string& path) {
    if (auto current_directory = pqrs::filesystem::realpath(".")) {
      current_directory_ = *current_directory;
    }

    changed_ = (chdir(path.c_str()) == 0);
  }

  ~scoped_current_directory() {
    if (!current_directory_.empty()) {
      chdir(current_directory_.c_str());
    }
  }

  [[nodiscard]] bool changed() const noexcept {
    return changed_;
  }

private:
  std::string current_directory_;
  bool changed_ = false;
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "initialize"_test = [] {
    // Ensure directory creation tests start from a clean fixture.
    system("rm -rf mkdir_example");
  };

  "exists"_test = [] {
    // Check existing, missing, and symlink paths for both files and directories.
    expect(pqrs::filesystem::exists("data/file") == true);
    expect(pqrs::filesystem::exists("data/symlink") == true);
    expect(pqrs::filesystem::exists("data/not_found") == false);
    expect(pqrs::filesystem::exists("data/not_found_symlink") == false);
    expect(pqrs::filesystem::exists("data/directory") == true);
    expect(pqrs::filesystem::exists("data/directory_symlink") == true);
  };

  "uid"_test = [] {
    // Return owner uid for an existing path and nullopt for a missing path.
    expect(pqrs::filesystem::uid("/") == 0);
    expect(pqrs::filesystem::uid("data/file") == getuid());
    expect(pqrs::filesystem::uid("data/not_found") == std::nullopt);
  };

  "gid"_test = [] {
    // Return group id for an existing path and nullopt for a missing path.
    expect(pqrs::filesystem::gid("/bin/ls") == 0);
    expect(pqrs::filesystem::gid("data/file") == getgid());
    expect(pqrs::filesystem::gid("data/not_found") == std::nullopt);
  };

  "is_directory"_test = [] {
    // Classify directories after following symlinks, while files and missing paths are false.
    expect(pqrs::filesystem::is_directory("/") == true);
    expect(pqrs::filesystem::is_directory(".") == true);
    expect(pqrs::filesystem::is_directory("..") == true);
    expect(pqrs::filesystem::is_directory("data/file") == false);
    expect(pqrs::filesystem::is_directory("data/symlink") == false);
    expect(pqrs::filesystem::is_directory("data/not_found") == false);
    expect(pqrs::filesystem::is_directory("data/not_found_symlink") == false);
    expect(pqrs::filesystem::is_directory("data/directory") == true);
    expect(pqrs::filesystem::is_directory("data/directory_symlink") == true);
  };

  "is_owned"_test = [] {
    // Follow symlinks and compare the target owner with the supplied uid.
    expect(!pqrs::filesystem::is_owned("/bin/ls", getuid()));
    expect(pqrs::filesystem::is_owned("data/file", getuid()));
    expect(!pqrs::filesystem::is_owned("data/not_found", getuid()));
    // Follow symlink
    expect(pqrs::filesystem::is_owned("data/bin-ls-symlink", 0));
  };

  "dirname"_test = [] {
    // Match dirname-like behavior for trailing slashes, root, bare names, and empty paths.
    expect(pqrs::filesystem::dirname("data/directory/file") == "data/directory");
    expect(pqrs::filesystem::dirname("data/directory/file/") == "data/directory");
    expect(pqrs::filesystem::dirname("data/not_found_directory/file") == "data/not_found_directory");
    expect(pqrs::filesystem::dirname("data/not_found_directory/file/") == "data/not_found_directory");
    expect(pqrs::filesystem::dirname("/usr") == "/");
    expect(pqrs::filesystem::dirname("/") == "/");
    expect(pqrs::filesystem::dirname("data") == ".");
    expect(pqrs::filesystem::dirname("data/") == ".");
    expect(pqrs::filesystem::dirname("") == ".");
  };

  "realpath"_test = [] {
    // Resolve existing absolute paths and return nullopt for paths that cannot be resolved.
    auto actual = pqrs::filesystem::realpath("/bin/ls");
    expect(*actual == "/bin/ls");

    actual = pqrs::filesystem::realpath("/var/log/not_found");
    expect(actual == std::nullopt);

    actual = pqrs::filesystem::realpath("/etc/hosts");
    expect(*actual == "/private/etc/hosts");
  };

  "file_access_permissions"_test = [] {
    // Report permission bits for existing targets, following symlinks, and nullopt for missing paths.
    expect(pqrs::filesystem::file_access_permissions("data/not_found") == std::nullopt);
    chmod("data/file", 0644);
    expect(pqrs::filesystem::file_access_permissions("data/file") == 0644);
    // Follow symlink
    expect(pqrs::filesystem::file_access_permissions("data/symlink") == 0644);
  };

  "file_size"_test = [] {
    // Report target file sizes, following symlinks, and nullopt for missing paths.
    expect(pqrs::filesystem::file_size("data/not_found") == std::nullopt);
    expect(pqrs::filesystem::file_size("data/file") == data_file_size);
    // Follow symlink
    expect(pqrs::filesystem::file_size("data/symlink") == data_file_size);
  };

  "create_directory_with_intermediate_directories"_test = [] {
    // Existing directories are accepted without touching paths outside the disposable fixture.
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example", 0755) == true);

    {
      // "." is supported when it points at an existing directory.
      scoped_current_directory current_directory("mkdir_example");
      expect(current_directory.changed() >> fatal);
      expect(pqrs::filesystem::create_directory_with_intermediate_directories(".", 0755) == true);
    }

    // Absolute paths are supported for existing directories.
    auto absolute_path = pqrs::filesystem::realpath("mkdir_example");
    expect((absolute_path != std::nullopt) >> fatal);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories(*absolute_path, 0755) == true);

    // Missing intermediate directories are created with the mode from that creation call.
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b", 0755) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d", 0750) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0700) == true);

    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0700);

    // Reusing an existing final directory updates only that final directory's mode.
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0755) == true);

    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0755);

    // Non-directory targets fail, while existing directory targets and directory symlinks succeed.
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/file", 0700) == false);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/file/a", 0700) == false);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/symlink", 0700) == false);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory", 0700) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory_symlink", 0700) == true);
  };

  "copy"_test = [] {
    // Copy preserves file contents and size.
    unlink("data/file.tmp");
    expect(pqrs::filesystem::file_size("data/file.tmp") == std::nullopt);
    pqrs::filesystem::copy("data/file", "data/file.tmp");
    expect(pqrs::filesystem::file_size("data/file.tmp") == data_file_size);
    expect(read_file("data/file.tmp") == read_file("data/file"));
    unlink("data/file.tmp");

    // Missing sources are ignored and do not create the destination.
    pqrs::filesystem::copy("data/not_found", "data/file.tmp");
    expect(pqrs::filesystem::file_size("data/file.tmp") == std::nullopt);
  };

  return 0;
}
