#include "jsi_install.h"

#include "NodeJSRuntimeApi.h"
#include "SyncCallInvoker.h"

#include <NodeApiJsiRuntime.h>
#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>
#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <jsi/jsi.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {

using audioapi::AudioBuffer;
using audioapi::AudioBufferHostObject;
using audioapi::AudioContextHostObject;
using audioapi::AudioEventHandlerRegistry;
using audioapi::AudioEventHandlerRegistryHostObject;
using audioapi::IAudioEventHandlerRegistry;
using audioapi::OfflineAudioContextHostObject;
using facebook::jsi::Function;
using facebook::jsi::JSError;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;
using Microsoft::NodeApiJsi::makeNodeApiJsiRuntime;
using rnaudioapi::node::NodeJSRuntimeApi;
using rnaudioapi::node::SyncCallInvoker;

struct JsiInstallState {
  std::unique_ptr<NodeJSRuntimeApi> jsRuntimeApi;
  std::unique_ptr<Runtime> runtime;
  std::shared_ptr<SyncCallInvoker> callInvoker;
  std::shared_ptr<AudioEventHandlerRegistry> eventRegistry;
};

std::mutex gInstallMutex;
std::unordered_map<napi_env, std::unique_ptr<JsiInstallState>> gInstallStates;

void cleanupInstallState(void *data) {
  auto *env = reinterpret_cast<napi_env>(data);
  std::lock_guard<std::mutex> lock(gInstallMutex);
  gInstallStates.erase(env);
}

/// Advertised GC cost of one audio context HostObject. A context owns worker
/// threads (promise offloader, disposer, per-context pools) and buffers that
/// live outside the V8 heap; without this hint V8 sees a tiny object and lets
/// abandoned contexts linger for the rest of the process.
constexpr size_t kAudioContextExternalMemoryPressure = 8 * 1024 * 1024;

Object makeContextObject(
    Runtime &rt,
    const std::shared_ptr<facebook::jsi::HostObject> &hostObject) {
  auto object = Object::createFromHostObject(rt, hostObject);
  try {
    object.setExternalMemoryPressure(rt, kAudioContextExternalMemoryPressure);
  } catch (...) {
    // Runtimes without instrumentation support just skip the hint.
  }
  return object;
}

napi_value makeBoolean(napi_env env, bool value) {
  napi_value result;
  napi_get_boolean(env, value, &result);
  return result;
}

[[noreturn]] void throwTypeError(Runtime &runtime, const std::string &message) {
  auto typeError = runtime.global().getPropertyAsFunction(runtime, "TypeError");
  auto error = typeError.callAsConstructor(runtime, String::createFromUtf8(runtime, message));
  throw JSError(runtime, std::move(error));
}

bool isNullOrUndefined(const Value &value) {
  return value.isNull() || value.isUndefined();
}

void parseOfflineContextArgs(
    Runtime &runtime,
    const Value *args,
    size_t count,
    int &numberOfChannels,
    size_t &length,
    float &sampleRate) {
  numberOfChannels = 1;
  length = 0;
  sampleRate = 0.0f;

  if (count >= 3 && args[0].isNumber() && args[1].isNumber() && args[2].isNumber()) {
    numberOfChannels = static_cast<int>(args[0].getNumber());
    length = static_cast<size_t>(args[1].getNumber());
    sampleRate = static_cast<float>(args[2].getNumber());
    return;
  }

  if (count >= 2 && args[0].isNumber() && args[1].isNumber()) {
    numberOfChannels = 1;
    length = static_cast<size_t>(args[0].getNumber());
    sampleRate = static_cast<float>(args[1].getNumber());
    return;
  }

  if (count >= 3 && isNullOrUndefined(args[0]) && args[1].isNumber() && args[2].isNumber()) {
    numberOfChannels = 1;
    length = static_cast<size_t>(args[1].getNumber());
    sampleRate = static_cast<float>(args[2].getNumber());
    return;
  }

  throwTypeError(
      runtime,
      "createOfflineAudioContext expects at least (length, sampleRate) or "
      "(numberOfChannels, length, sampleRate).");
}

void installOfflineBindings(
    Runtime &runtime,
    const std::shared_ptr<IAudioEventHandlerRegistry> &eventRegistry,
    const std::shared_ptr<SyncCallInvoker> &callInvoker) {
  auto createOfflineAudioContext = Function::createFromHostFunction(
      runtime,
      PropNameID::forAscii(runtime, "createOfflineAudioContext"),
      3,
      [eventRegistry, callInvoker](
          Runtime &rt,
          const Value & /* thisValue */,
          const Value *args,
          size_t count) -> Value {
        int numberOfChannels = 1;
        size_t length = 0;
        float sampleRate = 0.0f;
        parseOfflineContextArgs(
            rt, args, count, numberOfChannels, length, sampleRate);

        auto hostObject = std::make_shared<OfflineAudioContextHostObject>(
            numberOfChannels,
            length,
            sampleRate,
            eventRegistry,
            &rt,
            callInvoker);

        return makeContextObject(rt, hostObject);
      });
  runtime.global().setProperty(runtime, "createOfflineAudioContext", createOfflineAudioContext);

  auto createAudioBuffer = Function::createFromHostFunction(
      runtime,
      PropNameID::forAscii(runtime, "createAudioBuffer"),
      3,
      [](
          Runtime &rt,
          const Value & /* thisValue */,
          const Value *args,
          size_t count) -> Value {
        if (count < 3 || !args[0].isNumber() || !args[1].isNumber() || !args[2].isNumber()) {
          throwTypeError(
              rt,
              "createAudioBuffer(numberOfChannels, length, sampleRate) requires 3 numeric arguments.");
        }

        const auto numberOfChannels = static_cast<int>(args[0].getNumber());
        const auto length = static_cast<size_t>(args[1].getNumber());
        const auto sampleRate = static_cast<float>(args[2].getNumber());

        auto audioBuffer = std::make_shared<AudioBuffer>(length, numberOfChannels, sampleRate);
        auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
        return Object::createFromHostObject(rt, audioBufferHostObject);
      });
  runtime.global().setProperty(runtime, "createAudioBuffer", createAudioBuffer);
}

void installAudioContextBinding(
    Runtime &runtime,
    const std::shared_ptr<IAudioEventHandlerRegistry> &eventRegistry,
    const std::shared_ptr<SyncCallInvoker> &callInvoker) {
  auto createAudioContext = Function::createFromHostFunction(
      runtime,
      PropNameID::forAscii(runtime, "createAudioContext"),
      2,
      [eventRegistry, callInvoker](
          Runtime &rt,
          const Value & /* thisValue */,
          const Value *args,
          size_t count) -> Value {
        if (count < 1 || !args[0].isNumber()) {
          throwTypeError(rt, "createAudioContext(sampleRate) requires a numeric sampleRate.");
        }

        const auto sampleRate = static_cast<float>(args[0].getNumber());
        auto hostObject = std::make_shared<AudioContextHostObject>(
            sampleRate,
            eventRegistry,
            &rt,
            callInvoker);

        return makeContextObject(rt, hostObject);
      });

  runtime.global().setProperty(runtime, "createAudioContext", createAudioContext);
}

void installUnsupportedGlobal(Runtime &runtime, const char *name) {
  auto fn = Function::createFromHostFunction(
      runtime,
      PropNameID::forAscii(runtime, name),
      0,
      [name](Runtime &rt, const Value &, const Value *, size_t) -> Value {
        throw JSError(rt, std::string(name) + "() is not available in Node phase-1 JSI bindings.");
      });
  runtime.global().setProperty(runtime, name, std::move(fn));
}

void installAudioEventEmitter(
    Runtime &runtime,
    const std::shared_ptr<IAudioEventHandlerRegistry> &eventRegistry) {
  auto eventEmitter = std::make_shared<AudioEventHandlerRegistryHostObject>(eventRegistry);
  runtime.global().setProperty(
      runtime,
      "AudioEventEmitter",
      Object::createFromHostObject(runtime, eventEmitter));
}

} // namespace

napi_value installUsingJsiBindings(napi_env env) {
  try {
    std::scoped_lock lock(gInstallMutex);
    auto existing = gInstallStates.find(env);
    if (existing != gInstallStates.end()) {
      return makeBoolean(env, true);
    }

    auto state = std::make_unique<JsiInstallState>();
    state->jsRuntimeApi = std::make_unique<NodeJSRuntimeApi>();
    Microsoft::NodeApiJsi::JSRuntimeApi::setCurrent(state->jsRuntimeApi->api());
    state->runtime = makeNodeApiJsiRuntime(env, state->jsRuntimeApi->api(), [] {});
    state->callInvoker = std::make_shared<SyncCallInvoker>();
    state->callInvoker->initialize(env);
    state->callInvoker->setRuntime(state->runtime.get());
    state->eventRegistry =
        std::make_shared<AudioEventHandlerRegistry>(state->runtime.get(), state->callInvoker);

    installOfflineBindings(*state->runtime, state->eventRegistry, state->callInvoker);
    installAudioContextBinding(*state->runtime, state->eventRegistry, state->callInvoker);
    installUnsupportedGlobal(*state->runtime, "createAudioRecorder");
    installUnsupportedGlobal(*state->runtime, "createAudioDecoder");
    installUnsupportedGlobal(*state->runtime, "createAudioFileUtils");
    installUnsupportedGlobal(*state->runtime, "createAudioStretcher");
    installAudioEventEmitter(*state->runtime, state->eventRegistry);

    napi_add_env_cleanup_hook(env, cleanupInstallState, env);
    gInstallStates.emplace(env, std::move(state));
    return makeBoolean(env, true);
  } catch (const std::exception &error) {
    napi_throw_error(env, nullptr, error.what());
    return nullptr;
  } catch (...) {
    napi_throw_error(env, nullptr, "Unexpected error while installing JSI bindings.");
    return nullptr;
  }
}
