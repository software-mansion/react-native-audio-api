#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <memory>
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

  decoding::DecoderResult openResult = Ok(None);

  int contextSampleRate = static_cast<int>(options.contextSampleRate);
  if (!options.filePath.empty()) {
    openResult = decoder_->openFile(contextSampleRate, options.filePath);
  } else if (!options.memoryData.empty()) {
    openResult = decoder_->openMemory(
        contextSampleRate, options.memoryData.data(), options.memoryData.size());
  }

  if (!openResult.is_ok()) {
    decoder_->close();
    sharedState_->isDaemonRunning.store(false, std::memory_order_release);
    return;
  }

  sharedState_->channelCount.store(decoder_->outputChannels(), std::memory_order_release);
  sharedState_->sampleRate.store(
      static_cast<float>(decoder_->outputSampleRate()), std::memory_order_release);
  sharedState_->duration.store(decoder_->getDurationInSeconds(), std::memory_order_release);
  sharedState_->loop.store(options.loop, std::memory_order_release);
  sharedState_->isReady.store(true, std::memory_order_release);
}

bool SeekDecoderDaemon::processSeekCommands() {
  SeekRequest seekReq;
  bool seekHappened = false;
  while (commandReceiver_.try_receive(seekReq) == ResponseStatus::SUCCESS) {
    if (decoder_ && decoder_->isOpen() && decoder_->seekToTime(seekReq.seconds).is_ok()) {
      sharedState_->currentTime.store(seekReq.seconds, std::memory_order_release);
      sharedState_->onPositionChangedFlush.store(true, std::memory_order_release);
      sharedState_->pendingOffloadedSeeks.fetch_sub(1, std::memory_order_release);
    }
    seekHappened = true;
  }
  if (seekHappened) {
    // Drain stale pre-seek frames before releasing the seek gate on the audio thread
    DecoderData drop;
    while (frameReceiverForDrain_->try_receive(drop) == ResponseStatus::SUCCESS) {}
  }
  return seekHappened;
}

bool SeekDecoderDaemon::decodeNextChunk(DecoderData &data) {
  if (!decoder_ || !decoder_->isOpen()) {
    return false;
  }

  size_t framesRead = decoder_->readPcmFrames(data.interleavedBuffer.data(), RENDER_QUANTUM_SIZE);

  if (framesRead == 0) {
    if (sharedState_->loop.load(std::memory_order_acquire) && decoder_->seekToTime(0).is_ok()) {
      sharedState_->currentTime.store(0.0, std::memory_order_release);
      sharedState_->onPositionChangedFlush.store(true, std::memory_order_release);
    } else {
      sharedState_->isEof.store(true, std::memory_order_release);
    }
    return false;
  }

  sharedState_->isEof.store(false, std::memory_order_release);
  data.size = framesRead;
  return true;
}

void SeekDecoderDaemon::operator()() {
  DecoderData localData;
  bool hasPendingChunk = false;

  while (sharedState_->isDaemonRunning.load(std::memory_order_acquire)) {
    if (processSeekCommands()) {
      hasPendingChunk = false;
      continue;
    }

    if (!hasPendingChunk) {
      hasPendingChunk = decodeNextChunk(localData);
    }

    if (!hasPendingChunk) {
      continue;
    }

    if (frameSender_.try_send(localData) == ResponseStatus::SUCCESS) {
      hasPendingChunk = false;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_ON_FULL));
    }
  }

  if (decoder_) {
    decoder_->close();
  }
}

} // namespace audioapi