#include <folly/Try.h>
#include <folly/executors/ThreadedExecutor.h>
#include <folly/futures/Future.h>
#include <folly/small_vector.h>

#include <boost/thread.hpp>
#include <iostream>
#include <utility>

using namespace folly;
using namespace folly::small_vector_policy;
using namespace std;

Try<int> foo(int x) {
  // do something with x
  cout << "foo(" << x << ")" << endl;
  return Try(x * x);
}

void testFuture() {
  // ...
  folly::ThreadedExecutor executor;
  cout << "making Promise" << endl;
  Promise<int> p;
  Future<int> f = p.getSemiFuture().via(&executor);

  // 虽然foo返回的是Try<int>, 但thenValue支持自动将其展开
  Future<int> f2 = std::move(f).thenValue(foo);

  cout << "Future chain made" << endl;

  std::move(f2).thenValue([](int v) {
    std::cout << "f2 value: " << v << std::endl;
  });

  // ... now perhaps in another event callback

  cout << "fulfilling Promise" << endl;
  p.setValue(42);
  sleep(1);
  
  // 上面已经执行了std::move(f2), 这里不应该再使用f2
  // cout << "Promise fulfilled: " << f2.value() << endl;

  Try<int> t(200);
  std::cout << "try: " << *t << std::endl;
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

int main() {
  checkFirstArg(foo);
  testFuture();
  testSmallVector();
  return 0;
}