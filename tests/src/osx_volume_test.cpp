#include <boost/ut.hpp>
#include <pqrs/filesystem/osx.hpp>

int main() {
  using namespace boost::ut;
  "osx volume metadata errors preserve errno"_test = [] {
    auto result = pqrs::filesystem::osx::get_volume_properties("/dev/null/not-a-directory");
    expect(!result.has_value());
    if (!result) expect(result.error() == std::errc::not_a_directory);
  };
}
