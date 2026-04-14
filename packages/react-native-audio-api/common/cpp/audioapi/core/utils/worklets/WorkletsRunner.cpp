#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <memory>
#include <utility>

namespace audioapi {

WorkletsRunner::WorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> weakRuntime,
    const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    bool shouldLockRuntime)
    : weakRuntime_(std::move(weakRuntime)), shouldLockRuntime(shouldLockRuntime) {
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    return;
  }
#if RN_AUDIO_API_ENABLE_WORKLETS
  unsafeRuntimePtr = &strongRuntime->getJSIRuntime();
  strongRuntime->executeSync([this, shareableWorklet](jsi::Runtime &rt) -> jsi::Value {
    /// Placement new to avoid dynamic memory allocation
    new (reinterpret_cast<jsi::Function *>(&unsafeWorklet))
        jsi::Function(shareableWorklet->toJSValue(*unsafeRuntimePtr)
                          .asObject(*unsafeRuntimePtr)
                          .asFunction(*unsafeRuntimePtr));
    return jsi::Value::undefined();
  });
  workletInitialized = true;
#else
  unsafeRuntimePtr = nullptr;
  workletInitialized = false;
#endif
}

WorkletsRunner::WorkletsRunner(WorkletsRunner &&other)
    : weakRuntime_(std::move(other.weakRuntime_)),
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

std::optional<jsi::Value> WorkletsRunner::executeOnRuntimeGuarded(
    const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job)) {
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    return std::nullopt;
  }
#if RN_AUDIO_API_ENABLE_WORKLETS
  return strongRuntime->executeSync(std::move(job));
#else
  return std::nullopt;
#endif
}

std::optional<jsi::Value> WorkletsRunner::executeOnRuntimeUnsafe(
    const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job)) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  return job(*unsafeRuntimePtr);
#else
  return std::nullopt;
#endif
}

}; // namespace audioapi
