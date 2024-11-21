#include <chrono>
#include <iostream>
#include <thread>

int main() {
  std::chrono::seconds duration1(10);
  std::cout << duration1.count() << std::endl;

  // 将 秒 转成 毫秒
  std::cout
      << std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count()
      << std::endl;

  // steady_clock 用于精确计算interval
  auto time = std::chrono::steady_clock::now().time_since_epoch();
  std::cout << time.count() << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto time2 = std::chrono::steady_clock::now().time_since_epoch();
  std::cout << time2.count() - time.count() << std::endl;

  auto time3 = std::chrono::system_clock::now().time_since_epoch();
  std::cout << time3.count() << std::endl;

  return 0;
}