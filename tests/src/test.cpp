#include <boost/ut.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <pqrs/filesystem.hpp>
#include <sstream>
#include <sys/stat.h>
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
    auto current_directory = pqrs::filesystem::realpath(".");
    expect((current_directory != std::nullopt) >> fatal);

    auto actual = pqrs::filesystem::realpath("data/file");
    expect(*actual == *current_directory + "/data/file");

    actual = pqrs::filesystem::realpath("/var/log/not_found");
    expect(actual == std::nullopt);

    actual = pqrs::filesystem::realpath("data/symlink");
    expect(*actual == *current_directory + "/data/file");
  };

  "file_access_permissions"_test = [] {
    // Report permission bits for existing targets, following symlinks, and nullopt for missing paths.
    expect(pqrs::filesystem::file_access_permissions("data/not_found") == std::nullopt);
    chmod("data/file", 0644);
    expect(pqrs::filesystem::file_access_permissions("data/file") == 0644);
    // Follow symlink
    expect(pqrs::filesystem::file_access_permissions("data/symlink") == 0644);
  };

  "copy"_test = [] {
    // Copy preserves file contents and size.
    unlink("data/file.tmp");
    expect(std::filesystem::exists("data/file.tmp") == false);
    pqrs::filesystem::copy("data/file",
                           "data/file.tmp");
    expect(std::filesystem::file_size("data/file.tmp") == data_file_size);
    expect(read_file("data/file.tmp") == read_file("data/file"));
    unlink("data/file.tmp");

    // Missing sources are ignored and do not create the destination.
    pqrs::filesystem::copy("data/not_found",
                           "data/file.tmp");
    expect(std::filesystem::exists("data/file.tmp") == false);
  };

  return 0;
}
