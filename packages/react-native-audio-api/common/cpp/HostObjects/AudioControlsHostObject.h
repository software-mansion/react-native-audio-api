#pragma once

#include <jsi/jsi.h>
#include <JsiHostObject.h>
#include <JsiPromise.h>
#include <cstdio>
#include <memory>

#include "AudioControls.h"
#include "AudioEventName.h"
#include "NowPlayingInfo.h"
#include "AudioSessionOptions.h"

namespace audioapi {
using namespace facebook;

class AudioControlsHostObject: public JsiHostObject {
 public:
  explicit AudioControlsHostObject(
      const std::shared_ptr<AudioControls> &audioControls,
      const std::shared_ptr<PromiseVendor> &promiseVendor)
      : audioControls_(audioControls), promiseVendor_(promiseVendor) {
    addFunctions(
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, init),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, updateOptions),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, disable),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, showNowPlayingInfo),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, updateNowPlayingInfo),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, hideNowPlayingInfo),
      JSI_EXPORT_FUNCTION(AudioControlsHostObject, addEventListener)
    );
  }

  JSI_HOST_FUNCTION(init) {
    auto options = AudioSessionOptions::fromJSIValue(args[0], runtime);
    audioControls_->init(options);

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(updateOptions) {
    auto options = AudioSessionOptions::fromJSIValue(args[0], runtime);
    audioControls_->updateOptions(options);

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(disable) {
    audioControls_->disable();
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(showNowPlayingInfo) {
    auto options = NowPlayingInfo::fromJSIValue(args[0], runtime);
    audioControls_->showNowPlayingInfo(options);

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(updateNowPlayingInfo) {
    auto options = NowPlayingInfo::fromJSIValue(args[0], runtime);
    audioControls_->updateNowPlayingInfo(options);

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(hideNowPlayingInfo) {
    audioControls_->hideNowPlayingInfo();
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(addEventListener) {
    AudioEventName eventName = AudioEventName::fromJSIValue(args[0], runtime);

    audioControls_->addEventListener();
    return jsi::Value::undefined();
  }

 protected:
  std::shared_ptr<AudioControls> audioControls_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};

} // namespace audioapi
