#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioFileSourceOptions;
class BaseAudioContext;

class AudioFileSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioFileSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);

  ~AudioFileSourceNodeHostObject() override = default;
};

} // namespace audioapi
