#pragma once

#include <jsi/jsi.h>
#include <fbjni/fbjni.h>
#include <fbjni/detail/Hybrid.h>
#include "AudioContext.h"

namespace audiocontext
{
  using namespace facebook;

  class AudioContext;

  class AudioContextHostObject : public jsi::HostObject
  {
  private:
    AudioContext *audiocontext_;

  public:
    explicit AudioContextHostObject(AudioContext *audiocontext) : audiocontext_(audiocontext) {}

    jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
    void set(jsi::Runtime &runtime, const jsi::PropNameID &name, const jsi::Value &value) override;
    std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

    jsi::Value createOscillator(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
    jsi::Value getDestination(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
    jsi::Value toString(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
  };
} // namespace audiocontext
