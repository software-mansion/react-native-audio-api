#pragma once

#include <fbjni/fbjni.h>
#include <oboe/Oboe.h>

#include <cstdint>
#include <optional>

namespace audioapi {

using namespace facebook;

/// @brief JNI bridge to the Kotlin MediaSessionManager, which owns the input device
/// selected through AudioManager.setInputDevice and the routing that device needs.
class NativeInputRouting : public jni::JavaClass<NativeInputRouting> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/system/NativeInputRouting;";

  /// Leaves the capture device to the platform, which is Oboe's default.
  static constexpr int32_t kSystemDefaultDeviceId = oboe::kUnspecified;

  static void registerNatives();

  /// @brief Announces a capture that is about to open its stream and asks which device
  /// to open it on.
  ///
  /// Every call must be balanced by releaseInputRoute(), including when it returns nullopt.
  /// @returns The device id to open the stream on (kSystemDefaultDeviceId to let the
  /// platform pick), or nullopt when the selected input could not be made to deliver audio.
  static std::optional<int32_t> acquireInputRoute();

  /// @brief Gives back a route from acquireInputRoute(); the last one tears the routing down.
  static void releaseInputRoute();

  /// @brief Called by Kotlin when the input to capture from changed while a recording
  /// may be running; moves the active recorder onto it.
  /// @returns false when the recorder could not continue on the new input.
  static jboolean rerouteActiveCapture(jni::alias_ref<jni::JClass>);
};

} // namespace audioapi
