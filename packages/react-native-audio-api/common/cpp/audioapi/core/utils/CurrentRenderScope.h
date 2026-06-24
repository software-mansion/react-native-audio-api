#pragma once
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>

namespace {

struct CurrentRenderScope {
  explicit CurrentRenderScope(std::atomic<uint32_t> &counter) : counter_(counter) {
    counter_.fetch_add(1, std::memory_order_acq_rel);
  }

  ~CurrentRenderScope() {
    counter_.fetch_sub(1, std::memory_order_release);
  }

  DELETE_COPY_AND_MOVE(CurrentRenderScope);

 private:
  std::atomic<uint32_t> &counter_;
};

} // namespace
