#include "StereoPannerNode.h"

namespace audioapi {

using namespace facebook::jni;

AudioParam *StereoPannerNode::getPanParam() {
  static const auto method =
      javaClassLocal()->getMethod<AudioParam()>("getPan");
  auto pan = method(javaPart_.get());

  return pan->cthis();
}
} // namespace audioapi
