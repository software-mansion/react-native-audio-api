#include <audioapi/android/core/AudioInputSelection.h>

#include <mutex>

namespace audioapi::AudioInputSelection {

namespace {
/// Guards both values together, so setPreferredDeviceId decides and writes
/// without a window in which a recorder could claim the selection in between.
std::mutex selectionMutex;
int32_t preferredDeviceId = kSystemDefaultDeviceId;
int32_t runningCaptureCount = 0;
} // namespace

bool setPreferredDeviceId(int32_t deviceId) {
  std::scoped_lock selectionLock(selectionMutex);

  if (deviceId == preferredDeviceId) {
    return true;
  }

  if (runningCaptureCount > 0) {
    return false;
  }

  preferredDeviceId = deviceId;
  return true;
}

int32_t getPreferredDeviceId() {
  std::scoped_lock selectionLock(selectionMutex);
  return preferredDeviceId;
}

void captureStarted() {
  std::scoped_lock selectionLock(selectionMutex);
  ++runningCaptureCount;
}

void captureStopped() {
  std::scoped_lock selectionLock(selectionMutex);
  --runningCaptureCount;
}

} // namespace audioapi::AudioInputSelection
