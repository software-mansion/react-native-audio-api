#include "BiquadFilterNode.h"

namespace audiocontext {

using namespace facebook::jni;

AudioParam *BiquadFilterNode::getFrequencyParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getFrequency");
  auto frequency = method(javaPart_.get());

  return frequency->cthis();
}

AudioParam *BiquadFilterNode::getDetuneParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getDetune");
  auto detune = method(javaPart_.get());

  return detune->cthis();
}

AudioParam *BiquadFilterNode::getQParam() {
  static const auto method = javaClassLocal()->getMethod<AudioParam()>("getQ");
  auto Q = method(javaPart_.get());

  return Q->cthis();
}

AudioParam *BiquadFilterNode::getGainParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getGain");
  auto gain = method(javaPart_.get());

  return gain->cthis();
}

std::string BiquadFilterNode::getFilterType() {
  static const auto method =
      javaClassLocal()->getMethod<JString()>("getFilterType");
  return method(javaPart_.get())->toStdString();
}

void BiquadFilterNode::setFilterType(const std::string &filterType) {
  static const auto method =
      javaClassLocal()->getMethod<void(JString)>("setFilterType");
  method(javaPart_.get(), *make_jstring(filterType));
}
} // namespace audiocontext
