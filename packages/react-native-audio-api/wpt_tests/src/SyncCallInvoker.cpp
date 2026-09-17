#include "SyncCallInvoker.h"

#include <utility>

namespace rnaudioapi::node {

namespace {

napi_value noopDispatch(napi_env env, napi_callback_info /* info */) {
  napi_value undefined;
  napi_get_undefined(env, &undefined);
  return undefined;
}

} // namespace

SyncCallInvoker::SyncCallInvoker() : mainThreadId_(std::this_thread::get_id()) {}

SyncCallInvoker::~SyncCallInvoker() {
  if (tsfn_ != nullptr) {
    napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
    tsfn_ = nullptr;
  }
}

void SyncCallInvoker::initialize(napi_env env) {
  env_ = env;
  mainThreadId_ = std::this_thread::get_id();

  napi_value resourceName;
  napi_create_string_utf8(env, "SyncCallInvoker", NAPI_AUTO_LENGTH, &resourceName);

  napi_value noopCallback;
  napi_create_function(
      env,
      "syncCallInvokerDispatch",
      NAPI_AUTO_LENGTH,
      noopDispatch,
      nullptr,
      &noopCallback);

  napi_create_threadsafe_function(
      env,
      noopCallback,
      nullptr,
      resourceName,
      0,
      1,
      this,
      nullptr,
      this,
      SyncCallInvoker::onMainThread,
      &tsfn_);
}

void SyncCallInvoker::setRuntime(facebook::jsi::Runtime *runtime) {
  runtime_ = runtime;
}

void SyncCallInvoker::onMainThread(
    napi_env /* env */,
    napi_value /* jsCallback */,
    void *context,
    void *data) {
  auto *invoker = static_cast<SyncCallInvoker *>(context);
  auto *callFunc = static_cast<facebook::react::CallFunc *>(data);
  if (invoker == nullptr || invoker->runtime_ == nullptr || callFunc == nullptr) {
    delete callFunc;
    return;
  }

  (*callFunc)(*invoker->runtime_);
  delete callFunc;
}

void SyncCallInvoker::invokeAsync(facebook::react::CallFunc &&func) noexcept {
  if (runtime_ == nullptr) {
    return;
  }

  // Before initialize() only the main thread exists; run inline.
  if (tsfn_ == nullptr) {
    func(*runtime_);
    return;
  }

  // Always queue — even from the JS thread. Running main-thread posts inline
  // let them jump ahead of audio-thread posts already sitting in the queue, so
  // promise-resolution order depended on which thread happened to enqueue
  // first (the suspend-after-construct WPT flake). One queue gives one FIFO
  // order, matching React Native's CallInvoker contract that invokeAsync never
  // executes synchronously.
  auto *callFunc = new facebook::react::CallFunc(std::move(func));
  napi_call_threadsafe_function(tsfn_, callFunc, napi_tsfn_blocking);
}

void SyncCallInvoker::invokeSync(facebook::react::CallFunc &&func) {
  // Genuinely synchronous only on the JS thread. From any other thread a
  // blocking wait would deadlock against the event loop this queue drains on,
  // so those posts degrade to async FIFO — a documented harness limitation.
  if (std::this_thread::get_id() == mainThreadId_ && runtime_ != nullptr) {
    func(*runtime_);
    return;
  }

  invokeAsync(std::move(func));
}

} // namespace rnaudioapi::node
