#pragma once

#include <ReactCommon/CallInvoker.h>
#include <node_api.h>

#include <thread>

namespace rnaudioapi::node {

class SyncCallInvoker final : public facebook::react::CallInvoker {
 public:
  SyncCallInvoker();
  ~SyncCallInvoker() override;

  void initialize(napi_env env);
  void setRuntime(facebook::jsi::Runtime *runtime);

  void invokeAsync(facebook::react::CallFunc &&func) noexcept override;
  void invokeSync(facebook::react::CallFunc &&func) override;

 private:
  static void onMainThread(napi_env env, napi_value jsCallback, void *context, void *data);

  napi_env env_{nullptr};
  napi_threadsafe_function tsfn_{nullptr};
  std::thread::id mainThreadId_;
  facebook::jsi::Runtime *runtime_{nullptr};
};

} // namespace rnaudioapi::node
