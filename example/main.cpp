#include <iostream>
#include <pqrs/filesystem.hpp>

int main(void) {
  {
    std::string path("/usr/bin/yes");
    std::cout << "dirname(" << path << ") = "
              << pqrs::filesystem::dirname(path) << std::endl;
  }

  {
    std::string path("/usr/bin/not_found/../yes");
    std::cout << "normalize_file_path(" << path << ") = "
              << pqrs::filesystem::normalize_file_path_copy(path) << std::endl;
  }

  return 0;
}
