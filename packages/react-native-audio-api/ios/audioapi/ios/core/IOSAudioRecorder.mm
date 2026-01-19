#import <AVFoundation/AVFoundation.h>
#import <AudioEngine.h>
#import <AudioSessionManager.h>
#import <Foundation/Foundation.h>

#include <unordered_map>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/IOSRecorderCallback.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

/// @brief Constructs an IOSAudioRecorder instance.
/// This constructor initializes the receiver block and native side recorder wrapper (AVAudioSinkNode).
/// All other necessary fields (like buffers) are initialized in start() method.
/// This "method" should be called from the JS thread only.
/// @param audioEventHandlerRegistry Shared pointer to the AudioEventHandlerRegistry for event handling.
IOSAudioRecorder::IOSAudioRecorder(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(audioEventHandlerRegistry)
{
  AudioReceiverBlock receiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames) {
    if (usesFileOutput()) {
      if (auto lock = Locker::tryLock(fileWriterMutex_)) {
        std::static_pointer_cast<IOSFileWriter>(fileWriter_)
            ->writeAudioData(inputBuffer, numFrames);
      }
    }

    if (usesCallback()) {
      if (auto lock = Locker::tryLock(callbackMutex_)) {
        std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
            ->receiveAudioData(inputBuffer, numFrames);
      }
    }

    if (isConnected()) {
      if (auto lock = Locker::tryLock(adapterNodeMutex_)) {
        for (size_t channel = 0; channel < adapterNode_->channelCount_; ++channel) {
          float *channelData = (float *)inputBuffer->mBuffers[channel].mData;

          adapterNode_->buff_[channel]->write(channelData, numFrames);
        }
      }
    }
  };

  nativeRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:receiverBlock];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  stop();
  [nativeRecorder_ cleanup];
}

/// @brief Starts the audio recording process and prepares necessary resources.
/// This method should be called from the JS thread only.
/// @returns Result containing the file path if recording started successfully, or an error message.
Result<std::string, std::string> IOSAudioRecorder::start(const std::string &fileNameOverride)
{
  if (!isIdle()) {
    return Result<std::string, std::string>::Err("Recorder is already recording");
  }

  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);
  AudioSessionManager *audioSessionManager = [AudioSessionManager sharedInstance];

  if ([[audioSessionManager checkRecordingPermissions] isEqual:@"Denied"]) {
    return Result<std::string, std::string>::Err("Microphone permissions are not granted");
  }

  // TODO: recorder should probably request the options if not set by user
  // but lets handle that in another PR
  if (![audioSessionManager isSessionActive]) {
    return Result<std::string, std::string>::Err("Audio session is not active");
  }

  // TODO: this is a bit ugly, and could be written slightly better
  // we need to stop the audio engine if it's running, to be able to get
  // proper input format values, otherwise the system my zero out the sample rate or channel count
  // if input wasn't used yet
  // (especially on simulators)
  // Engine will be started again once the native recorder starts
  [AudioEngine.sharedInstance stopIfNecessary];

  // Estimate the maximum input buffer lengths that can be expected from the sink node
  size_t maxInputBufferLength = [nativeRecorder_ getBufferSize];
  auto inputFormat = [nativeRecorder_ getInputFormat];

  if (usesFileOutput()) {
    auto fileResult = std::static_pointer_cast<IOSFileWriter>(fileWriter_)
                          ->openFile(inputFormat, maxInputBufferLength, fileNameOverride);

    if (fileResult.is_err()) {
      return Result<std::string, std::string>::Err(
          "Failed to open file for writing: " + fileResult.unwrap_err());
    }

    filePath_ = fileResult.unwrap();
  }

  if (usesCallback()) {
    auto callbackResult = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                              ->prepare(inputFormat, maxInputBufferLength);

    if (callbackResult.is_err()) {
      return Result<std::string, std::string>::Err(
          "Failed to prepare callback: " + callbackResult.unwrap_err());
    }
  }

  if (isConnected()) {
    // TODO: pass sample rate, in case conversion is necessary
    adapterNode_->init(maxInputBufferLength, inputFormat.channelCount);
  }

  [nativeRecorder_ start];
  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<std::string, std::string>::Ok(filePath_);
}

/// @brief Stops the audio recording process and releases resources.
/// It finalizes any data receiver and closes the stream.
/// This method should be called from the JS thread only.
/// @returns Result containing a tuple of the output file path, size, and duration if stopped successfully, or an error message.
Result<std::tuple<std::string, double, double>, std::string> IOSAudioRecorder::stop()
{
  std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

  std::string filePath = filePath_;
  double outputFileSize = 0;
  double outputDuration = 0;

  if (isIdle()) {
    return Result<std::tuple<std::string, double, double>, std::string>::Err(
        "Recorder is not in recording state.");
  }

  state_.store(RecorderState::Idle, std::memory_order_release);
  [nativeRecorder_ stop];

  if (usesFileOutput()) {
    auto fileResult = fileWriter_->closeFile();

    if (fileResult.is_err()) {
      return Result<std::tuple<std::string, double, double>, std::string>::Err(
          "Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());
  }

  if (usesCallback()) {
    dataCallback_->cleanup();
  }

  if (isConnected()) {
    adapterNode_->cleanup();
  }

  filePath_ = "";
  return Result<std::tuple<std::string, double, double>, std::string>::Ok(
      std::make_tuple(filePath, outputFileSize, outputDuration));
}

/// @brief Enables file output for the recorder with specified properties.
/// If the recorder is already active, it will open the file for writing immediately.
/// This method should be called from the JS thread only.
/// @param properties Shared pointer to AudioFileProperties defining the output file format.
/// @returns Result containing the output file path if enabled successfully, or an error message.
Result<std::string, std::string> IOSAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties)
{
  std::scoped_lock lock(fileWriterMutex_, errorCallbackMutex_);
  fileWriter_ = std::make_shared<IOSFileWriter>(audioEventHandlerRegistry_, properties);

  if (!isIdle()) {
    auto result =
        std::static_pointer_cast<IOSFileWriter>(fileWriter_)
            ->openFile([nativeRecorder_ getInputFormat], [nativeRecorder_ getBufferSize], "");

    if (result.is_err()) {
      return Result<std::string, std::string>::Err(
          "Failed to open file for writing: " + result.unwrap_err());
    }

    filePath_ = result.unwrap();
  }

  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  fileOutputEnabled_.store(true, std::memory_order_release);
  return Result<std::string, std::string>::Ok(filePath_);
}

void IOSAudioRecorder::disableFileOutput()
{
  std::scoped_lock lock(fileWriterMutex_);
  fileOutputEnabled_.store(false, std::memory_order_release);
  fileWriter_ = nullptr;
}

/// @brief Connects a RecorderAdapterNode to the recorder for audio data routing.
/// If the recorder is already active, it will initialize the adapter node immediately.
/// This method should be called from the JS thread only.
/// @param node Shared pointer to the RecorderAdapterNode to connect.
void IOSAudioRecorder::connect(const std::shared_ptr<RecorderAdapterNode> &node)
{
  std::scoped_lock lock(adapterNodeMutex_);
  adapterNode_ = node;

  if (!isIdle()) {
    adapterNode_->init(
        [nativeRecorder_ getBufferSize], [nativeRecorder_ getInputFormat].channelCount);
  }

  isConnected_.store(true, std::memory_order_release);
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it will stop routing audio data immediately.
/// This method should be called from the JS thread only.
void IOSAudioRecorder::disconnect()
{
  std::scoped_lock lock(adapterNodeMutex_);
  adapterNode_ = nullptr;
  isConnected_.store(false, std::memory_order_release);
}

void IOSAudioRecorder::pause()
{
  if (!isRecording()) {
    return;
  }

  [nativeRecorder_ pause];
  state_.store(RecorderState::Paused, std::memory_order_release);
}

void IOSAudioRecorder::resume()
{
  if (!isPaused()) {
    return;
  }

  [nativeRecorder_ resume];
  state_.store(RecorderState::Recording, std::memory_order_release);
}

/// @brief Checks if the recorder is currently recording.
/// Besides recorder internal state, it also check if the audio engine is running.
/// this helps with restarts after interruptions or other audio session changes.
/// This method can be called from any thread.
/// @returns True if recording, false otherwise.
bool IOSAudioRecorder::isRecording() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  return state_.load(std::memory_order_acquire) == RecorderState::Recording &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

/// @brief Checks if the recorder is currently paused.
/// Besides recorder internal state, it also check if the audio engine is running.
/// this helps with restarts after interruptions or other audio session changes.
/// This method can be called from any thread.
/// @returns True if paused, false otherwise.
bool IOSAudioRecorder::isPaused() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  auto currentState = state_.load(std::memory_order_acquire);

  if (currentState == RecorderState::Idle) {
    return false;
  }

  return currentState == RecorderState::Paused ||
      [audioEngine getState] != AudioEngineState::AudioEngineStateRunning;
}

/// @brief Checks if the recorder is currently idle (not recording or paused).
/// This method can be called from any thread.
/// @returns True if idle, false otherwise.
bool IOSAudioRecorder::isIdle() const
{
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

/// @brief Sets the callback to be invoked when audio data is ready.
/// If the recorder is already active, it will prepare the callback for receiving audio data immediately.
/// This method should be called from the JS thread only.
/// @param sampleRate Desired sample rate for the callback audio data.
/// @param bufferLength Desired buffer length in frames for the callback audio data.
/// @param channelCount Number of channels for the callback audio data.
/// @param callbackId Identifier for the JS callback to be invoked.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> IOSAudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId)
{
  std::scoped_lock lock(callbackMutex_, errorCallbackMutex_);

  dataCallback_ = std::make_shared<IOSRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);

  if (!isIdle()) {
    auto result = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                      ->prepare([nativeRecorder_ getInputFormat], [nativeRecorder_ getBufferSize]);

    if (result.is_err()) {
      return Result<NoneType, std::string>::Err(result.unwrap_err());
    }
  }

  dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  callbackOutputEnabled_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Clears the audio data callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void IOSAudioRecorder::clearOnAudioReadyCallback()
{
  std::scoped_lock lock(callbackMutex_);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

} // namespace audioapi
