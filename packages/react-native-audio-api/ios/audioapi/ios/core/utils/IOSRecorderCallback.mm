#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/IOSRecorderCallback.h>
#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/Result.hpp>
#include <memory>
#include <mutex>
#include <utility>

namespace audioapi {

using ios::allocateOwnedAudioBufferList;
using ios::copyIntoOwnedAudioBufferList;
using ios::OwnedAudioBufferListPtr;

IOSRecorderCallback::IOSRecorderCallback(
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
          callbackId)
{
}

IOSRecorderCallback::~IOSRecorderCallback()
{
  cleanup();
}

/// @brief Prepares the IOSRecorderCallback for receiving audio data.
/// This involves setting up the audio converter and buffers based on the provided input format.
/// This method should be called from the JS thread only.
/// @param bufferFormat The format of the incoming audio data.
/// @param maxInputBufferLength The maximum length of the input buffer in frames.
/// @returns Result indicating success or error with message.
Result<NoneType, std::string> IOSRecorderCallback::prepare(
    AVAudioFormat *bufferFormat,
    size_t maxInputBufferLength)
{
  @autoreleasepool {
    bufferFormat_ = bufferFormat;
    converterInputBufferSize_ = maxInputBufferLength;

    if (bufferFormat.sampleRate <= 0 || bufferFormat.channelCount == 0) {
      return Result<NoneType, std::string>::Err(
          "Invalid input format: sampleRate and channelCount must be greater than 0");
    }

    if (sampleRate_ <= 0 || channelCount_ == 0) {
      return Result<NoneType, std::string>::Err(
          "Invalid callback format: sampleRate and channelCount must be greater than 0");
    }

    converterOutputBufferSize_ = std::max(
        (double)maxInputBufferLength, sampleRate_ / bufferFormat.sampleRate * maxInputBufferLength);

    callbackFormat_ = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                       sampleRate:sampleRate_
                                                         channels:channelCount_
                                                      interleaved:NO];

    converter_ = [[AVAudioConverter alloc] initFromFormat:bufferFormat toFormat:callbackFormat_];
    converter_.sampleRateConverterAlgorithm = AVSampleRateConverterAlgorithm_Normal;
    converter_.sampleRateConverterQuality = AVAudioQualityMax;
    converter_.primeMethod = AVAudioConverterPrimeMethod_None;

    converterInputBuffer_ =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:bufferFormat_
                                      frameCapacity:(AVAudioFrameCount)converterInputBufferSize_];
    converterOutputBuffer_ =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:callbackFormat_
                                      frameCapacity:(AVAudioFrameCount)converterOutputBufferSize_];

    inputBufferPool_.clear();
    inputBufferPool_.reserve(RECORDER_CALLBACK_POOL_SIZE);
    auto bufferCount =
        static_cast<UInt32>(bufferFormat_.isInterleaved ? 1 : bufferFormat_.channelCount);
    auto bytesPerBuffer = static_cast<UInt32>(
        converterInputBufferSize_ * bufferFormat_.streamDescription->mBytesPerFrame);
    inputBufferBytesPerBuffer_ = bytesPerBuffer;
    for (size_t i = 0; i < RECORDER_CALLBACK_POOL_SIZE; ++i) {
      OwnedAudioBufferListPtr buffer(allocateOwnedAudioBufferList(bufferCount, bytesPerBuffer));
      if (buffer == nullptr) {
        inputBufferPool_.clear();
        return Result<NoneType, std::string>::Err(
            "Failed to preallocate iOS recorder callback buffers");
      }
      inputBufferPool_.push_back(std::move(buffer));
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
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Cleans up resources used by the IOSRecorderCallback.
/// This method should be called from the JS GC thread only.
void IOSRecorderCallback::cleanup()
{
  std::scoped_lock audioLock(destructionAudioGuard_);
  @autoreleasepool {
    // join the worker
    offloader_.reset();

    freeSlots_.reset();
    inputBufferPool_.clear();
    inputBufferBytesPerBuffer_ = 0;

    if (circularBuffer_[0]->getNumberOfAvailableFrames() > 0) {
      emitAudioData(true);
    }

    converter_ = nil;
    bufferFormat_ = nil;
    callbackFormat_ = nil;
    converterInputBuffer_ = nil;
    converterOutputBuffer_ = nil;

    for (int i = 0; i < channelCount_; ++i) {
      circularBuffer_[i]->zero();
    }
  }
}

/// @brief Receives audio data from the recorder, processes it, and stores it in the circular buffer.
/// The data is converted using AVAudioConverter if the input format differs from the user desired callback format.
/// This method runs on the audio thread.
/// @param audioBufferList Pointer to the AudioBufferList containing the incoming audio data.
/// @param numFrames Number of frames in the input buffer.
void IOSRecorderCallback::receiveAudioData(const AudioBufferList *audioBufferList, int numFrames)
{
  // Don't block the audio thread: if cleanup() holds the guard we're being destroyed, drop the buffer.
  std::unique_lock<std::mutex> lock(destructionAudioGuard_, std::try_to_lock);
  if (!lock.owns_lock()) {
    return;
  }
  if (offloader_ == nullptr) {
    return;
  }
  if (freeSlots_ == nullptr || inputBufferPool_.empty() || inputBufferBytesPerBuffer_ == 0) {
    return;
  }
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  auto slot = freeSlots_->tryAcquire();
  if (!slot.has_value()) {
    return;
  }

  // CoreAudio owns `audioBufferList` only for the duration of this synchronous
  // callback. Copy into an owned AudioBufferList before handing off to the
  // worker thread; the consumer in taskOffloaderFunction frees it.
  auto *targetBuffer = inputBufferPool_[slot.value()].get();
  if (!copyIntoOwnedAudioBufferList(
          targetBuffer, audioBufferList, static_cast<unsigned int>(inputBufferBytesPerBuffer_))) {
    freeSlots_->release(slot.value());
    return;
  }

  CallbackData callbackData{.slot = slot.value(), .numFrames = numFrames};
  // send() cannot block here: we hold a slot from a pool of RECORDER_CALLBACK_POOL_SIZE,
  // and the channel is sized one larger (see static_assert in AudioRecorderCallback.h),
  // so the ring always has room while any slot is in flight.
  offloader_->getSender()->send(callbackData);
}

void IOSRecorderCallback::taskOffloaderFunction(CallbackData data)
{
  auto [slot, numFrames] = data;

  // The TaskOffloader destructor sends a default-constructed CallbackData
  // with a sentinel slot to unblock the receiver; ignore it here.
  if (slot == FreeList::kSentinel) {
    return;
  }

  if (slot >= inputBufferPool_.size() || freeSlots_ == nullptr) {
    return;
  }
  auto *inputBuffer = inputBufferPool_[slot].get();

  @autoreleasepool {
    NSError *error = nil;

    if (bufferFormat_.sampleRate == sampleRate_ && bufferFormat_.channelCount == channelCount_ &&
        !bufferFormat_.isInterleaved) {
      // Directly write to circular buffer
      for (int i = 0; i < channelCount_; ++i) {
        auto *data = static_cast<float *>(inputBuffer->mBuffers[i].mData);
        circularBuffer_[i]->push_back(data, numFrames);
      }

      freeSlots_->release(slot);
      if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
        emitAudioData();
      }
      return;
    }

    for (size_t i = 0; i < bufferFormat_.channelCount; ++i) {
      memcpy(
          converterInputBuffer_.mutableAudioBufferList->mBuffers[i].mData,
          inputBuffer->mBuffers[i].mData,
          inputBuffer->mBuffers[i].mDataByteSize);
    }

    freeSlots_->release(slot);
    converterInputBuffer_.frameLength = numFrames;

    __block BOOL handedOff = false;
    AVAudioConverterInputBlock inputBlock = ^AVAudioBuffer *_Nullable(
        AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
    {
      if (handedOff) {
        *outStatus = AVAudioConverterInputStatus_NoDataNow;
        return nil;
      }

      handedOff = true;
      *outStatus = AVAudioConverterInputStatus_HaveData;
      return converterInputBuffer_;
    };

    [converter_ convertToBuffer:converterOutputBuffer_ error:&error withInputFromBlock:inputBlock];

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error during audio conversion, native error: ") +
          [[error debugDescription] UTF8String]);
      return;
    }

    AVAudioFrameCount producedFrames = converterOutputBuffer_.frameLength;
    if (producedFrames == 0) {
      return;
    }

    for (int i = 0; i < channelCount_; ++i) {
      auto *data = static_cast<float *>(converterOutputBuffer_.audioBufferList->mBuffers[i].mData);
      circularBuffer_[i]->push_back(data, producedFrames);
    }

    if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
      emitAudioData();
    }
  }
}
} // namespace audioapi
