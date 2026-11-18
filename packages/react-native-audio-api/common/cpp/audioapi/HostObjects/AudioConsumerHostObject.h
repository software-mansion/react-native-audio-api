#pragma once

#include <audioapi/core/AudioConsumer.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/utils/Macros.h>

#include <memory>

namespace audioapi {

class AudioConsumerHostObject : public HostObject {
 public:
  explicit AudioConsumerHostObject(std::shared_ptr<AudioConsumer> consumer);
  ~AudioConsumerHostObject() override;
  DELETE_COPY_AND_MOVE(AudioConsumerHostObject);

  [[nodiscard]] const std::shared_ptr<AudioConsumer> &consumer() const {
    return consumer_;
  }

 private:
  std::shared_ptr<AudioConsumer> consumer_;
};

} // namespace audioapi
