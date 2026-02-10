#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <memory>
#include <utility>

namespace audioapi {

WorkletsRunner::WorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> weakRuntime,
    const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    bool shouldLockRuntime)
    : weakRuntime_(std::move(weakRuntime)),
      shareableWorklet_(shareableWorklet),
      shouldLockRuntime(shouldLockRuntime) {
}

WorkletsRunner::WorkletsRunner(WorkletsRunner &&other)
    : weakRuntime_(std::move(other.weakRuntime_)),
      shareableWorklet_(std::move(other.shareableWorklet_)),
      unsafeRuntimePtr(other.unsafeRuntimePtr),
      workletInitialized(other.workletInitialized),
      shouldLockRuntime(other.shouldLockRuntime) {
  if (workletInitialized) {
    std::memcpy(&unsafeWorklet, &other.unsafeWorklet, sizeof(unsafeWorklet));
    other.workletInitialized = false;
    other.unsafeRuntimePtr = nullptr;
  }
}

WorkletsRunner::~WorkletsRunner() {
  if (!workletInitialized) {
    return;
  }
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    // We cannot safely destroy the worklet without a valid runtime
    return;
  }
  reinterpret_cast<jsi::Function *>(&unsafeWorklet)->~Function();
  workletInitialized = false;
}

bool WorkletsRunner::ensureWorkletInitialized(jsi::Runtime &rt) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  if (workletInitialized) {
    return true;
  }
  if (!shareableWorklet_) {
    return false;
  }
  auto valueUnpacker = rt.global().getProperty(rt, "__valueUnpacker");
  if (!valueUnpacker.isObject()) {
    return false;
  }
  unsafeRuntimePtr = &rt;
  /// Placement new to avoid dynamic memory allocation
  new (reinterpret_cast<jsi::Function *>(&unsafeWorklet))
      jsi::Function(shareableWorklet_->toJSValue(rt).asObject(rt).asFunction(rt));
  workletInitialized = true;
  return true;
#else
  (void)rt;
  return false;
#endif
}

std::optional<jsi::Value> WorkletsRunner::executeOnRuntimeGuarded(
    const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job)) {
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    return std::nullopt;
  }
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto &rt = strongRuntime->getJSIRuntime();
  if (!const_cast<WorkletsRunner *>(this)->ensureWorkletInitialized(rt)) {
    return std::nullopt;
  }
  return job(rt);
#else
  return std::nullopt;
#endif
}

std::optional<jsi::Value> WorkletsRunner::executeOnRuntimeUnsafe(
    const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job)) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  jsi::Runtime *rt = unsafeRuntimePtr;
  if (rt == nullptr) {
    auto strongRuntime = weakRuntime_.lock();
    if (strongRuntime == nullptr) {
      return std::nullopt;
    }
    rt = &strongRuntime->getJSIRuntime();
  }
  if (!const_cast<WorkletsRunner *>(this)->ensureWorkletInitialized(*rt)) {
    return std::nullopt;
  }
  return job(*rt);
#else
  return std::nullopt;
#endif
}

}; // namespace audioapi
