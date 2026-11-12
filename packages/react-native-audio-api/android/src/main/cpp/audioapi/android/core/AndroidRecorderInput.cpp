#include <android/log.h>
#include <audioapi/android/core/AndroidRecorderInput.h>
#include <audioapi/android/system/NativeInputRouting.h>

#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/utils/Constants.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace audioapi {

namespace {

/// Oboe's preset for each option. PlatformDefault sets none, which keeps Oboe's own default.
std::optional<oboe::InputPreset> toOboeInputPreset(
    AudioRecorderOptions::AndroidInputPreset preset) {
  using AndroidInputPreset = AudioRecorderOptions::AndroidInputPreset;
  switch (preset) {
    case AndroidInputPreset::PlatformDefault:
      return std::nullopt;
    case AndroidInputPreset::Generic:
      return oboe::InputPreset::Generic;
    case AndroidInputPreset::Camcorder:
      return oboe::InputPreset::Camcorder;
    case AndroidInputPreset::VoiceRecognition:
      return oboe::InputPreset::VoiceRecognition;
    case AndroidInputPreset::VoiceCommunication:
      return oboe::InputPreset::VoiceCommunication;
    case AndroidInputPreset::Unprocessed:
      return oboe::InputPreset::Unprocessed;
    case AndroidInputPreset::VoicePerformance:
      return oboe::InputPreset::VoicePerformance;
  }
  return std::nullopt;
}

} // namespace

AndroidRecorderInput::AndroidRecorderInput(const AudioRecorderOptions &options)
    : inputPreset_(options.androidInputPreset) {}

AndroidRecorderInput::~AndroidRecorderInput() {
  close();
}

RouteLease AndroidRecorderInput::acquireRoute() {
  return {NativeInputRouting::acquireInputRoute(), &NativeInputRouting::releaseInputRoute};
}

Result<StreamFormat, std::string> AndroidRecorderInput::open(
    RouteLease route,
    InputOpenPurpose purpose,
    std::shared_ptr<RecorderInputGate> gate,
    RecorderInputEvents events) {
  if (!route.deviceId().has_value()) {
    if (purpose == InputOpenPurpose::StartSession) {
      return Err("The selected Bluetooth microphone could not be connected.");
    }
    __android_log_print(
        ANDROID_LOG_WARN,
        LOG_TAG,
        "The selected input could not be routed, continuing on the default input");
  }

  const int32_t deviceId = route.deviceId().value_or(NativeInputRouting::kSystemDefaultDeviceId);
  auto streamResult = openStream(deviceId);
  if (streamResult.is_err()) {
    return Err(streamResult.unwrap_err());
  }

  auto stream = std::move(streamResult).unwrap();
  if (stream->getSampleRate() <= 0 || stream->getChannelCount() <= 0 ||
      stream->getBufferSizeInFrames() <= 0) {
    stream->close();
    return Err("Audio stream is not initialized.");
  }
  if (stream->getChannelCount() > MAX_CHANNEL_COUNT) {
    stream->close();
    return Err("Input channel count exceeds MAX_CHANNEL_COUNT");
  }

  // The selection is a preference, as on iOS: when the platform routes elsewhere (the device is
  // gone, or OpenSL ES, which ignores setDeviceId and reports kUnspecified), recording continues
  // on the routed device instead of failing.
  if (deviceId != NativeInputRouting::kSystemDefaultDeviceId && stream->getDeviceId() != deviceId) {
    __android_log_print(
        ANDROID_LOG_WARN,
        LOG_TAG,
        "Input device %d was requested, but the capture stream opened on device %d",
        deviceId,
        stream->getDeviceId());
  }
  NativeInputRouting::reportCaptureDevice(stream->getDeviceId());

  const auto sampleRate = static_cast<float>(stream->getSampleRate());
  channelCount_ = stream->getChannelCount();
  format_ = StreamFormat{
      .layout = {.sampleRate = sampleRate, .channelCount = channelCount_},
      .maxFramesPerBuffer = static_cast<size_t>(stream->getBufferSizeInFrames())};
  planarInput_ = AudioBuffer(format_.maxFramesPerBuffer, channelCount_, sampleRate);
  gate_ = std::move(gate);

  {
    std::scoped_lock streamLock(streamMutex_);
    stream_ = std::move(stream);
    events_ = std::move(events);
  }
  route_ = std::move(route);

  return Ok(format_);
}

Result<std::shared_ptr<oboe::AudioStream>, std::string> AndroidRecorderInput::openStream(
    int32_t deviceId) {
  oboe::AudioStreamBuilder builder;
  builder.setSharingMode(oboe::SharingMode::Exclusive)
      ->setDirection(oboe::Direction::Input)
      ->setFormat(oboe::AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(oboe::PerformanceMode::None)
      ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
      ->setDataCallback(shared_from_this())
      ->setErrorCallback(shared_from_this());

  if (auto preset = toOboeInputPreset(inputPreset_)) {
    builder.setInputPreset(*preset);
  }

  if (deviceId != NativeInputRouting::kSystemDefaultDeviceId) {
    builder.setDeviceId(deviceId);
  }

  std::shared_ptr<oboe::AudioStream> stream;
  auto result = builder.openStream(stream);

  if (result != oboe::Result::OK || stream == nullptr) {
    return Err("Failed to open audio stream: " + std::string(oboe::convertToText(result)));
  }
  return Ok(std::move(stream));
}

Result<NoneType, std::string> AndroidRecorderInput::start() {
  if (stream_ == nullptr) {
    return Err("Audio stream is not open");
  }

  auto result = stream_->requestStart();
  if (result != oboe::Result::OK) {
    return Err("Failed to start stream: " + std::string(oboe::convertToText(result)));
  }

  isStarted_.store(true, std::memory_order_release);
  return Ok(None);
}

/// Stops the stream and keeps it open for resume(). AAudio cannot pause an input stream, so a
/// stopped one is what lets the microphone go idle.
void AndroidRecorderInput::pause() {
  isStarted_.store(false, std::memory_order_release);

  if (stream_ != nullptr) {
    stream_->stop();
  }
}

Result<NoneType, std::string> AndroidRecorderInput::resume() {
  if (stream_ == nullptr) {
    return Err("Audio stream is not open");
  }

  auto result = stream_->start();
  if (result != oboe::Result::OK) {
    return Err("Failed to resume stream: " + std::string(oboe::convertToText(result)));
  }

  isStarted_.store(true, std::memory_order_release);
  return Ok(None);
}

void AndroidRecorderInput::close() {
  isStarted_.store(false, std::memory_order_release);

  {
    std::scoped_lock streamLock(streamMutex_);
    if (stream_ != nullptr) {
      stream_->requestStop();
      stream_->close();
      stream_.reset();
    }
  }

  route_ = RouteLease{};
}

Result<StreamFormat, std::string> AndroidRecorderInput::currentFormat() const {
  if (stream_ == nullptr) {
    return Err("audio stream is not open");
  }
  return Ok(format_);
}

bool AndroidRecorderInput::isRunning() const {
  return isStarted_.load(std::memory_order_acquire);
}

double AndroidRecorderInput::latency(double callbackSeconds) const {
  std::scoped_lock streamLock(streamMutex_);

  if (stream_ == nullptr) {
    return 0.0;
  }

  const auto latencyResult = stream_->calculateLatencyMillis();
  if (latencyResult && latencyResult.value() > 0.0) {
    return latencyResult.value() / 1000.0;
  }

  if (callbackSeconds > 0.0) {
    return callbackSeconds;
  }

  const int32_t framesPerBurst = stream_->getFramesPerBurst();
  const int32_t sampleRate = stream_->getSampleRate();
  if (framesPerBurst > 0 && sampleRate > 0) {
    return static_cast<double>(framesPerBurst) / static_cast<double>(sampleRate);
  }

  return 0.0;
}

oboe::DataCallbackResult AndroidRecorderInput::onAudioReady(
    oboe::AudioStream * /*stream*/,
    void *audioData,
    int32_t numFrames) {
  const auto frames = static_cast<size_t>(numFrames);
  if (audioData == nullptr || numFrames <= 0 || frames > planarInput_.getSize()) {
    return oboe::DataCallbackResult::Continue;
  }

  planarInput_.deinterleaveFrom(static_cast<const float *>(audioData), frames);

  std::array<const float *, MAX_CHANNEL_COUNT> channels{};
  for (int channel = 0; channel < channelCount_; ++channel) {
    channels[channel] = planarInput_.getChannel(channel)->begin();
  }
  gate_->deliver(channels.data(), channelCount_, numFrames);

  return oboe::DataCallbackResult::Continue;
}

void AndroidRecorderInput::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  if (error != oboe::Result::ErrorDisconnected) {
    return;
  }

  // The reroute below destroys the stream, which holds a reference to this input, and can stop the
  // session, which drops the recorder's.
  auto self = shared_from_this();

  std::optional<RecorderInputEvents> events;
  {
    std::scoped_lock streamLock(streamMutex_);
    // Reported late, for a stream that was already closed or replaced.
    if (stream_.get() != stream) {
      return;
    }
    events = events_;
  }

  events->lost();
}

std::shared_ptr<IRecorderInput> createPlatformRecorderInput(const AudioRecorderOptions &options) {
  return std::make_shared<AndroidRecorderInput>(options);
}

} // namespace audioapi
