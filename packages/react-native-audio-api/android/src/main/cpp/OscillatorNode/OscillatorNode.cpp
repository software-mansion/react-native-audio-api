#include "OscillatorNode.h"

namespace audioapi {

using namespace facebook::jni;

AudioParam *OscillatorNode::getFrequencyParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getFrequency");
  auto frequency = method(javaPart_.get());

  return frequency->cthis();
}

AudioParam *OscillatorNode::getDetuneParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getDetune");
  auto detune = method(javaPart_.get());

  return detune->cthis();
}

std::string OscillatorNode::getWaveType() {
  static const auto method =
      javaClassLocal()->getMethod<JString()>("getWaveType");
  return method(javaPart_.get())->toStdString();
}

void OscillatorNode::setWaveType(const std::string &waveType) {
  static const auto method =
      javaClassLocal()->getMethod<void(JString)>("setWaveType");
  method(javaPart_.get(), *make_jstring(waveType));
}

} // namespace audioapi
