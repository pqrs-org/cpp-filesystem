#include <iostream>
#include <pqrs/filesystem.hpp>

int main(void) {
  {
    std::string path("/usr/bin/yes");
    std::cout << "dirname(" << path << ") = "
              << pqrs::filesystem::dirname(path) << std::endl;
  }

  if (auto uid = pqrs::filesystem::uid(".")) {
    std::cout << "uid of pwd: " << *uid << std::endl;
  }

  if (auto gid = pqrs::filesystem::gid(".")) {
    std::cout << "gid of pwd: " << *gid << std::endl;
  }

  return 0;
}
