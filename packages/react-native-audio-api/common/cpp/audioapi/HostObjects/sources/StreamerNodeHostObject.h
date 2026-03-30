#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/StreamerNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct StreamerOptions;
class BaseAudioContext;

class StreamerNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit StreamerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const StreamerOptions &options)
      : AudioScheduledSourceNodeHostObject(
            context->getGraph(),
            std::make_unique<StreamerNode>(context, options),
            options) {}

  [[nodiscard]] static size_t getSizeInBytes() {
    return SIZE;
  }

 private:
  static constexpr size_t SIZE = 4'000'000; // 4MB
};
} // namespace audioapi
