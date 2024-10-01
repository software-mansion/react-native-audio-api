#include "AudioBufferSourceNode.h"

namespace audioapi {

// using namespace facebook::jni;
//
// bool AudioBufferSourceNode::getLoop() {
//   static const auto method =
//   javaClassLocal()->getMethod<jboolean()>("getLoop"); return
//   method(javaPart_.get());
// }
//
// void AudioBufferSourceNode::setLoop(bool loop) {
//   static const auto method =
//       javaClassLocal()->getMethod<void(jboolean)>("setLoop");
//   method(javaPart_.get(), loop);
// }
//
// AudioBuffer *AudioBufferSourceNode::getBuffer() {
//   static const auto method =
//       javaClassLocal()->getMethod<AudioBuffer()>("getBuffer");
//   auto buffer = method(javaPart_.get());
//
//   return buffer->cthis();
// }
//
// void AudioBufferSourceNode::setBuffer(const AudioBuffer *buffer) {
//   static const auto method =
//       javaClassLocal()->getMethod<void(AudioBuffer::javaobject)>("setBuffer");
//   method(javaPart_.get(), buffer->javaPart_.get());
// }
} // namespace audioapi
