#include <fstream>
#include <iostream>
#include <pqrs/filesystem/osx.hpp>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
  bool skip = argc == 3 && std::string(argv[2]) == "--skip-exfat";
  bool restricted = argc == 3 && std::string(argv[2]) == "--restricted";
  if (argc != 2 && !skip && !restricted) return 2;
  const auto root = std::filesystem::path(argv[1]);
  if (restricted) {
    for (auto path : {root / "apfs-owners", root / "apfs-owners" / "file"}) {
      auto result = pqrs::filesystem::osx::get_volume_properties(path);
      if (!result || !result->mount_flags.read_only || !result->mount_flags.no_exec || !result->mount_flags.no_suid) {
        std::cerr << "FAIL: read_only/no_exec/no_suid: " << path << '\n';
        return 1;
      }
      std::cout << "PASS: read_only/no_exec/no_suid: " << path << '\n';
    }
    return 0;
  }
  int failures = 0;
  auto check = [&](const auto& path, bool supports, bool ignored) {
    auto result = pqrs::filesystem::osx::get_volume_properties(path);
    if (!result || result->capabilities.supports_permissions != std::optional(supports) || result->mount_flags.ownership_ignored != ignored ||
        // FAT/exFAT drivers may report symbolic-link support independently of permissions.
        !result->capabilities.supports_symbolic_links.has_value() ||
        result->capabilities.supports_hard_links != std::optional(supports) ||
        result->capabilities.supports_extended_security != std::optional(supports) ||
        result->capabilities.case_sensitive != std::optional(false) ||
        result->capabilities.case_preserving != std::optional(true) || result->mount_flags.read_only || result->mount_flags.no_exec || !result->mount_flags.local) {
      std::cerr << "FAIL: " << path;
      if (!result) std::cerr << ": " << result.error().message();
      if (result) {
        auto value = [](auto v) { return v ? static_cast<int>(*v) : -1; };
        std::cerr << " symbolic_links=" << value(result->capabilities.supports_symbolic_links)
                  << " hard_links=" << value(result->capabilities.supports_hard_links)
                  << " extended_security=" << value(result->capabilities.supports_extended_security)
                  << " case_sensitive=" << value(result->capabilities.case_sensitive)
                  << " case_preserving=" << value(result->capabilities.case_preserving)
                  << " readonly=" << result->mount_flags.read_only << " noexec=" << result->mount_flags.no_exec << " local=" << result->mount_flags.local;
      }
      std::cerr << '\n';
      ++failures;
    } else {
      std::cout << "PASS: " << path << '\n';
    }
  };
  try {
    for (auto name : {"apfs-owners", "apfs-noowners", "fat32", "exfat"}) {
      if (skip && std::string(name) == "exfat") continue;
      auto volume = root / name;
      bool supports = std::string(name).starts_with("apfs-");
      bool ignored = std::string(name) != "apfs-owners";
      check(volume, supports, ignored);
      auto file = volume / "file";
      std::ofstream output(file);
      output << "{}";
      output.close();
      if (!output) throw std::runtime_error("Failed to create " + file.string());
      check(file, supports, ignored);
      auto link = root / "apfs-owners" / (std::string("link-") + name);
      std::filesystem::create_symlink(file, link);
      check(link, supports, ignored);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return failures ? 1 : 0;
}
