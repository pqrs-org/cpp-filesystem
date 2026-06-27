#include <boost/ut.hpp>
#include <fstream>
#include <optional>
#include <ostream>
#include <pqrs/filesystem.hpp>
#include <sys/stat.h>
#include <unistd.h>

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

  return 0;
}
