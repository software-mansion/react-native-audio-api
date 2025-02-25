// #include "AudioAPIModuleInstaller.h"
//
// #include "AudioContext.h"
// #include "AudioContextHostObject.h"
//
// namespace audioapi {
//
// AudioAPIModuleInstaller::AudioAPIModuleInstaller(
//     jsi::Runtime *jsiRuntime,
//     const std::shared_ptr<react::CallInvoker> &jsCallInvoker):
//     jsiRuntime_(jsiRuntime), jsCallInvoker_(jsCallInvoker) {}
//
// static void AudioAPIModuleInstaller::injectJSIBindings() {
//   auto createAudioContext = getCreateAudioContextFunction();
//   jsiRuntime_->global().setProperty(
//       *jsiRuntime_, "createAudioContext", createAudioContext);
// }
//
// jsi::Function AudioAPIModuleInstaller::getCreateAudioContextFunction() {
//   return jsi::Function::createFromHostFunction(
//       *jsiRuntime_,
//       jsi::PropNameID::forAscii(*jsiRuntime_, "createAudioContext"),
//       0,
//       [this](
//           jsi::Runtime &runtime,
//           const jsi::Value &thisValue,
//           const jsi::Value *args,
//           size_t count) -> jsi::Value {
//         std::shared_ptr<AudioContext> audioContext;
//         if (args[0].isUndefined()) {
//           audioContext = std::make_shared<AudioContext>();
//         } else {
//           auto sampleRate = static_cast<float>(args[0].getNumber());
//           audioContext = std::make_shared<AudioContext>(sampleRate);
//         }
//
//             auto promiseVendor = std::make_shared<PromiseVendor>(jsiRuntime_,
//             jsCallInvoker_);
//
//         auto audioContextHostObject =
//         std::make_shared<AudioContextHostObject>(
//             audioContext, promiseVendor);
//
//         return jsi::Object::createFromHostObject(
//             runtime, audioContextHostObject);
//       });
// }
//
// } // namespace audioapi
