#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/IOSRecorderCallback.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/Result.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace audioapi {

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
/// This method should be called from the JS thread only.
void IOSRecorderCallback::cleanup()
{
  std::scoped_lock lock(callbackMutex_);
  @autoreleasepool {
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
    offloader_.reset();
  }
}

/// @brief Receives audio data from the recorder, processes it, and stores it in the circular buffer.
/// The data is converted using AVAudioConverter if the input format differs from the user desired callback format.
/// This method runs on the audio thread.
/// @param inputBuffer Pointer to the AudioBufferList containing the incoming audio data.
/// @param numFrames Number of frames in the input buffer.
void IOSRecorderCallback::receiveAudioData(const AudioBufferList *inputBuffer, int numFrames)
{
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  // CoreAudio owns `inputBuffer` only for the duration of this synchronous
  // callback. Copy into an owned AudioBufferList before handing off to the
  // worker thread; the consumer in taskOffloaderFunction frees it.
  UInt32 bufferCount = inputBuffer->mNumberBuffers;
  size_t headerSize = offsetof(AudioBufferList, mBuffers) + sizeof(AudioBuffer) * bufferCount;
  AudioBufferList *owned = static_cast<AudioBufferList *>(std::malloc(headerSize));
  if (owned == nullptr) {
    return;
  }
  owned->mNumberBuffers = bufferCount;
  for (UInt32 i = 0; i < bufferCount; ++i) {
    UInt32 byteSize = inputBuffer->mBuffers[i].mDataByteSize;
    owned->mBuffers[i].mNumberChannels = inputBuffer->mBuffers[i].mNumberChannels;
    owned->mBuffers[i].mDataByteSize = byteSize;
    void *channelData = std::malloc(byteSize);
    if (channelData == nullptr) {
      for (UInt32 j = 0; j < i; ++j) {
        std::free(owned->mBuffers[j].mData);
      }
      std::free(owned);
      return;
    }
    std::memcpy(channelData, inputBuffer->mBuffers[i].mData, byteSize);
    owned->mBuffers[i].mData = channelData;
  }

  offloader_->getSender()->send({owned, numFrames});
}

static inline void freeOwnedAudioBufferList(const AudioBufferList *bufferList)
{
  if (bufferList == nullptr) {
    return;
  }
  for (UInt32 i = 0; i < bufferList->mNumberBuffers; ++i) {
    std::free(bufferList->mBuffers[i].mData);
  }
  std::free(const_cast<AudioBufferList *>(bufferList));
}

void IOSRecorderCallback::taskOffloaderFunction(CallbackData data)
{
  auto [inputBuffer, numFrames] = data;

  // The TaskOffloader destructor sends a default-constructed CallbackData
  // (data == nullptr) to unblock the receiver; ignore it here.
  if (inputBuffer == nullptr)
    return;
  std::scoped_lock lock(callbackMutex_);
  @autoreleasepool {
    NSError *error = nil;

    if (bufferFormat_.sampleRate == sampleRate_ && bufferFormat_.channelCount == channelCount_ &&
        !bufferFormat_.isInterleaved) {
      // Directly write to circular buffer
      for (size_t i = 0; i < channelCount_; ++i) {
        auto *data = static_cast<float *>(inputBuffer->mBuffers[i].mData);
        circularBuffer_[i]->push_back(data, numFrames);
      }

      freeOwnedAudioBufferList(inputBuffer);
      inputBuffer = nullptr;
      if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
        emitAudioData();
      }
      return;
    }

    size_t outputFrameCount = ceil(numFrames * (sampleRate_ / bufferFormat_.sampleRate));

    for (size_t i = 0; i < bufferFormat_.channelCount; ++i) {
      memcpy(
          converterInputBuffer_.mutableAudioBufferList->mBuffers[i].mData,
          inputBuffer->mBuffers[i].mData,
          inputBuffer->mBuffers[i].mDataByteSize);
    }

    freeOwnedAudioBufferList(inputBuffer);
    inputBuffer = nullptr;
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
    converterOutputBuffer_.frameLength = sampleRate_ / bufferFormat_.sampleRate * numFrames;

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error during audio conversion, native error: ") +
          [[error debugDescription] UTF8String]);
      return;
    }

    for (size_t i = 0; i < channelCount_; ++i) {
      auto *data = static_cast<float *>(converterOutputBuffer_.audioBufferList->mBuffers[i].mData);
      circularBuffer_[i]->push_back(data, outputFrameCount);
    }

    if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
      emitAudioData();
    }
  }
}
} // namespace audioapi
