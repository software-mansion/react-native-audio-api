#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class StreamerNode;

class StreamerNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit StreamerNodeHostObject(
          const std::shared_ptr<StreamerNode> &node);

    [[nodiscard]] static inline size_t getSizeInBytes() {
        return SIZE;
    }

  JSI_HOST_FUNCTION_DECL(initialize);

 private:
    static constexpr size_t SIZE = 4'000'000; // 4MB
};
} // namespace audioapi
