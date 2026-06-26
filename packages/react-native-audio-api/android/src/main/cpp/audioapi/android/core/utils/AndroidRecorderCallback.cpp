#include <android/log.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/android/core/utils/AndroidRecorderCallback.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/CircularArray.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>

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
  cleanup();
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
  result = ma_data_converter_init(&converterConfig, nullptr, converter_.get());

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

  deinterleavingBuffer_ =
      std::make_shared<AudioBuffer>(processingBufferLength_, channelCount_, sampleRate_);
  processingBuffer_ = ma_malloc(
      processingBufferLength_ * channelCount_ * ma_get_bytes_per_sample(ma_format_f32), nullptr);

  inputBufferBytesPerSlot_ = maxInputBufferLength_ * static_cast<size_t>(streamChannelCount_) *
      ma_get_bytes_per_sample(ma_format_f32);
  size_t inputPoolBytes = inputBufferBytesPerSlot_ * RECORDER_CALLBACK_POOL_SIZE;
  // nothrow new keeps the graceful failure path (return Err) instead of throwing.
  inputBufferPool_.reset(new (std::nothrow) std::byte[inputPoolBytes]);
  if (inputBufferPool_ == nullptr) {
    if (processingBuffer_ != nullptr) {
      ma_free(processingBuffer_, nullptr);
      processingBuffer_ = nullptr;
      processingBufferLength_ = 0;
    }
    return Result<NoneType, std::string>::Err(
        "Failed to preallocate Android recorder callback buffers");
  }

  inputBuffers_.clear();
  inputBuffers_.reserve(RECORDER_CALLBACK_POOL_SIZE);
  std::byte *poolHead = inputBufferPool_.get();
  for (size_t i = 0; i < RECORDER_CALLBACK_POOL_SIZE; ++i) {
    inputBuffers_.push_back(poolHead + i * inputBufferBytesPerSlot_);
  }

  freeSlots_ = std::make_unique<FreeList>();
  freeSlots_->seed();

  auto offloaderLambda = [this](CallbackData data) {
    taskOffloaderFunction(data);
  };
  offloader_ = std::make_unique<task_offloader::TaskOffloader<
      CallbackData,
      RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY,
      RECORDER_CALLBACK_SPSC_WAIT_STRATEGY>>(RECORDER_CALLBACK_CHANNEL_CAPACITY, offloaderLambda);
  return Result<NoneType, std::string>::Ok(None);
}

void AndroidRecorderCallback::cleanup() {
  std::scoped_lock audioLock(destructionAudioGuard_);
  // join the worker
  offloader_.reset();

  if (circularBuffer_[0]->getNumberOfAvailableFrames() > 0) {
    emitAudioData(true);
  }

  if (converter_ != nullptr) {
    ma_data_converter_uninit(converter_.get(), nullptr);
    converter_.reset();
  }

  if (processingBuffer_ != nullptr) {
    ma_free(processingBuffer_, nullptr);
    processingBuffer_ = nullptr;
    processingBufferLength_ = 0;
  }
  inputBufferPool_.reset();
  inputBufferBytesPerSlot_ = 0;
  inputBuffers_.clear();
  freeSlots_.reset();

  for (const auto &arr : circularBuffer_) {
    arr->zero();
  }
}

/// @brief Receives audio data from the recorder, processes it (resampling and deinterleaving if necessary),
/// and pushes it into the circular buffer.
/// @param data Pointer to the incoming audio data.
/// @param numFrames Number of frames in the incoming audio data.
void AndroidRecorderCallback::receiveAudioData(void *data, int numFrames) {
  // Don't block the audio thread: if cleanup() holds the guard we're being destroyed, drop the buffer.
  std::unique_lock<std::mutex> lock(destructionAudioGuard_, std::try_to_lock);
  if (!lock.owns_lock()) {
    return;
  }
  if (offloader_ == nullptr) {
    return;
  }
  if (freeSlots_ == nullptr || inputBuffers_.empty() || inputBufferBytesPerSlot_ == 0) {
    return;
  }
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  auto slot = freeSlots_->tryAcquire();
  if (!slot.has_value()) {
    return;
  }

  size_t bytes =
      static_cast<size_t>(numFrames) * streamChannelCount_ * ma_get_bytes_per_sample(ma_format_f32);
  if (bytes > inputBufferBytesPerSlot_) {
    freeSlots_->release(slot.value());
    return;
  }

  // Oboe owns `data` only for the duration of this synchronous
  // callback. Copy into an owned buffer before handing off to the
  // worker thread; the consumer in taskOffloaderFunction releases the slot.
  std::memcpy(inputBuffers_[slot.value()], data, bytes);
  CallbackData callbackData{.slot = slot.value(), .numFrames = numFrames};
  // send() cannot block here: we hold a slot from a pool of RECORDER_CALLBACK_POOL_SIZE,
  // and the channel is sized one larger, so the ring always has room while
  // any slot is in flight.
  offloader_->getSender()->send(callbackData);
}

/// @brief Deinterleaves the audio data and pushes it into the circular buffer.
/// @param data Pointer to the interleaved audio data.
/// @param numFrames Number of frames in the audio data.
void AndroidRecorderCallback::deinterleaveAndPushAudioData(void *data, int numFrames) {
  auto *inputData = static_cast<float *>(data);
  deinterleavingBuffer_->deinterleaveFrom(inputData, numFrames);

  for (int ch = 0; ch < channelCount_; ++ch) {
    circularBuffer_[ch]->push_back(*deinterleavingBuffer_->getChannel(ch), numFrames);
  }
}

/// @brief The handler function for the callback thread. It continuously receives audio data,
/// processes it (resampling and deinterleaving if necessary), and pushes it into the circular buffer.
void AndroidRecorderCallback::taskOffloaderFunction(CallbackData callbackData) {
  auto [slot, numFrames] = callbackData;

  // The TaskOffloader destructor sends a default-constructed CallbackData
  // with a sentinel slot to unblock the receiver; ignore it here.
  if (slot == FreeList::kSentinel) {
    return;
  }
  if (slot >= inputBuffers_.size() || freeSlots_ == nullptr) {
    return;
  }
  void *data = inputBuffers_[slot];

  ma_uint64 inputFrameCount = numFrames;
  ma_uint64 outputFrameCount = 0;

  if (streamSampleRate_ == sampleRate_ && streamChannelCount_ == channelCount_) {
    deinterleaveAndPushAudioData(data, numFrames);

    if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
      emitAudioData();
    }
    freeSlots_->release(slot);
    return;
  }

  ma_data_converter_get_expected_output_frame_count(
      converter_.get(), inputFrameCount, &outputFrameCount);

  ma_data_converter_process_pcm_frames(
      converter_.get(), data, &inputFrameCount, processingBuffer_, &outputFrameCount);

  deinterleaveAndPushAudioData(processingBuffer_, static_cast<int>(outputFrameCount));

  if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
    emitAudioData();
  }

  freeSlots_->release(slot);
}

} // namespace audioapi
