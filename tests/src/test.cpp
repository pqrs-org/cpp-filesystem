#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <ostream>
#include <pqrs/filesystem.hpp>

namespace {
// file size of `data/file`.
off_t data_file_size = 2230;
} // namespace

TEST_CASE("initialize") {
  system("rm -rf mkdir_example");
}

TEST_CASE("exists") {
  REQUIRE(pqrs::filesystem::exists("data/file") == true);
  REQUIRE(pqrs::filesystem::exists("data/symlink") == true);
  REQUIRE(pqrs::filesystem::exists("data/not_found") == false);
  REQUIRE(pqrs::filesystem::exists("data/not_found_symlink") == false);
  REQUIRE(pqrs::filesystem::exists("data/directory") == true);
  REQUIRE(pqrs::filesystem::exists("data/directory_symlink") == true);
}

TEST_CASE("uid") {
  REQUIRE(pqrs::filesystem::uid("/") == 0);
  REQUIRE(pqrs::filesystem::uid("data/not_found") == std::nullopt);
}

TEST_CASE("gid") {
  REQUIRE(pqrs::filesystem::gid("/bin/ls") == 0);
  REQUIRE(pqrs::filesystem::gid("data/not_found") == std::nullopt);
}

TEST_CASE("is_directory") {
  REQUIRE(pqrs::filesystem::is_directory("/") == true);
  REQUIRE(pqrs::filesystem::is_directory(".") == true);
  REQUIRE(pqrs::filesystem::is_directory("..") == true);
  REQUIRE(pqrs::filesystem::is_directory("data/file") == false);
  REQUIRE(pqrs::filesystem::is_directory("data/symlink") == false);
  REQUIRE(pqrs::filesystem::is_directory("data/not_found") == false);
  REQUIRE(pqrs::filesystem::is_directory("data/not_found_symlink") == false);
  REQUIRE(pqrs::filesystem::is_directory("data/directory") == true);
  REQUIRE(pqrs::filesystem::is_directory("data/directory_symlink") == true);
}

TEST_CASE("is_owned") {
  REQUIRE(!pqrs::filesystem::is_owned("/bin/ls", getuid()));
  REQUIRE(pqrs::filesystem::is_owned("data/file", getuid()));
  REQUIRE(!pqrs::filesystem::is_owned("data/not_found", getuid()));
  // Follow symlink
  REQUIRE(!pqrs::filesystem::is_owned("data/bin-ls-symlink", getuid()));
}

TEST_CASE("dirname") {
  REQUIRE(pqrs::filesystem::dirname("data/directory/file") == "data/directory");
  REQUIRE(pqrs::filesystem::dirname("data/directory/file/") == "data/directory");
  REQUIRE(pqrs::filesystem::dirname("data/not_found_directory/file") == "data/not_found_directory");
  REQUIRE(pqrs::filesystem::dirname("data/not_found_directory/file/") == "data/not_found_directory");
  REQUIRE(pqrs::filesystem::dirname("/usr") == "/");
  REQUIRE(pqrs::filesystem::dirname("/") == "/");
  REQUIRE(pqrs::filesystem::dirname("data") == ".");
  REQUIRE(pqrs::filesystem::dirname("data/") == ".");
  REQUIRE(pqrs::filesystem::dirname("") == ".");
}

TEST_CASE("realpath") {
  auto actual = pqrs::filesystem::realpath("/bin/ls");
  REQUIRE(*actual == "/bin/ls");

  actual = pqrs::filesystem::realpath("/var/log/not_found");
  REQUIRE(actual == std::nullopt);

  actual = pqrs::filesystem::realpath("/etc/hosts");
  REQUIRE(*actual == "/private/etc/hosts");
}

TEST_CASE("file_access_permissions") {
  REQUIRE(pqrs::filesystem::file_access_permissions("data/not_found") == std::nullopt);
  chmod("data/file", 0644);
  REQUIRE(pqrs::filesystem::file_access_permissions("data/file") == 0644);
  // Follow symlink
  REQUIRE(pqrs::filesystem::file_access_permissions("data/symlink") == 0644);
}

TEST_CASE("file_size") {
  REQUIRE(pqrs::filesystem::file_size("data/not_found") == std::nullopt);
  REQUIRE(pqrs::filesystem::file_size("data/file") == data_file_size);
  // Follow symlink
  REQUIRE(pqrs::filesystem::file_size("data/symlink") == data_file_size);
}

TEST_CASE("create_directory_with_intermediate_directories") {
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("/", 0700) == true);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories(".", 0700) == true);

  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b", 0755) == true);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d", 0750) == true);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0700) == true);

  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0700);

  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0755) == true);

  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b")) == 0755);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c")) == 0750);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d")) == 0750);
  REQUIRE(*(pqrs::filesystem::file_access_permissions("mkdir_example/a/b/c/d/e")) == 0755);

  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("data/file", 0700) == false);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("data/symlink", 0700) == false);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory", 0700) == true);
  REQUIRE(pqrs::filesystem::create_directory_with_intermediate_directories("data/directory_symlink", 0700) == true);
}

TEST_CASE("copy") {
  unlink("data/file.tmp");
  REQUIRE(pqrs::filesystem::file_size("data/file.tmp") == std::nullopt);
  pqrs::filesystem::copy("data/file", "data/file.tmp");
  REQUIRE(pqrs::filesystem::file_size("data/file.tmp") == data_file_size);
  unlink("data/file.tmp");
}
