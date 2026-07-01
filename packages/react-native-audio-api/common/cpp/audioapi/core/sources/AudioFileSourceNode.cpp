#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/core/AudioContext.h>

#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>
#include <utility>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      decoderState_(std::make_shared<AudioFileDecoderState>()),
      volume_(options.volume),
      loop_(options.loop),
      targetPlaybackRate_(options.playbackRate),
      positionChanged_(
          context->getAudioEventHandlerRegistry(),
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL),
          true) {
  decoderState_->playbackRate.store(options.playbackRate, std::memory_order_release);
  decoderState_->preservesPitch.store(options.preservesPitch, std::memory_order_release);

  const bool useFilePath = !options.filePath.empty();
  const bool useData = !options.data.empty();
  const bool useUrl = !options.sourceUrl.empty();

  if (!useFilePath && !useData && !useUrl) {
    assert(
        false &&
        "AudioFileSourceNode requires a file path, remote URL, or memory data to initialize");
    return;
  }

  if (!initDecoder(context, options)) {
    return;
  }
}

AudioFileSourceNode::~AudioFileSourceNode() {
  stopDaemonThread();
}

void AudioFileSourceNode::stopDaemonThread() {
  decoderState_->isDaemonRunning.store(false, std::memory_order_release);

  // commandSender_ is only created in initDecoder(); skip if construction failed early
  // (e.g. neither filePath nor data provided) or teardown already completed.
  if (!seekDecoderThread_.joinable() && seekDecoderDaemon_ == nullptr) {
    return;
  }

  // Send a dummy command to unblock the daemon thread if it's waiting.
  // The command channel uses OVERWRITE_ON_FULL, so this never blocks.
  commandSender_.send(SeekRequest{0});
  if (seekDecoderThread_.joinable()) {
    seekDecoderThread_.join();
  }
}

void AudioFileSourceNode::sendOnPositionChangedEvent(int framesPlayed) {
  const double delta = static_cast<double>(framesPlayed) / sampleRate_;
  const double position = currentTime_.fetch_add(delta) + delta;
  positionChanged_.advance(framesPlayed, position);
}

void AudioFileSourceNode::assignOnPositionChangedCallbackId(uint64_t callbackId) {
  positionChanged_.assignCallbackId(callbackId);
}

bool AudioFileSourceNode::initDecoder(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceOptions &options) {
  auto [frameSender, frameReceiver] =
      channels::spsc::channel<DecoderData, FRAME_OVERFLOW_STRATEGY, FRAME_WAIT_STRATEGY>(
          FRAME_CHANNEL_CAPACITY);
  frameReceiver_ = std::make_shared<FrameReceiver>(std::move(frameReceiver));

  auto [commandSender, commandReceiver] =
      channels::spsc::channel<SeekRequest, COMMAND_OVERFLOW_STRATEGY, COMMAND_WAIT_STRATEGY>(
          COMMAND_CHANNEL_CAPACITY);
  commandSender_ = std::move(commandSender);

  SeekDecoderDaemonOptions daemonOptions{
      .requiresFFmpeg = options.requiresFFmpeg,
      .filePath = std::move(options.filePath),
      .sourceUrl = std::move(options.sourceUrl),
      .httpHeaders = std::move(options.httpHeaders),
      .memoryData = std::move(options.data),
      .contextSampleRate = context->getSampleRate(),
      .loop = options.loop};

  seekDecoderDaemon_ = std::make_unique<SeekDecoderDaemon>(
      std::move(daemonOptions),
      decoderState_,
      std::move(commandReceiver),
      std::move(frameSender),
      frameReceiver_);

  if (!decoderState_->isReady.load(std::memory_order_acquire)) {
    return false;
  }

  channelCount_ = decoderState_->channelCount;
  sampleRate_ = decoderState_->sampleRate;
  duration_ = decoderState_->duration;

  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      static_cast<size_t>(RENDER_QUANTUM_SIZE), channelCount_, context->getSampleRate());
  wsolaStretcher_.configure(static_cast<size_t>(channelCount_), static_cast<float>(sampleRate_));
  playbackRateBuffer_ = std::make_shared<DSPAudioBuffer>(
      std::max(
          static_cast<size_t>(DecoderData::MAX_FRAMES), wsolaStretcher_.getRequiredInputFrames()),
      channelCount_,
      context->getSampleRate());

  return true;
}

void AudioFileSourceNode::setPlaybackRate(float v) {
  if (decoderState_ == nullptr) {
    return;
  }

  if (!std::isfinite(v) || v < 0.0f || v > WsolaTimeStretcher::MAX_PLAYBACK_RATE) {
    return;
  }

  decoderState_->playbackRate.store(v, std::memory_order_release);
  targetPlaybackRate_ = v;
}

void AudioFileSourceNode::setPreservesPitch(bool v) {
  if (decoderState_ == nullptr) {
    return;
  }

  const bool previous = decoderState_->preservesPitch.exchange(v, std::memory_order_acq_rel);
  if (previous != v) {
    handlePitchPreservationModeChanged();
  }
}

void AudioFileSourceNode::resetPitchPreservationState() {
  wsolaStretcher_.reset();
}

void AudioFileSourceNode::clearPendingDecoderChunk() {
  pendingDecoderChunk_.size = 0;
}

void AudioFileSourceNode::stashPendingDecoderChunk(
    const DecoderData &chunk,
    size_t consumedFrames) {
  if (consumedFrames >= chunk.size) {
    return;
  }

  const size_t remaining = chunk.size - consumedFrames;
  const auto channels = static_cast<size_t>(channelCount_);

  pendingDecoderChunk_.state = chunk.state;
  pendingDecoderChunk_.timestamp = chunk.timestamp;
  pendingDecoderChunk_.size = remaining;

  std::memcpy(
      pendingDecoderChunk_.interleavedBuffer.data(),
      chunk.interleavedBuffer.data() + consumedFrames * channels,
      remaining * channels * sizeof(float));
}

void AudioFileSourceNode::consumePendingDecoderChunkFront(size_t consumedFrames) {
  if (consumedFrames == 0 || pendingDecoderChunk_.size == 0) {
    return;
  }

  if (consumedFrames >= pendingDecoderChunk_.size) {
    clearPendingDecoderChunk();
    return;
  }

  const size_t remaining = pendingDecoderChunk_.size - consumedFrames;
  const auto channels = static_cast<size_t>(channelCount_);

  std::memmove(
      pendingDecoderChunk_.interleavedBuffer.data(),
      pendingDecoderChunk_.interleavedBuffer.data() + consumedFrames * channels,
      remaining * channels * sizeof(float));

  pendingDecoderChunk_.size = remaining;
}

void AudioFileSourceNode::drainPendingFrames() {
  clearPendingDecoderChunk();

  if (frameReceiver_ == nullptr) {
    return;
  }

  DecoderData drop;
  while (frameReceiver_->try_receive(drop) == ResponseStatus::SUCCESS) {}
}

void AudioFileSourceNode::handlePitchPreservationModeChanged() {
  drainPendingFrames();
  resetPitchPreservationState();
  lastFillMode_ = FillMode::Passthrough;
}

bool AudioFileSourceNode::ensurePlaybackRateBufferSize(size_t frames) {
  if (frames == 0) {
    return true;
  }

  if (playbackRateBuffer_ == nullptr || playbackRateBuffer_->getSize() < frames) {
    const float bufferSampleRate =
        audioBuffer_ != nullptr ? audioBuffer_->getSampleRate() : static_cast<float>(sampleRate_);
    playbackRateBuffer_ = std::make_shared<DSPAudioBuffer>(frames, channelCount_, bufferSampleRate);
  }

  return playbackRateBuffer_ != nullptr;
}

AudioFileSourceNode::FillMode AudioFileSourceNode::chooseFillMode(
    bool preservesPitch,
    bool rateAffectsOutput) {
  if (!preservesPitch) {
    return FillMode::Resample;
  }

  if (rateAffectsOutput) {
    return FillMode::Wsola;
  }

  return FillMode::Passthrough;
}

void AudioFileSourceNode::setFillMode(FillMode mode) {
  if (mode == lastFillMode_) {
    return;
  }

  if (lastFillMode_ == FillMode::Wsola) {
    resetPitchPreservationState();
  }

  lastFillMode_ = mode;
  if (hasPreviousOutputFrame_) {
    transitionFadeFramesRemaining_ = MODE_TRANSITION_FADE_FRAMES;
  }
}

void AudioFileSourceNode::applyModeTransitionFade(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer) {
  if (transitionFadeFramesRemaining_ == 0 || processingBuffer == nullptr ||
      !hasPreviousOutputFrame_) {
    return;
  }

  const size_t frames = std::min(transitionFadeFramesRemaining_, processingBuffer->getSize());
  if (frames == 0) {
    return;
  }

  const size_t channels =
      std::min(processingBuffer->getNumberOfChannels(), static_cast<size_t>(MAX_CHANNEL_COUNT));
  for (size_t ch = 0; ch < channels; ++ch) {
    float *data = processingBuffer->getChannel(ch)->begin();
    const float startSample = previousOutputFrame_[ch];
    for (size_t frame = 0; frame < frames; ++frame) {
      const float mix = static_cast<float>(frame + 1) / static_cast<float>(frames);
      data[frame] = startSample * (1.0f - mix) + data[frame] * mix;
    }
  }

  transitionFadeFramesRemaining_ -= frames;
}

void AudioFileSourceNode::applyFadeOutToSilence(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer) {
  if (processingBuffer == nullptr) {
    return;
  }

  const size_t frames = processingBuffer->getSize();
  if (frames == 0) {
    return;
  }

  const size_t channels =
      std::min(processingBuffer->getNumberOfChannels(), static_cast<size_t>(MAX_CHANNEL_COUNT));

  if (!hasPreviousOutputFrame_) {
    processingBuffer->zero();
    return;
  }

  for (size_t ch = 0; ch < channels; ++ch) {
    float *data = processingBuffer->getChannel(ch)->begin();
    const float startSample = previousOutputFrame_[ch];
    for (size_t frame = 0; frame < frames; ++frame) {
      const float mix = 1.0f - static_cast<float>(frame + 1) / static_cast<float>(frames);
      data[frame] = startSample * mix;
    }
  }
}

void AudioFileSourceNode::captureLastOutputFrame(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer) {
  if (processingBuffer == nullptr || processingBuffer->getSize() == 0) {
    return;
  }

  const size_t lastFrame = processingBuffer->getSize() - 1;
  const size_t channels =
      std::min(processingBuffer->getNumberOfChannels(), static_cast<size_t>(MAX_CHANNEL_COUNT));

  for (size_t ch = 0; ch < channels; ++ch) {
    previousOutputFrame_[ch] = processingBuffer->getChannel(ch)->begin()[lastFrame];
  }
  hasPreviousOutputFrame_ = true;
}

std::shared_ptr<DSPAudioBuffer> AudioFileSourceNode::handleEndOfStreamPlayback(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    float activeRate) {
  const auto frames = static_cast<size_t>(framesToProcess);
  processingBuffer->zero();

  // Play out any audio still buffered inside the time-stretcher. While it keeps
  // filling whole quanta we are still mid-tail, so keep draining on the next call.
  const size_t drained = lastFillMode_ == FillMode::Wsola
      ? wsolaStretcher_.drainOutput(*processingBuffer, frames, activeRate)
      : 0;
  if (drained >= frames) {
    captureLastOutputFrame(processingBuffer);
    return processingBuffer;
  }

  // The tail has run out. Fade the whole final quantum from the last rendered
  // sample down to silence (a full render quantum ~2.7ms ramp) so the stop is
  // inaudible. A short partial-tail fade is too abrupt and clicks, so we always
  // ramp across the full quantum starting from previousOutputFrame_.
  applyFadeOutToSilence(processingBuffer);
  captureLastOutputFrame(processingBuffer);
  resetPitchPreservationState();
  endOfStreamDrainPending_ = false;
  endOfStreamStopPending_ = true;
  return processingBuffer;
}

//void AudioFileSourceNode::connect(const std::shared_ptr<AudioNode> &node) {
//  if (isRoutedThroughMediaElement()) {
//    return;
//  }
//
//  AudioScheduledSourceNode::connect(node);
//}
//

void AudioFileSourceNode::start(double when) {
  AudioScheduledSourceNode::start(when);
  filePaused_ = false;
  endOfStreamStopPending_ = false;
  endOfStreamDrainPending_ = false;
  positionChanged_.requestFlush();

  if (seekDecoderDaemon_) {
    seekDecoderThread_ = std::thread(std::move(*seekDecoderDaemon_));
    seekDecoderDaemon_.reset();
  }
}

void AudioFileSourceNode::bindMediaElementSource(uint64_t bindingId) {
  activeMediaBindingId_.store(bindingId, std::memory_order_release);
}

void AudioFileSourceNode::releaseMediaElementSource(uint64_t bindingId) {
  uint64_t expected = bindingId;
  if (!activeMediaBindingId_.compare_exchange_strong(
          expected, 0, std::memory_order_acq_rel, std::memory_order_acquire)) {
    return;
  }

  ensureConnectedForDirectPlayback();
}

void AudioFileSourceNode::ensureConnectedForDirectPlayback() {
  if (filePaused_ || isUnscheduled() || isFinished()) {
    return;
  }

  //  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
  //    connect(context->getDestination());
  //  }
}

bool AudioFileSourceNode::isCurrentMediaElementSource(uint64_t bindingId) const {
  if (bindingId == 0) {
    return false;
  }

  return activeMediaBindingId_.load(std::memory_order_acquire) == bindingId;
}

void AudioFileSourceNode::pause() {
  filePaused_ = true;
}

void AudioFileSourceNode::disable() {
  stopDaemonThread();
  filePaused_ = false;
  endOfStreamStopPending_ = false;
  endOfStreamDrainPending_ = false;

  AudioScheduledSourceNode::disable();
}

void AudioFileSourceNode::seekToTime(double seconds) {
  if (decoderState_ == nullptr) {
    return;
  }
  const double dur = duration_;
  if (dur > 0) {
    seconds = std::clamp(seconds, 0.0, dur);
  } else {
    seconds = std::max(0.0, seconds);
  }
  decoderState_->pendingOffloadedSeeks.fetch_add(1, std::memory_order_acq_rel);
  commandSender_.send(SeekRequest{seconds});
}

bool AudioFileSourceNode::readNextFrameChunk(DecoderData &outData) {
  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    return false;
  }
  return frameReceiver_->try_receive(outData) == ResponseStatus::SUCCESS;
}

size_t AudioFileSourceNode::appendFromInterleaved(
    const float *interleaved,
    size_t frameCount,
    size_t startFrame,
    size_t framesNeeded,
    size_t &totalInputFrames) {
  if (playbackRateBuffer_ == nullptr || frameCount <= startFrame ||
      totalInputFrames >= framesNeeded) {
    return 0;
  }

  const size_t capacity = playbackRateBuffer_->getSize();
  if (totalInputFrames >= capacity) {
    return 0;
  }

  const size_t available = frameCount - startFrame;
  const size_t framesStillNeeded = framesNeeded - totalInputFrames;
  const size_t frames = std::min({available, capacity - totalInputFrames, framesStillNeeded});

  if (frames == 0) {
    return 0;
  }

  const auto channels = static_cast<size_t>(channelCount_);
  playbackRateBuffer_->deinterleaveFrom(
      interleaved + startFrame * channels, totalInputFrames, frames);

  totalInputFrames += frames;
  return frames;
}

size_t AudioFileSourceNode::renderWithWsolaPitchPreservation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    DecoderData &incoming,
    int framesToProcess,
    float activeRate) {
  const auto requestedInputFrames =
      static_cast<size_t>(std::ceil(activeRate * static_cast<float>(framesToProcess)));
  const size_t bufferedInputFrames = wsolaStretcher_.getBufferedInputFrames();
  const size_t requiredInputFrames = wsolaStretcher_.getRequiredInputFrames();
  const size_t warmupFramesNeeded =
      bufferedInputFrames < requiredInputFrames ? requiredInputFrames - bufferedInputFrames : 0;
  const size_t inputFrames = accumulateStretchInput(
      incoming, activeRate, framesToProcess, requestedInputFrames + warmupFramesNeeded);
  if (inputFrames == 0) {
    processingBuffer->zero();
    return 0;
  }

  wsolaStretcher_.process(
      *playbackRateBuffer_,
      inputFrames,
      *processingBuffer,
      static_cast<size_t>(framesToProcess),
      activeRate);

  return std::min(inputFrames, requestedInputFrames);
}

size_t AudioFileSourceNode::accumulateStretchInput(
    DecoderData &incoming,
    float activeRate,
    int framesToProcess,
    size_t minFramesNeeded) {
  const size_t framesNeeded = std::max(
      minFramesNeeded,
      static_cast<size_t>(std::ceil(activeRate * static_cast<float>(framesToProcess))));

  // The decoder delivers fixed-size chunks, so the final chunk may overshoot
  // framesNeeded by up to one chunk. Reserve that headroom to avoid writing
  // past the buffer (heap overflow) when accumulating multiple chunks.
  const size_t bufferCapacity =
      std::max(framesNeeded, incoming.size) + static_cast<size_t>(RENDER_QUANTUM_SIZE);
  if (!ensurePlaybackRateBufferSize(bufferCapacity)) {
    return 0;
  }

  playbackRateBuffer_->zero();
  size_t totalInputFrames = 0;

  if (pendingDecoderChunk_.size > 0) {
    const size_t copiedFromPending = appendFromInterleaved(
        pendingDecoderChunk_.interleavedBuffer.data(),
        pendingDecoderChunk_.size,
        0,
        framesNeeded,
        totalInputFrames);
    consumePendingDecoderChunkFront(copiedFromPending);
  }

  size_t incomingConsumed = 0;
  if (totalInputFrames < framesNeeded && incoming.size > 0) {
    incomingConsumed = appendFromInterleaved(
        incoming.interleavedBuffer.data(), incoming.size, 0, framesNeeded, totalInputFrames);
  }

  DecoderData extra;
  while (totalInputFrames < framesNeeded && readNextFrameChunk(extra)) {
    if (extra.state == StreamState::END_OF_STREAM) {
      break;
    }
    if (extra.state == StreamState::DISCONTINUOUS) {
      currentTime_.store(extra.timestamp, std::memory_order_release);
      resetPitchPreservationState();
      clearPendingDecoderChunk();
      break;
    }

    const size_t extraConsumed = appendFromInterleaved(
        extra.interleavedBuffer.data(), extra.size, 0, framesNeeded, totalInputFrames);
    if (extraConsumed < extra.size) {
      stashPendingDecoderChunk(extra, extraConsumed);
      break;
    }
  }

  if (incoming.size > 0 && incomingConsumed < incoming.size && pendingDecoderChunk_.size == 0) {
    stashPendingDecoderChunk(incoming, incomingConsumed);
  }

  if (totalInputFrames == 0) {
    return 0;
  }

  if (volume_ != 1.0f) {
    playbackRateBuffer_->scale(volume_);
  }

  return totalInputFrames;
}

size_t AudioFileSourceNode::renderWithoutPitchPreservation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    DecoderData &incoming,
    int framesToProcess,
    float playbackRate) {
  const size_t inputFrames = accumulateStretchInput(incoming, playbackRate, framesToProcess);
  if (inputFrames == 0 || framesToProcess <= 0) {
    processingBuffer->zero();
    return 0;
  }

  processingBuffer->zero();

  const auto outputFrames = static_cast<size_t>(framesToProcess);
  const float step = inputFrames > 1 && outputFrames > 1
      ? static_cast<float>(inputFrames - 1) / static_cast<float>(outputFrames - 1)
      : 0.0f;

  for (size_t channel = 0; channel < static_cast<size_t>(channelCount_); ++channel) {
    const float *input = playbackRateBuffer_->getChannel(channel)->begin();
    float *output = processingBuffer->getChannel(channel)->begin();

    for (size_t frame = 0; frame < outputFrames; ++frame) {
      const float position = static_cast<float>(frame) * step;
      const auto index = static_cast<size_t>(position);
      const auto nextIndex = std::min(index + 1, inputFrames - 1);
      const float fraction = position - static_cast<float>(index);
      output[frame] = input[index] + (input[nextIndex] - input[index]) * fraction;
    }
  }

  return inputFrames;
}

void AudioFileSourceNode::processNode(int framesToProcess) {
  if (isRoutedThroughMediaElement()) {
    audioBuffer_->zero();
    return;
  }
  processDecodedOutput(framesToProcess);
}

std::optional<std::shared_ptr<DSPAudioBuffer>> AudioFileSourceNode::renderPreflight(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    std::shared_ptr<BaseAudioContext> &outContext) {
  if (decoderState_ == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (endOfStreamStopPending_) {
    endOfStreamStopPending_ = false;
    filePaused_ = true;
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    handleStopScheduled();
    processingBuffer->zero();
    return processingBuffer;
  }

  if (endOfStreamDrainPending_) {
    return handleEndOfStreamPlayback(processingBuffer, framesToProcess, eofDrainRate_);
  }

  if (filePaused_) {
    processingBuffer->zero();
    return processingBuffer;
  }

  outContext = context_.lock();
  if (outContext == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    processingBuffer->zero();
    return processingBuffer;
  }

  return std::nullopt;
}

void AudioFileSourceNode::processDecodedOutput(
    int framesToProcess,
    const std::shared_ptr<DSPAudioBuffer> &outputBuffer) {
  const std::shared_ptr<DSPAudioBuffer> &processingBuffer =
      outputBuffer != nullptr ? outputBuffer : audioBuffer_;

  std::shared_ptr<BaseAudioContext> context;
  if (renderPreflight(processingBuffer, framesToProcess, context)) {
    return;
  }

  size_t startOffset = 0;
  size_t offsetLength = 0;
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return;
  }

  if (startOffset > 0) {
    processingBuffer->zero(0, startOffset);
  }

  const bool preservesPitch = decoderState_->preservesPitch.load(std::memory_order_acquire);
  const bool rateAffectsOutput = std::abs(targetPlaybackRate_ - 1.0f) > RATE_SETTLE_EPSILON;
  const auto framesNeededForRate =
      static_cast<size_t>(std::ceil(targetPlaybackRate_ * static_cast<float>(framesToProcess)));

  DecoderData incoming{};
  const bool needsFreshDecoderChunk = pendingDecoderChunk_.size < framesNeededForRate;
  const bool hasFreshChunk = needsFreshDecoderChunk && readNextFrameChunk(incoming);

  if (!hasFreshChunk && pendingDecoderChunk_.size == 0) {
    processingBuffer->zero();
    return;
  }

  if (hasFreshChunk && incoming.state == StreamState::END_OF_STREAM) {
    currentTime_.store(duration_, std::memory_order_release);
    sendOnPositionChangedEvent(0);
    clearPendingDecoderChunk();

    endOfStreamDrainPending_ = true;
    eofDrainRate_ = targetPlaybackRate_;
    handleEndOfStreamPlayback(processingBuffer, framesToProcess, targetPlaybackRate_);
    return;
  }

  bool forceFlushEvent = false;
  if (hasFreshChunk && incoming.state == StreamState::DISCONTINUOUS) {
    currentTime_.store(incoming.timestamp, std::memory_order_release);
    resetPitchPreservationState();
    clearPendingDecoderChunk();
    forceFlushEvent = true;
  }

  const bool hasDecoderInput = pendingDecoderChunk_.size > 0 || incoming.size > 0;
  const FillMode fillMode = chooseFillMode(preservesPitch, rateAffectsOutput);
  setFillMode(fillMode);

  const bool shouldStretch =
      fillMode == FillMode::Wsola && playbackRateBuffer_ != nullptr && hasDecoderInput;
  const bool shouldResample =
      fillMode == FillMode::Resample && playbackRateBuffer_ != nullptr && hasDecoderInput;

  size_t framesPlayed = std::min(static_cast<size_t>(framesToProcess), incoming.size);
  size_t sourceFramesConsumed = incoming.size;
  if (shouldStretch) {
    sourceFramesConsumed = renderWithWsolaPitchPreservation(
        processingBuffer, incoming, framesToProcess, targetPlaybackRate_);
  } else if (shouldResample) {
    sourceFramesConsumed = renderWithoutPitchPreservation(
        processingBuffer, incoming, framesToProcess, targetPlaybackRate_);
  } else {
    processingBuffer->deinterleaveFrom(incoming.interleavedBuffer.data(), framesPlayed);

    if (volume_ != 1.0f && framesPlayed > 0) {
      processingBuffer->scale(volume_);
    }
  }

  if (forceFlushEvent) {
    positionChanged_.requestFlush();
  }

  // Fill tail end with silence if the chunk returned short
  if (!shouldStretch && !shouldResample && std::cmp_less(framesPlayed, framesToProcess)) {
    processingBuffer->zero(framesPlayed, framesToProcess - framesPlayed);
  }

  applyModeTransitionFade(processingBuffer);
  captureLastOutputFrame(processingBuffer);

  sendOnPositionChangedEvent(static_cast<int>(sourceFramesConsumed));

  if (isStopScheduled()) {
    handleStopScheduled();
  }
}

} // namespace audioapi
