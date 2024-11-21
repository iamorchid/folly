#include <string>
#include <iostream>

int main() {
  const char *test = "test abc";
  std::string str(test, 4);
  std::cout << str.c_str() << std::endl;
  str[2] = 'E';
  std::cout << str.c_str() << std::endl;

  std::cout << test << std::endl;
  return 0;
}