#include "GainNode.h"

namespace audiocontext {

using namespace facebook::jni;

AudioParam *GainNode::getGainParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getGain");
  auto gain = method(javaPart_.get());

  return gain->cthis();
}
} // namespace audiocontext
