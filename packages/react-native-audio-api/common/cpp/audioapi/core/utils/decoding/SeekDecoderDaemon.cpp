#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <memory>
#include <utility>
namespace audioapi {

SeekDecoderDaemon::SeekDecoderDaemon(
    SeekDecoderDaemonOptions options,
    std::shared_ptr<AudioFileDecoderState> sharedState,
    channels::spsc::Receiver<
        SeekRequest,
        channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT> commandReceiver,
    channels::spsc::Sender<
        DecoderData,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT> frameSender)
    : sharedState_(std::move(sharedState)),
      commandReceiver_(std::move(commandReceiver)),
      frameSender_(std::move(frameSender)) {
  if (options.requiresFFmpeg) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    decoder_ = std::make_unique<ffmpegdecoder::FFmpegDecoder>();
#endif
  } else {
    decoder_ = std::make_unique<miniaudio_decoder::MiniAudioDecoder>();
  }

  decoding::DecoderResult openResult = Ok(None);
  if (!options.filePath.empty()) {
    openResult = decoder_->openFile(options.contextSampleRate, options.filePath);
  } else if (!options.memoryData.empty()) {
    openResult = decoder_->openMemory(
        options.contextSampleRate, options.memoryData.data(), options.memoryData.size());
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
  sharedState_->isReady.store(true, std::memory_order_release);
}

void SeekDecoderDaemon::operator()() {
  const size_t chunkSize = RENDER_QUANTUM_SIZE;
  const size_t chCount = static_cast<size_t>(sharedState_->channelCount);

  DecoderData localData;
  localData.interleavedBuffer.resize(chunkSize * chCount);
  bool hasPendingChunk = false;

  while (sharedState_->isDaemonRunning.load(std::memory_order_acquire)) {
    SeekRequest seekReq;
    bool seekHappened = false;

    while (commandReceiver_.try_receive(seekReq) == ResponseStatus::SUCCESS) {
      if (decoder_ && decoder_->isOpen()) {
        if (decoder_->seekToTime(seekReq.seconds).is_ok()) {
          sharedState_->currentTime.store(seekReq.seconds, std::memory_order_release);
          sharedState_->onPositionChangedFlush.store(true, std::memory_order_release);
        }
      }
      seekHappened = true;
    }

    if (seekHappened) {
      hasPendingChunk = false; // Dump old timeline data
      sharedState_->pendingOffloadedSeeks.fetch_sub(1, std::memory_order_release);
      continue;
    }

    if (!hasPendingChunk) {
      if (!decoder_ || !decoder_->isOpen()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }

      size_t framesRead = decoder_->readPcmFrames(localData.interleavedBuffer.data(), chunkSize);

      if (framesRead == 0) {
        if (sharedState_->loop.load(std::memory_order_acquire)) {
          if (decoder_->seekToTime(0).is_ok()) {
            sharedState_->currentTime.store(0.0, std::memory_order_release);
            sharedState_->onPositionChangedFlush.store(true, std::memory_order_release);
            continue; // Loop back immediately to start
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      localData.size = framesRead;
      hasPendingChunk = true;
    }

    if (frameSender_.try_send(std::move(localData)) == ResponseStatus::SUCCESS) {
      hasPendingChunk = false;
      localData = DecoderData{};
      localData.interleavedBuffer.resize(chunkSize * chCount);
    } else {
      // Audio thread queue is packed. Yield current time slice.
      std::this_thread::yield();
    }
  }

  if (decoder_) {
    decoder_->close();
  }
}

} // namespace audioapi