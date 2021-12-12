#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <stop_token>
#include <thread>

namespace bond::threads {
class worker;
class thread_pool;

class worker {
public:
  auto assign(std::function<void()> func) {
    std::unique_lock lock{mutex};
    functions_stack.push_back(func);
  }
  auto start() {
    t = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
        std::unique_lock lock{mutex};
        if (!functions_stack.empty()) {
          functions_stack.front()();
          functions_stack.pop_front();
        }
      }
    });
  }
  std::size_t work_load() noexcept {
    std::unique_lock lock{mutex};
    return functions_stack.size();
  }

  ~worker() {
    if (t.joinable() && t.get_stop_token().stop_possible()) {
      t.request_stop();
      t.join();
    }
  }

private:
  std::jthread t;
  std::list<std::function<void()>> functions_stack;
  std::mutex mutex;
};
