#include <iostream>
#include <utility>
#include <string>

#include <folly/Utility.h>

template <class T>
struct Base1
{
  explicit Base1(T v) noexcept
  {
    std::cout << "Base1(int)" << std::endl;
  }
  Base1() noexcept
  {
    std::cout << "Base1()" << std::endl;
  }
  ~Base1() noexcept
  {
    std::cout << "~Base1()" << std::endl;
  }

  Base1(Base1 &&) noexcept
  {
    std::cout << "Base1(Base1&&)" << std::endl;
  }
  Base1 &operator=(Base1 &&) noexcept = default;
  Base1(const Base1 &) = delete;
  Base1 &operator=(const Base1 &) = delete;
};

struct Base2
{
  explicit Base2(const std::string& s) noexcept
  {
    std::cout << "Base2(const std::string&)" << std::endl;
  }

  Base2() noexcept
  {
    std::cout << "Base2()" << std::endl;
  }
  ~Base2() noexcept
  {
    std::cout << "~Base2()" << std::endl;
  }
};

template <class T>
class Dummy: public Base1<T>, public Base2 {

public:
  // 下列申明用于表示继承父类的构造函数，注意：Base1 和 Base2中不能
  // 存在参数签名相同的构造函数，否则编译时会报二义性错误。
  using Base1<T>::Base1;
  using Base2::Base2;
};

class Student : public Base1<bool>, public Base2
{
};

class Teacher : public Base1<bool>, public Base2
{
public:
  // 默认调用Base1::Base1() 和 Base2::Base2()
  Teacher() {}

  //
  // 这里不显式调用父类构造函数时，默认调用Base1::Base1()和Base2::Base2()。
  //
  // 如果我们不提供自己的复制构造函数，则默认生成的情况下，将会调用Base1::Base1(const Base1&)
  // 和Base2::Base2(const Base2&)。这时编译将会报错，因为Base1::Base1(const Base1&)标记
  // 为deleted。
  // 
  Teacher(const Teacher &t)
  {

  }
};

int main()
{
  {
    Dummy<int>();
  }
  {
    // 这里会调用父类的构造函数: Base1::Base1(int) 和 Base1::Base2()
    Dummy<int>(100);
  }
  {
    Dummy<int>("test");
  }
  {
    // 下面编译不过，集成的构造函数存在冲突
    // Dummy<std::string>("test");
  }
  {
    Student stud;
    Student stud2 = std::move(stud);

    // 编译不通过: class 'Base1' has a deleted copy constructor
    // Student stud3 = stud2;
  }
  {
    Teacher teacher;
    Teacher teacher2 = teacher;
  }
  return 0;
}