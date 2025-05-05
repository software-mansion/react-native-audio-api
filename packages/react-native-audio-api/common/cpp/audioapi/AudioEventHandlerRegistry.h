#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistryHostObject;

class AudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  template<typename... Args>
  void invokeHandler(const std::string &eventName, Args&&... args);

// private:
 public:
  std::shared_ptr<AudioEventHandlerRegistryHostObject> eventHandlerRegistry_;
};

} // namespace audioapi
