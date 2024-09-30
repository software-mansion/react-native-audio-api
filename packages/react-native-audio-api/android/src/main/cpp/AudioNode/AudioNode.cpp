#include "AudioNode.h"

namespace audioapi {

AudioNode::AudioNode(alias_ref<AudioNode::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

int AudioNode::getNumberOfInputs() {
  static const auto method =
      javaClassLocal()->getMethod<int()>("getNumberOfInputs");
  return method(javaPart_.get());
}

int AudioNode::getNumberOfOutputs() {
  static const auto method =
      javaClassLocal()->getMethod<int()>("getNumberOfOutputs");
  return method(javaPart_.get());
}

int AudioNode::getChannelCount() {
  static const auto method =
      javaClassLocal()->getMethod<int()>("getChannelCount");
  return method(javaPart_.get());
}

std::string AudioNode::getChannelCountMode() {
  static const auto method =
      javaClassLocal()->getMethod<JString()>("getChannelCountMode");
  return method(javaPart_.get())->toStdString();
}

std::string AudioNode::getChannelInterpretation() {
  static const auto method =
      javaClassLocal()->getMethod<JString()>("getChannelInterpretation");
  return method(javaPart_.get())->toStdString();
}

void AudioNode::connect(const AudioNode *node) {
  static const auto method =
      javaClassLocal()->getMethod<void(AudioNode::javaobject)>("connect");
  method(javaPart_.get(), node->javaPart_.get());
}

void AudioNode::disconnect(const AudioNode *node) {
  static const auto method =
      javaClassLocal()->getMethod<void(AudioNode::javaobject)>("disconnect");
  method(javaPart_.get(), node->javaPart_.get());
}

} // namespace audioapi
