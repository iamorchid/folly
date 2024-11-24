#include <folly/Try.h>
#include <folly/executors/ThreadedExecutor.h>
#include <folly/futures/Future.h>
#include <folly/small_vector.h>
#include "folly/Conv.h"

#include <boost/thread.hpp>
#include <iostream>
#include <utility>
#include <exception>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace folly;
using namespace folly::small_vector_policy;
using namespace std;

int fooVal1(int x) {
  // do something with x
  cout << "fooVal1(" << x << ")" << endl;
  return x * x;
}

Try<int> fooVal2(int x) {
  // do something with x
  if (x < 100) {
    cout << "fooVal2(" << x << ")" << endl;
    return Try(x * x);
  }
  // 对应: explicit TryBase(exception_wrapper e) noexcept
  // 这里用到了将std::invalid_argument隐式转成了exception_wrapper
  return Try<int>(std::invalid_argument("invalid x: " + std::to_string(x)));
}

Try<int> fooTry(Try<int> x) {
  // do something with x
  cout << "fooTry(" << x.value() << ")" << endl;
  return Try(x.value() * x.value());
}

void testFuture(int value) {
  // ...
  folly::ThreadedExecutor executor;
  auto [p, f] = folly::makePromiseContract<int>(&executor);

  // Future<int> f2 = std::move(f).thenValue(fooVal1);

  // 虽然foo返回的是Try<int>, 但thenValue支持自动将其展开
  Future<int> f2 = std::move(f).thenValue(fooVal2);

  // 无法通过编译
  // Future<int> f2 = std::move(f).thenValue(fooTry);

  // 正确用法
  // Future<int> f2 = std::move(f).thenTry(fooTry);

  // 无法编译, thenValue和thenTry返回的内部func的result
  // 的inner类型. 对于Try<int>, 其inner类型为int.
  // Future<Try<int>> f2 = std::move(f).thenTry(fooTry);

  cout << "Future chain made" << endl;

  std::move(f2)
    .thenValue([](int v) {
      std::cout << "f2 value: " << v << std::endl;
    })
    .thenError(
      folly::tag_t<std::invalid_argument>{},
      [](auto&& e) {
        std::cout << "error: " << e.what() << std::endl;
      }
    );

  // ... now perhaps in another event callback

  cout << "fulfilling Promise" << endl;
  p.setValue(value);
  sleep(1);
  
  // 上面已经执行了std::move(f2), 这里不应该再使用f2
  // cout << "Promise fulfilled: " << f2.value() << endl;

  Try<int> t(200);
  std::cout << "try: " << *t << std::endl;
}

void testFuture2(int value) {
  folly::ThreadedExecutor executor;
  auto [p, f] = folly::makePromiseContract<int>(&executor);

  auto sendHttpReq = [&](int arg) -> Future<int> {
    std::cout << "send http# now ..." << std::endl;

    auto [hp, hf] = folly::makePromiseContract<int>(&executor);
    executor.add([hp_ = std::move(hp), arg]() mutable {
      // mock http req#1 delay
      std::this_thread::sleep_for(std::chrono::seconds(3));
      hp_.setValue(arg * 1000);
    });

    auto hf2_ = std::move(hf).thenValue([&](int arg) -> Future<int> {
      auto [hp2, hf2] = folly::makePromiseContract<int>(&executor);
      executor.add([hp2_ = std::move(hp2), arg]() mutable {
        // mock http req#2 delay
        std::this_thread::sleep_for(std::chrono::seconds(3));
        hp2_.setValue(arg * 8);
      });
      return std::move(hf2);
    });

    // 结果何时ready, 并不确定
    return std::move(hf2_);
  };

  // 这里的thenValue返回一个Future对象
  auto f2 = std::move(f).thenValue([&](int v) -> Future<int> {
    auto square = v * v;
    return sendHttpReq(square);
  });

  p.setValue(value);
  std::cout << "p.setValue called" << std::endl;

  auto val = std::move(f2).get();
  std::cout << "val: " << val << std::endl;
}

void testSmallVector() {
  // small_vector<int, 2, policy_in_situ_only<false>> ints;
  small_vector<int, 2, policy_size_type<int8_t>> ints;
  ints.emplace_back(1);
  ints.emplace_back(2);
  ints.emplace_back(3);
  ints.emplace_back(3);
  std::cout << ints.size() << std::endl;
}

template <typename F>
void checkFirstArg(F&& f) {
  // 这里为true
  std::cout << std::is_same_v<typename futures::detail::valueCallableResult<int, F>::FirstArg, int&&> << std::endl;
}

int main(int argc, char* argv[]) {
  checkFirstArg(fooVal1);

  if (argc <= 1) {
    std::cout << "no argument no provided" << std::endl;
    return -1;
  }
  // testFuture(folly::to<int>(argv[1]));
  testFuture2(folly::to<int>(argv[1]));

  // testSmallVector();

  return 0;
}