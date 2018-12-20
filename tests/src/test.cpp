#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <ostream>
#include <pqrs/filesystem.hpp>

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

TEST_CASE("dirname") {
  REQUIRE(pqrs::filesystem::dirname("/usr/bin/ls") == "/usr/bin");
  REQUIRE(pqrs::filesystem::dirname("/usr/bin/ls/") == "/usr/bin");
  REQUIRE(pqrs::filesystem::dirname("/usr/bin/l") == "/usr/bin");
  REQUIRE(pqrs::filesystem::dirname("/usr/bin/l/") == "/usr/bin");
  REQUIRE(pqrs::filesystem::dirname("/usr") == "/");
  REQUIRE(pqrs::filesystem::dirname("/") == "/");
  REQUIRE(pqrs::filesystem::dirname("usr/bin/ls") == "usr/bin");
  REQUIRE(pqrs::filesystem::dirname("usr/bin/") == "usr");
  REQUIRE(pqrs::filesystem::dirname("usr") == ".");
  REQUIRE(pqrs::filesystem::dirname("usr/") == ".");
  REQUIRE(pqrs::filesystem::dirname("") == ".");
}

TEST_CASE("normalize_file_path") {
  std::string file_path;

  file_path = "";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = ".";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "./";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "..";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "..");

  file_path = "../";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../");

  file_path = "..//foo";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "abcde";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde");

  file_path = "abcde/";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde/");

  file_path = "/foo//bar/../baz";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "/../foo//bar/../baz";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "foo/../bar";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "bar");

  file_path = "foo/.../bar";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/.../bar");

  file_path = "a/../b/../c/d";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "c/d");

  file_path = "a/./b/./c/d";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "a/b/c/d");

  file_path = "foo/bar/..";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo");

  file_path = "foo/bar/../";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/");

  file_path = "foo/bar/.";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "foo/bar/./";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar/");

  file_path = "../foo/bar";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar");

  file_path = "../../../foo/bar";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../../foo/bar");

  file_path = "./foo/bar";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "../foo/bar/..";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "../foo/bar///...";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar/...");

  file_path = "../a/b/../c/../d///..";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../a");

  file_path = "a/../..////../b/c";
  pqrs::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../b/c");
}

TEST_CASE("realpath") {
  auto actual = pqrs::filesystem::realpath("/bin/ls");
  REQUIRE(*actual == "/bin/ls");

  actual = pqrs::filesystem::realpath("/var/log/not_found");
  REQUIRE(actual == std::nullopt);

  actual = pqrs::filesystem::realpath("/etc/hosts");
  REQUIRE(*actual == "/private/etc/hosts");
}
