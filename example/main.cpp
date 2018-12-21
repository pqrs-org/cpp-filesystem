#include <iostream>
#include <pqrs/filesystem.hpp>

int main(void) {
  {
    std::string path("/usr/bin/yes");
    std::cout << "dirname(" << path << ") = "
              << pqrs::filesystem::dirname(path) << std::endl;
  }

  return 0;
}
