#include <android/log.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/android/core/utils/AndroidRecorderCallback.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {

/// @brief Constructor
/// Allocates circular buffer (as every property to do so is already known at this point).
/// @param audioEventHandlerRegistry The audio event handler registry
/// @param sampleRate The user desired sample rate
/// @param bufferLength The user desired buffer length
/// @param channelCount The user desired channel count
/// @param callbackId The callback identifier
AndroidRecorderCallback::AndroidRecorderCallback(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId)
    : AudioRecorderCallback(
          audioEventHandlerRegistry,
          sampleRate,
          bufferLength,
          channelCount,
          callbackId) {}

AndroidRecorderCallback::~AndroidRecorderCallback() {
  if (converter_ != nullptr) {
    ma_data_converter_uninit(converter_.get(), NULL);
    converter_.reset();
  }

  if (processingBuffer_ != nullptr) {
    ma_free(processingBuffer_, NULL);
    processingBuffer_ = nullptr;
    processingBufferLength_ = 0;
  }

  for (size_t i = 0; i < circularBus_.size(); ++i) {
    circularBus_[i]->zero();
  }
}

/// @brief Prepares the recorder callback by initializing the data converter and allocating necessary buffers.
/// @param streamSampleRate The sample rate of the incoming audio stream.
/// @param streamChannelCount The channel count of the incoming audio stream.
/// @param maxInputBufferLength The maximum buffer length of the incoming audio stream.
Result<NoneType, std::string> AndroidRecorderCallback::prepare(
    float streamSampleRate,
    int32_t streamChannelCount,
    size_t maxInputBufferLength) {
  ma_result result;

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxInputBufferLength_ = maxInputBufferLength;

  ma_data_converter_config converterConfig = ma_data_converter_config_init(
      ma_format_f32,
      ma_format_f32,
      streamChannelCount_,
      channelCount_,
      streamSampleRate_,
      static_cast<int32_t>(sampleRate_));

  converter_ = std::make_unique<ma_data_converter>();
  result = ma_data_converter_init(&converterConfig, NULL, converter_.get());

  if (result != MA_SUCCESS) {
    return Result<NoneType, std::string>::Err(
        "Failed to initialize miniaudio data converter" +
        std::string(ma_result_description(result)));
  }

  if (streamSampleRate_ <= 0 || streamChannelCount_ <= 0) {
    return Result<NoneType, std::string>::Err("Invalid stream sample rate or channel count");
  }

  if (sampleRate_ <= 0 || channelCount_ <= 0) {
    return Result<NoneType, std::string>::Err("Invalid callback sample rate or channel count");
  }

  ma_data_converter_get_expected_output_frame_count(
      converter_.get(), maxInputBufferLength_, &processingBufferLength_);

  processingBufferLength_ = std::max(processingBufferLength_, (ma_uint64)maxInputBufferLength_);

  deinterleavingArray_ = std::make_shared<AudioArray>(processingBufferLength_);
  processingBuffer_ = ma_malloc(
      processingBufferLength_ * channelCount_ * ma_get_bytes_per_sample(ma_format_f32), NULL);

  return Result<NoneType, std::string>::Ok(None);
}

void AndroidRecorderCallback::cleanup() {
  if (circularBus_[0]->getNumberOfAvailableFrames() > 0) {
    emitAudioData(true);
  }

  if (converter_ != nullptr) {
    ma_data_converter_uninit(converter_.get(), NULL);
    converter_.reset();
  }

  if (processingBuffer_ != nullptr) {
    ma_free(processingBuffer_, NULL);
    processingBuffer_ = nullptr;
    processingBufferLength_ = 0;
  }

  for (size_t i = 0; i < circularBus_.size(); ++i) {
    circularBus_[i]->zero();
  }
}

/// @brief Receives audio data from the recorder, processes it (resampling and deinterleaving if necessary),
/// and pushes it into the circular buffer.
/// @param data Pointer to the incoming audio data.
/// @param numFrames Number of frames in the incoming audio data.
void AndroidRecorderCallback::receiveAudioData(void *data, int numFrames) {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  ma_uint64 inputFrameCount = numFrames;
  ma_uint64 outputFrameCount = 0;

  if (static_cast<float>(streamSampleRate_) == sampleRate_ &&
      streamChannelCount_ == channelCount_) {
    deinterleaveAndPushAudioData(data, numFrames);

    if (circularBus_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
      emitAudioData();
    }
    return;
  }

  ma_data_converter_get_expected_output_frame_count(
      converter_.get(), inputFrameCount, &outputFrameCount);

  ma_data_converter_process_pcm_frames(
      converter_.get(), data, &inputFrameCount, processingBuffer_, &outputFrameCount);

  deinterleaveAndPushAudioData(processingBuffer_, static_cast<int>(outputFrameCount));

  if (circularBus_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
    emitAudioData();
  }
}

/// @brief Deinterleaves the audio data and pushes it into the circular buffer.
/// @param data Pointer to the interleaved audio data.
/// @param numFrames Number of frames in the audio data.
void AndroidRecorderCallback::deinterleaveAndPushAudioData(void *data, int numFrames) {
  auto *inputData = static_cast<float *>(data);

  for (int channel = 0; channel < channelCount_; ++channel) {
    float *channelData = deinterleavingArray_->getData();

    for (int frame = 0; frame < numFrames; ++frame) {
      channelData[frame] = inputData[frame * channelCount_ + channel];
    }

    circularBus_[channel]->push_back(channelData, numFrames);
  }
}

} // namespace audioapi
