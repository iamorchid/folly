#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>

#include <iostream>

int main(int argc, char* argv[]) {
  // Initialize Folly
  folly::Init init(&argc, &argv);

  // Create a new EventBase
  folly::EventBase eventBase;

  // Schedule a task to run asynchronously
  eventBase.runInEventBaseThread(
      []() { std::cout << "Hello from EventBase!" << std::endl; });

  // Loop until there are no more scheduled events
  eventBase.loop();

  std::cout << "EventBase loop exited." << std::endl;

  return 0;
}
