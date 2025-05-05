#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry();

  void registerHandler(const std::string &eventName, uint64_t listenerId, const std::shared_ptr<jsi::Function> &handler);
  void unregisterHandler(const std::string &eventName, uint64_t listenerId);

  void invokeHandlerWithJsonString(const std::string &eventName, const std::string &jsonString);

//  template<typename... Args>
//  void invokeHandler(const std::string &eventName, Args&&... args);

  template<typename... Args>
  void invokeHandler(const std::string &eventName, uint64_t listenerId, Args&&... args);

 private:
    std::shared_ptr<react::CallInvoker> callInvoker_;
    jsi::Runtime *runtime_;
    std::vector<std::string> eventNames_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>> eventHandlers_;
};

} // namespace audioapi
