#include <boost/ut.hpp>
#include <pqrs/filesystem/osx.hpp>

int main() {
  using namespace boost::ut;
  "unreported capabilities stay unknown even when value bits are set"_test = [] {
    vol_capabilities_attr_t raw{};
    raw.capabilities[VOL_CAPABILITIES_FORMAT] = ~uint32_t(0);
    raw.capabilities[VOL_CAPABILITIES_INTERFACES] = ~uint32_t(0);
    auto result = pqrs::filesystem::osx::detail::make_volume_capabilities(raw);
    expect(!result.supports_permissions.has_value());
    expect(!result.supports_symbolic_links.has_value());
    expect(!result.supports_hard_links.has_value());
    expect(!result.case_sensitive.has_value());
    expect(!result.case_preserving.has_value());
    expect(!result.supports_extended_security.has_value());
  };
  "valid capability bits distinguish false from unknown"_test = [] {
    vol_capabilities_attr_t raw{};
    raw.valid[VOL_CAPABILITIES_FORMAT] = VOL_CAP_FMT_NO_PERMISSIONS | VOL_CAP_FMT_SYMBOLICLINKS;
    auto result = pqrs::filesystem::osx::detail::make_volume_capabilities(raw);
    expect(result.supports_permissions == std::optional(true));
    expect(result.supports_symbolic_links == std::optional(false));
    expect(!result.supports_hard_links.has_value());
    expect(!result.supports_extended_security.has_value());

    raw.capabilities[VOL_CAPABILITIES_FORMAT] = raw.valid[VOL_CAPABILITIES_FORMAT];
    raw.valid[VOL_CAPABILITIES_INTERFACES] = VOL_CAP_INT_EXTENDED_SECURITY;
    raw.capabilities[VOL_CAPABILITIES_INTERFACES] = VOL_CAP_INT_EXTENDED_SECURITY;
    result = pqrs::filesystem::osx::detail::make_volume_capabilities(raw);
    expect(result.supports_permissions == std::optional(false));
    expect(result.supports_symbolic_links == std::optional(true));
    expect(result.supports_extended_security == std::optional(true));
  };
  "osx volume metadata errors preserve errno"_test = [] {
    auto result = pqrs::filesystem::osx::get_volume_properties("/dev/null/not-a-directory");
    expect(!result.has_value());
    if (!result) expect(result.error() == std::errc::not_a_directory);
  };
}
