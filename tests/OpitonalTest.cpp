#include <iostream>
#include <optional>
#include <string>
#include <memory>

using OptType=std::optional<std::shared_ptr<int32_t>>;

int main() {
  int32_t result;
  bool overflow = __builtin_add_overflow(int32_t(1500000000), int32_t(1500000000), &result);
  std::cout << overflow << ", " << result << std::endl;

  uint32_t result2;
  bool overflow2 = __builtin_add_overflow(uint32_t(1500000000), uint32_t(1500000000), &result2);
  std::cout << overflow2 << ", " << result2 << std::endl;

  OptType opt1 = std::make_shared<int32_t>(20);
  if (opt1) {
    const auto& ptr = *opt1;
    if (ptr) {
      std::cout << "opt1's ptr value: " << *ptr << std::endl;
    } else {
      std::cout << "opt1's ptr value is null" << std::endl;
    }
  } else {
    std::cout << "opt1 has no ptr value"<< std::endl;
  }

  // std::optional的构造函数会用null构造shared_ptr，即类似：
  // std::shared_ptr<int32_t> ptr = null;
  OptType opt2 = nullptr;
  if (opt2) {
    const auto& ptr = *opt2;
    if (ptr) {
      std::cout << "opt2's ptr value: " << *ptr << std::endl;
    } else {
      std::cout << "opt2's ptr value is null" << std::endl;
    }
  } else {
    std::cout << "opt1 has no ptr value"<< std::endl;
  }

  OptType opt3 = std::nullopt;
  if (opt3) {
    std::cout << "opt3: " << *opt3 << std::endl;
  } else {
    std::cout << "opt1 has no ptr value"<< std::endl;
  }
}