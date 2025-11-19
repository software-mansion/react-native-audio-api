#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioRecorderHostObject;

class RecorderAdapterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit RecorderAdapterNodeHostObject(const std::shared_ptr<RecorderAdapterNode> &node)
      : AudioNodeHostObject(node) {}

 private:
  friend class AudioRecorderHostObject;
};

} // namespace audioapi
