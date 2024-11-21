#include <iostream>
#include <functional>
#include <string>

class Student {
 private:
  int age_;

 public:
  Student(): age_(3) {
    std::cout << "call Student()" << std::endl;
  }
  Student(int age) : age_(age) {
    std::cout << "call Student(int)" << std::endl;
  }
  Student(const Student& other) : age_(other.age_) {
    std::cout << "call Student(const Student&)" << std::endl;
  }

  Student(Student&& other) = default;

  int getAge() {
    return age_;
  }
};

class School {
private:
  Student s;
};

void test1() {
  const Student s(10);

  // 调用const对象的非const方法，无法编译
  // s.getAge();

  Student s1 = s;
  std::cout << s1.getAge() << std::endl;

  Student s2 = std::move(s1);
  std::cout << s2.getAge() << std::endl;

  School school1;
  School school2 = school1;

  // 这里会调用Student的move构造函数
  School school3 = std::move(school1);
}

template <class T>
void outputUpperSingle(T&& arg) {
  std::cout << arg << std::endl;
}

template <class... T>
void outputUpperMany(T&&... args) {
  int arr[] = {(outputUpperSingle(std::forward<T>(args)), 0)...};
}

struct Person {
  std::string name;
  int age;
};

int main() {
  outputUpperMany("abc", "efg");

  std::make_tuple("1", 1);

  std::vector<uint64_t> vec;
  std::cout << vec.data() << std::endl;

  vec.resize(0, 0);
  std::cout << vec.data() << std::endl;

  std::vector<uint64_t> vec1{1, 2 , 3};
  std::vector<uint64_t> vec2{2};
  std::cout << (vec1 < vec2) << std::endl;

  Person p = {"chuzhi", 30};
  auto& [name, age] = p;
  std::cout << name << ", " << age << std::endl;

  name = "Will Zhang";
  age = 20;
  std::cout << p.name << ", " << p.age << std::endl;

  return 0;
}