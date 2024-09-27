#include "AudioScheduledSourceNode.h"

namespace audioapi {

using namespace facebook::jni;

void AudioScheduledSourceNode::start(double time) {
  static const auto method =
      javaClassLocal()->getMethod<void(jdouble)>("start");
  method(javaPart_.get(), time);
}

void AudioScheduledSourceNode::stop(double time) {
  static const auto method = javaClassLocal()->getMethod<void(jdouble)>("stop");
  method(javaPart_.get(), time);
}

} // namespace audioapi
