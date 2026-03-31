#pragma once

#include <atomic>
#include <cstddef>

// Single Producer Single Consumer lock-free ring buffer.
// Template parameters:
//   T    - element type
//   Size - capacity (must be > 0)
//
// Realtime-safe: no heap allocations, no mutex, no exceptions.
template<typename T, size_t Size>
class LockFreeQueue
{
  static_assert(Size > 0, "LockFreeQueue Size must be greater than 0");

public:
  LockFreeQueue()
    : mHead(0)
    , mTail(0)
  {
  }

  // Push an item. Returns false if the queue is full.
  bool push(const T& item)
  {
    const size_t tail = mTail.load(std::memory_order_relaxed);
    const size_t nextTail = (tail + 1) % (Size + 1);

    if (nextTail == mHead.load(std::memory_order_acquire))
      return false; // full

    mBuffer[tail] = item;
    mTail.store(nextTail, std::memory_order_release);
    return true;
  }

  // Pop an item. Returns false if the queue is empty.
  bool pop(T& item)
  {
    const size_t head = mHead.load(std::memory_order_relaxed);

    if (head == mTail.load(std::memory_order_acquire))
      return false; // empty

    item = mBuffer[head];
    mHead.store((head + 1) % (Size + 1), std::memory_order_release);
    return true;
  }

  // Returns true if the queue is empty.
  bool empty() const
  {
    return mHead.load(std::memory_order_acquire) == mTail.load(std::memory_order_acquire);
  }

  // Returns true if the queue is full.
  bool full() const
  {
    const size_t tail = mTail.load(std::memory_order_acquire);
    const size_t nextTail = (tail + 1) % (Size + 1);
    return nextTail == mHead.load(std::memory_order_acquire);
  }

private:
  // One extra slot to distinguish full from empty without a counter.
  T mBuffer[Size + 1];
  std::atomic<size_t> mHead;
  std::atomic<size_t> mTail;
};
