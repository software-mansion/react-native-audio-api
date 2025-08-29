#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<UiWorkletsRunner> &workletRunner)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      audioEventHandlerRegistry_(audioEventHandlerRegistry),
      workletRunner_(workletRunner) {
  constexpr int minRingBufferSize = 8192;
  ringBufferSize_ = std::max(2 * bufferLength, minRingBufferSize);
  circularBuffer_ = std::make_shared<CircularAudioArray>(ringBufferSize_);
  isRunning_.store(false);
}

#if RN_AUDIO_API_ENABLE_WORKLETS
void AudioRecorder::setWorkletCallback(
    std::shared_ptr<worklets::ShareableWorklet> &callback) {
  std::scoped_lock<std::mutex> lock(workletCallbackMutex_);
  shareableWorklet_ = callback;
}
void AudioRecorder::invokeWorkletOnAudioReadyCallback(
    const std::shared_ptr<AudioBus> &bus,
    int numFrames,
    double when) {
  std::scoped_lock<std::mutex> lock(workletCallbackMutex_);

  if (!shareableWorklet_) {
    return;
  }

  auto channelData = bus->getChannel(0);

  auto uiRuntimeRaw = workletRunner_->getJSIRuntime();
  if (uiRuntimeRaw == nullptr) {
    return;
  }
  /// this part might throw
  auto jsArray = jsi::Array(*uiRuntimeRaw, numFrames);
  for (size_t i = 0; i < numFrames; i++) {
    jsArray.setValueAtIndex(*uiRuntimeRaw, i, jsi::Value((*channelData)[i]));
  }

  workletRunner_->executeWorkletAsync(
      shareableWorklet_, jsArray, jsi::Value(when));
}
#endif

void AudioRecorder::setOnAudioReadyCallbackId(uint64_t callbackId) {
  onAudioReadyCallbackId_ = callbackId;
}

void AudioRecorder::invokeOnAudioReadyCallback(
    const std::shared_ptr<AudioBus> &bus,
    int numFrames,
    double when) {
  auto audioBuffer = std::make_shared<AudioBuffer>(bus);
  auto audioBufferHostObject =
      std::make_shared<AudioBufferHostObject>(audioBuffer);

  std::unordered_map<std::string, EventValue> body = {};
  body.insert({"buffer", audioBufferHostObject});
  body.insert({"numFrames", numFrames});
  body.insert({"when", when});

#if RN_AUDIO_API_ENABLE_WORKLETS
  invokeWorkletOnAudioReadyCallback(bus, numFrames, when);
#endif
  if (audioEventHandlerRegistry_ != nullptr) {
    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        "audioReady", onAudioReadyCallbackId_, body);
  }
}

void AudioRecorder::sendRemainingData() {
  auto bus = std::make_shared<AudioBus>(
      circularBuffer_->getNumberOfAvailableFrames(), 1, sampleRate_);
  auto *outputChannel = bus->getChannel(0)->getData();
  auto availableFrames =
      static_cast<int>(circularBuffer_->getNumberOfAvailableFrames());

  circularBuffer_->pop_front(
      outputChannel, circularBuffer_->getNumberOfAvailableFrames());

  invokeOnAudioReadyCallback(bus, availableFrames, 0);
}

void AudioRecorder::connect(const std::shared_ptr<RecorderAdapterNode> &node) {
  node->init(ringBufferSize_);
  adapterNodeLock_.lock();
  adapterNode_ = node;
  adapterNodeLock_.unlock();
}

void AudioRecorder::disconnect() {
  adapterNodeLock_.lock();
  adapterNode_ = nullptr;
  adapterNodeLock_.unlock();
}

void AudioRecorder::writeToBuffers(const float *data, int numFrames) {
  if (adapterNodeLock_.try_lock()) {
    if (adapterNode_ != nullptr) {
      adapterNode_->buff_->write(data, numFrames);
    }
    adapterNodeLock_.unlock();
  }
  circularBuffer_->push_back(data, numFrames);
}

} // namespace audioapi
