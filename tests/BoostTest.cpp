#include <boost/operators.hpp>
#include <iostream>

struct Person {
  int age{10};
};

class Student: public boost::totally_ordered1<Student, Person> {

public:
  bool operator < (const Student& other) const {
    return false;
  }

  bool operator == (const Student& other) const {
    return false;
  }
};

int main() {
  Student s1;
  Student s2;
  std::cout << (s1 >= s2) << std::endl;
  std::cout << s1.age << std::endl;

  const Person& p = s1;
  std::cout << p.age << std::endl;
  return 0;
}