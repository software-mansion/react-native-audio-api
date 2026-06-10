#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <memory>
#include <thread>
#include <utility>

namespace audioapi {

SeekDecoderDaemon::SeekDecoderDaemon(
    SeekDecoderDaemonOptions options,
    std::shared_ptr<AudioFileDecoderState> sharedState,
    CommandReceiver commandReceiver,
    FrameSender frameSender,
    std::shared_ptr<FrameReceiver> frameReceiver)
    : sharedState_(std::move(sharedState)),
      commandReceiver_(std::move(commandReceiver)),
      frameSender_(std::move(frameSender)),
      frameReceiverForDrain_(std::move(frameReceiver)) {
  if (options.requiresFFmpeg) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    decoder_ = std::make_unique<ffmpeg_decoder::FFmpegDecoder>();
#endif
  } else {
    decoder_ = std::make_unique<miniaudio_decoder::MiniAudioDecoder>();
  }

  decoding::DecoderResult openResult = Err("Failed to initialize decoder");

  int contextSampleRate = static_cast<int>(options.contextSampleRate);

  // Check in AudioFileSourceNode constructor ensures that at least one of filePath or memoryData is provided,
  // so we can attempt to open the decoder with either without additional checks here.
  if (!options.filePath.empty()) {
    openResult = decoder_->openFile(contextSampleRate, options.filePath);
  } else {
    openResult = decoder_->openMemory(
        contextSampleRate, options.memoryData.data(), options.memoryData.size());
  }

  if (openResult.is_err()) {
    decoder_->close();
    sharedState_->isDaemonRunning.store(false, std::memory_order_release);
    return;
  }

  sharedState_->channelCount.store(decoder_->outputChannels(), std::memory_order_release);
  sharedState_->sampleRate.store(
      static_cast<float>(decoder_->outputSampleRate()), std::memory_order_release);
  sharedState_->duration.store(decoder_->getDurationInSeconds(), std::memory_order_release);
  sharedState_->loop.store(options.loop, std::memory_order_release);
  sharedState_->isHlsStreaming.store(decoder_->isHlsStreaming(), std::memory_order_release);
  sharedState_->isReady.store(true, std::memory_order_release);
}

std::optional<SeekRequest> SeekDecoderDaemon::processSeekCommands() {
  SeekRequest seekReq;
  SeekRequest latestReq;
  bool seekHappened = false;
  int drainedCount = 0;

  // Fast-drain the command SPSC pipe to grab ONLY the final target position
  while (commandReceiver_.try_receive(seekReq) == ResponseStatus::SUCCESS) {
    latestReq = seekReq;
    seekHappened = true;
    drainedCount++;
  }

  if (!seekHappened) {
    return std::nullopt;
  }

  bool seekSucceeded =
      decoder_ && decoder_->isOpen() && decoder_->seekToTime(latestReq.seconds).is_ok();

  // Always release the gate — we consumed the commands regardless of seek outcome
  sharedState_->pendingOffloadedSeeks.fetch_sub(drainedCount, std::memory_order_release);

  if (!seekSucceeded) {
    return std::nullopt;
  }

  // Drain stale pre-seek frames out of the pipe so old audio doesn't play
  DecoderData drop;
  while (frameReceiverForDrain_->try_receive(drop) == ResponseStatus::SUCCESS) {}

  return latestReq;
}

bool SeekDecoderDaemon::decodeNextChunk(
    DecoderData &data,
    const std::optional<SeekRequest> &seekRequest) {
  if (!decoder_ || !decoder_->isOpen()) {
    return false;
  }

  size_t framesRead = decoder_->readPcmFrames(data.interleavedBuffer.data(), RENDER_QUANTUM_SIZE);

  if (framesRead > 0) {
    data.size = framesRead;
    // After seeks mark the chunk as discontinuous with the target seek time
    if (seekRequest.has_value()) {
      data.state = StreamState::DISCONTINUOUS;
      data.timestamp = seekRequest->seconds;
    } else {
      data.state = StreamState::PLAYING;
      data.timestamp = decoder_->getCurrentPositionInSeconds();
    }
    return true;
  }

  // HLS live streams have no true EOF — wait for the next segment instead of stopping.
  if (sharedState_->isHlsStreaming.load(std::memory_order_acquire)) {
    return false;
  }

  // If loop is enabled and we hit EOF, attempt to seek back to the start and read again
  if (sharedState_->loop.load(std::memory_order_acquire) && decoder_->seekToTime(0).is_ok()) {
    data.state = StreamState::DISCONTINUOUS;
    data.timestamp = 0.0;
    framesRead = decoder_->readPcmFrames(data.interleavedBuffer.data(), RENDER_QUANTUM_SIZE);
    data.size = framesRead;
    return true;
  }

  // EOF reached without loop - end of stream
  data.size = 0;
  data.state = StreamState::END_OF_STREAM;
  return true;
}

void SeekDecoderDaemon::operator()() {
  DecoderData localData;
  bool hasPendingChunk = false;

  while (sharedState_->isDaemonRunning.load(std::memory_order_acquire)) {

    auto seekResult = processSeekCommands();
    if (seekResult.has_value()) {
      hasPendingChunk = false;
    }

    if (!hasPendingChunk) {
      hasPendingChunk = decodeNextChunk(localData, seekResult);
    }

    if (!hasPendingChunk) {
      if (sharedState_->isHlsStreaming.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(SLEEP_DURATION_ON_FULL);
      }
      continue;
    }

    if (frameSender_.try_send(localData) == ResponseStatus::SUCCESS) {
      hasPendingChunk = false;
    } else {
      std::this_thread::sleep_for(SLEEP_DURATION_ON_FULL);
    }
  }

  if (decoder_) {
    decoder_->close();
  }
}

} // namespace audioapi