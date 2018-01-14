#pragma once

#include <mutex>
#include <queue>
#include <memory>

namespace nodePd {

/**
 * Thread safe locked queue of shared pointers.
 * adapted (simplified) from: http://coliru.stacked-crooked.com/a/d04973a3cf6ba8a5
 */
template<typename T>
class LockedQueue {
  public:
    explicit LockedQueue()
      : mut()
      , data_queue()
    {}

    virtual ~LockedQueue() = default;

    /**
     * add an element to th queue
     */
    void push(std::shared_ptr<T> value) {
      std::lock_guard<std::mutex> lock(mut);
      data_queue.push(value);
    }

    /**
     * return first element of the queue, nullptr if empty
     */
    std::shared_ptr<T> pop() {
      std::lock_guard<std::mutex> lock(mut);

      if (data_queue.empty()) {
        return nullptr;
      }

      auto res = data_queue.front();
      data_queue.pop();

      return res;
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(mut);
      return data_queue.empty();
    }

    int size() const {
      std::lock_guard<std::mutex> lock(mut);
      return data_queue.size();
    }

  private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T>> data_queue;
};

}; // namespace
