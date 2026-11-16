#include <boost/ut.hpp>
#include <cstdlib>
#include <fstream>
#include <pqrs/filesystem.hpp>

int main() {
  using namespace boost::ut;
  "effective access checks"_test = [] {
    char pattern[] = "/tmp/cpp-filesystem-access-XXXXXX";
    auto directory = ::mkdtemp(pattern);
    expect(directory != nullptr);
    if (!directory) return;
    auto file = std::filesystem::path(directory) / "file";
    auto missing = pqrs::filesystem::is_writable_by_current_process(file);
    auto unreadable_missing = pqrs::filesystem::is_readable_by_current_process(file);
    expect(!unreadable_missing.has_value());
    if (!unreadable_missing) expect(unreadable_missing.error() == std::errc::no_such_file_or_directory);
    expect(!missing.has_value());
    if (!missing) expect(missing.error() == std::errc::no_such_file_or_directory);
    std::ofstream(file) << "test";
    expect(pqrs::filesystem::is_writable_by_current_process(file).value_or(false));
    expect(pqrs::filesystem::is_readable_by_current_process(file).value_or(false));
    expect(pqrs::filesystem::is_executable_by_current_process(directory).value_or(false));
    if (::geteuid() != 0) {
      expect(::chmod(file.c_str(), 0200) == 0);
      auto unreadable = pqrs::filesystem::is_readable_by_current_process(file);
      expect(unreadable.has_value() && !*unreadable);
      expect(::chmod(file.c_str(), 0600) == 0);
      expect(pqrs::filesystem::is_executable_by_current_process(file) == std::expected<bool, std::error_code>(false));
      expect(::chmod(file.c_str(), 0700) == 0);
      expect(pqrs::filesystem::is_executable_by_current_process(file).value_or(false));
      expect(::chmod(file.c_str(), 0400) == 0);
      auto denied = pqrs::filesystem::is_writable_by_current_process(file);
      expect(denied.has_value() && !*denied);
      expect(::chmod(directory, 0600) == 0);
      auto search = pqrs::filesystem::is_executable_by_current_process(directory);
      expect(search.has_value() && !*search);
      expect(::chmod(directory, 0700) == 0);
    }
    std::filesystem::remove(file);
    std::filesystem::remove(directory);
  };
#if defined(__APPLE__)
#if TARGET_OS_OSX
  "volume metadata errors preserve errno"_test = [] {
    auto result = pqrs::filesystem::osx::get_volume_properties("/dev/null/not-a-directory");
    expect(!result.has_value());
    if (!result) expect(result.error() == std::errc::not_a_directory);
  };
#endif
#endif
}
