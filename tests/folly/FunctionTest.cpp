#include <functional>
#include <iostream>
#include <folly/Function.h>
#include <string>

using namespace std;

class FooBar {
 public:
  void callFunc() const {
    if (func_) {
      func_();
    }
    if (age > 0) {
      std::cout << age << std::endl;
    }
    // 无法编译
    // age = 20;

    // 可以修改mutable字段
    count = 10;
  }
 private:
  std::function<void()> func_;
  int age{10};
  mutable int count{0};
};

void runFunction(std::function<void()>&& f) {
  f();
}

void runFollyFunction(folly::Function<void()>&& f) {
  f();
}

int main() {
  FooBar foo;
  foo.callFunc();

  {
    // 下面不能使用std::unique_ptr, 否则编译报错
    // copy constructor is implicitly deleted because 'unique_ptr<std::string>' has a 
    // user-declared move constructor unique_ptr(unique_ptr&& __u) _NOEXCEPT
    // std::unique_ptr<std::string> ptr(new string("mytest"));
    std::shared_ptr<std::string> ptr(new string("mytest"));
    auto func1 = [p = std::move(ptr)]() {
      std::cout << *p << std::endl;
    };
    runFunction(func1);
  }

  {
    std::unique_ptr<std::string> ptr(new string("mytest"));
    folly::Function<void()> func = [p = std::move(ptr)]() {
      std::cout << *p << std::endl;
    };
    runFollyFunction(std::move(func));
  }
}