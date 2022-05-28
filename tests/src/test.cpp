#include <boost/ut.hpp>
#include <ostream>
#include <pqrs/filesystem.hpp>

namespace {
// file size of `data/file`.
off_t data_file_size = 2230;
} // namespace

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "initialize"_test = [] {
    system("rm -rf mkdir_example");
  };

  "exists"_test = [] {
    expect(pqrs::filesystem::exists("data/file") == true);
    expect(pqrs::filesystem::exists("data/symlink") == true);
    expect(pqrs::filesystem::exists("data/not_found") == false);
    expect(pqrs::filesystem::exists("data/not_found_symlink") == false);
    expect(pqrs::filesystem::exists("data/directory") == true);
    expect(pqrs::filesystem::exists("data/directory_symlink") == true);
  };

  "uid"_test = [] {
    expect(pqrs::filesystem::uid("/") == 0);
    expect(pqrs::filesystem::uid("data/not_found") == std::nullopt);
  };

  "gid"_test = [] {
    expect(pqrs::filesystem::gid("/bin/ls") == 0);
    expect(pqrs::filesystem::gid("data/not_found") == std::nullopt);
  };

  "is_directory"_test = [] {
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
    expect(!pqrs::filesystem::is_owned("/bin/ls", getuid()));
    expect(pqrs::filesystem::is_owned("data/file", getuid()));
    expect(!pqrs::filesystem::is_owned("data/not_found", getuid()));
    // Follow symlink
    expect(!pqrs::filesystem::is_owned("data/bin-ls-symlink", getuid()));
  };

  "dirname"_test = [] {
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
    auto actual = pqrs::filesystem::realpath("/bin/ls");
    expect(*actual == "/bin/ls");

    actual = pqrs::filesystem::realpath("/var/log/not_found");
    expect(actual == std::nullopt);

    actual = pqrs::filesystem::realpath("/etc/hosts");
    expect(*actual == "/private/etc/hosts");
  };

  "file_access_permissions"_test = [] {
    expect(pqrs::filesystem::file_access_permissions("data/not_found") == std::nullopt);
    chmod("data/file", 0644);
    expect(pqrs::filesystem::file_access_permissions("data/file") == 0644);
    // Follow symlink
    expect(pqrs::filesystem::file_access_permissions("data/symlink") == 0644);
  };

  "file_size"_test = [] {
    expect(pqrs::filesystem::file_size("data/not_found") == std::nullopt);
    expect(pqrs::filesystem::file_size("data/file") == data_file_size);
    // Follow symlink
    expect(pqrs::filesystem::file_size("data/symlink") == data_file_size);
  };

  "create_directory_with_intermediate_directories"_test = [] {
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("/", 0700) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories(".", 0700) == true);

    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b", 0755) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d", 0750) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0700) == true);

    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0700);

    expect(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0755) == true);

    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
    expect(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0755);

    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/file", 0700) == false);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/symlink", 0700) == false);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory", 0700) == true);
    expect(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory_symlink", 0700) == true);
  };

  "copy"_test = [] {
    unlink("data/file.tmp");
    expect(pqrs::filesystem::file_size("data/file.tmp") == std::nullopt);
    pqrs::filesystem::copy("data/file", "data/file.tmp");
    expect(pqrs::filesystem::file_size("data/file.tmp") == data_file_size);
    unlink("data/file.tmp");
  };

  return 0;
}
