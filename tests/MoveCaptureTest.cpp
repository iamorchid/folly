#include <iostream>
#include <memory>
#include <utility>
#include <vector>

void exec(std::function<void()> f) { f(); }

int main() {
  std::vector<int> ints{0, 1, 2, 3};
  std::cout << "[main#1] vector size: " << ints.size() << std::endl;

  auto f1 = [ints]() {
    std::cout << "[lambda#1] vector size: " << ints.size() << std::endl;
  };

  std::cout << "[main#2] vector size: " << ints.size() << std::endl;
  f1();
  std::cout << "[main#3] vector size: " << ints.size() << std::endl;

  // 这里创建lambda实例后，就已经将main函数中的ints给move走了。
  auto f2 = [ints = std::move(ints)]() {
    std::cout << "[lambda#2] vector size: " << ints.size() << std::endl;
  };

  // 到这里后，main函数中的ints已经被掏空了
  std::cout << "[main#4] vector size: " << ints.size() << std::endl;
  f2();
  std::cout << "[main#5] vector size: " << ints.size() << std::endl;

  std::unique_ptr<int> ptr = std::make_unique<int>(100);
  auto f3 = [ptr = std::move(ptr)]() {
    std::cout << "ptr value: " << *ptr << std::endl;
  };
  f3();

  exec(f2);

  // 编译报错：copy constructor is implicitly deleted because 'unique_ptr<int>'
  // has a user-declared move constructor exec(f3);
}