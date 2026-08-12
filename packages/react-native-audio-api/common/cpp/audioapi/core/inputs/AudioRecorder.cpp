#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <algorithm>
#include <memory>

namespace audioapi {

/// @brief Hands one buffer of recorded audio to every configured consumer.
/// Called on the audio thread; must not block, allocate, or throw.
void AudioRecorder::onAudioFrames(const float *interleavedFrames, int numFrames) {
  if (interleavedFrames == nullptr || numFrames <= 0) {
    return;
  }

  lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

  if (usesFileOutput()) {
    if (auto fileWriterLock = Locker::tryLock(fileWriterMutex_)) {
      auto fileWriter = fileWriter_;
      if (usesFileOutput() && fileWriter != nullptr) {
        fileWriter->writeAudioData(interleavedFrames, numFrames);
      }
    }
  }

  if (usesCallback()) {
    if (auto callbackLock = Locker::tryLock(callbackMutex_)) {
      auto dataCallback = dataCallback_;
      if (usesCallback() && dataCallback != nullptr) {
        dataCallback->receiveAudioData(interleavedFrames, numFrames);
      }
    }
  }

  if (isConnected()) {
    if (auto adapterLock = Locker::tryLock(adapterNodeMutex_)) {
      auto adapterNodeHandle = adapterNodeHandle_;
      auto deinterleavingBuffer = deinterleavingBuffer_;
      if (!isConnected() || adapterNodeHandle == nullptr || deinterleavingBuffer == nullptr) {
        return;
      }
      // The buffer is sized for the stream's maximum burst; a larger callback would
      // overrun it, so drop rather than write past the end.
      if (static_cast<size_t>(numFrames) > deinterleavingBuffer->getSize()) {
        return;
      }

      auto *adapterNode = static_cast<RecorderAdapterNode *>(adapterNodeHandle->audioNode.get());
      deinterleavingBuffer->deinterleaveFrom(interleavedFrames, numFrames);

      const size_t channelCount =
          std::min(adapterNode->getChannelCount(), deinterleavingBuffer->getNumberOfChannels());
      for (size_t channel = 0; channel < channelCount; ++channel) {
        adapterNode->buff_[channel]->write(*deinterleavingBuffer->getChannel(channel), numFrames);
      }
    }
  }
}

/// @brief Sets the error callback to be invoked when an error occurs during recording.
/// This method should be called from the JS thread only.
/// @param callbackId Identifier for the JS callback to be invoked.
void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput() && fileWriter_ != nullptr) {
    fileWriter_->setOnErrorCallback(callbackId);
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    dataCallback_->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

/// @brief Clears the error callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput() && fileWriter_ != nullptr) {
    fileWriter_->clearOnErrorCallback();
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    dataCallback_->clearOnErrorCallback();
  }

  errorCallbackId_.store(0, std::memory_order_release);
}

/// @brief Gets the current duration of the recorded audio in seconds.
/// @returns Duration in seconds.
double AudioRecorder::getCurrentDuration() const {
  std::scoped_lock lock(fileWriterMutex_);
  double duration = 0.0;

  if (usesFileOutput() && fileWriter_ != nullptr) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

bool AudioRecorder::usesCallback() const {
  return wantsCallback() && callbackOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::usesFileOutput() const {
  return wantsFileOutput() && fileOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::isConnected() const {
  return wantsConnection() && connectedConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsCallback() const {
  return callbackOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsFileOutput() const {
  return fileOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsConnection() const {
  return isConnected_.load(std::memory_order_acquire);
}

} // namespace audioapi
