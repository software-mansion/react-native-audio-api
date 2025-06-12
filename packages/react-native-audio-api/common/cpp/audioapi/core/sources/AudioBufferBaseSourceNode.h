#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>

namespace audioapi {

class AudioBufferBaseSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferBaseSourceNode(BaseAudioContext *context);

    void setOnPositionChangedCallbackId(uint64_t callbackId);
    void setOnPositionChangedInterval(int interval);

 protected:
    uint64_t onPositionChangedCallbackId_ = 0;
    int onPositionChangedInterval_;
    int onPositionChangedTime_ = 0;

    void sendOnPositionChangedEvent();
    virtual double getCurrentPosition() const = 0;
};

} // namespace audioapi
