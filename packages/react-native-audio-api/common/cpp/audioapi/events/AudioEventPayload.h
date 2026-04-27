#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <variant>

namespace audioapi {

// broadcast sentinel — listenerId 0 means dispatch to all registered handlers.
static constexpr uint64_t kBroadcastListenerId = 0;

struct EmptyPayload {
  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    return facebook::jsi::Object(rt);
  }
};

struct DoubleValuePayload {
  double value;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(rt, "value", value);
    return obj;
  }
};

struct InterruptionPayload {
  std::string type;
  bool shouldResume;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(rt, "type", facebook::jsi::String::createFromUtf8(rt, type));
    obj.setProperty(rt, "shouldResume", shouldResume);
    return obj;
  }
};

struct RouteChangePayload {
  std::string reason;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(rt, "reason", facebook::jsi::String::createFromUtf8(rt, reason));
    return obj;
  }
};

struct BufferEndedPayload {
  size_t bufferId;
  bool isLastBufferInQueue;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(
        rt, "bufferId", facebook::jsi::String::createFromUtf8(rt, std::to_string(bufferId)));
    obj.setProperty(rt, "isLastBufferInQueue", isLastBufferInQueue);
    return obj;
  }
};

struct AudioReadyPayload {
  std::shared_ptr<AudioBufferHostObject> buffer;
  int numFrames;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(rt, "buffer", facebook::jsi::Object::createFromHostObject(rt, buffer));
    obj.setProperty(rt, "numFrames", numFrames);
    obj.setExternalMemoryPressure(rt, buffer->getSizeInBytes());
    return obj;
  }
};

struct RecorderErrorPayload {
  std::string message;

  facebook::jsi::Object toJsiObject(facebook::jsi::Runtime &rt) const {
    facebook::jsi::Object obj(rt);
    obj.setProperty(rt, "message", facebook::jsi::String::createFromUtf8(rt, message));
    return obj;
  }
};

using AudioEventPayload = std::variant<
    EmptyPayload,
    DoubleValuePayload,
    InterruptionPayload,
    RouteChangePayload,
    BufferEndedPayload,
    AudioReadyPayload,
    RecorderErrorPayload>;

} // namespace audioapi
