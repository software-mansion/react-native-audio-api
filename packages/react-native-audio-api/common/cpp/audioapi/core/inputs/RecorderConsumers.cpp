#include <audioapi/core/inputs/RecorderConsumers.h>

#include <audioapi/core/AudioConsumer.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioFileProperties.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

RecorderConsumers::RecorderConsumers(
    std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
    const PlatformFileBackend &fileBackend)
    : errorEvent_(std::move(errorEvent)), fileBackend_(fileBackend) {}

RecorderConsumers::~RecorderConsumers() = default;

void RecorderConsumers::write(const float *const *channels, int numFrames) {
  worker_.push(channels, static_cast<size_t>(numFrames));
}

Result<NoneType, std::string> RecorderConsumers::prepare(const StreamFormat &format) {
  {
    std::scoped_lock lock(consumersMutex_);

    if (fileProperties_ != nullptr) {
      auto writer = std::make_shared<AudioFileWriter>(errorEvent_, fileProperties_, fileBackend_);
      auto fileResult = writer->prepare(format);
      if (fileResult.is_err()) {
        return Err("Failed to open file for writing: " + fileResult.unwrap_err());
      }
      file_ = {.consumer = std::move(writer), .isPrepared = true};
    }

    for (auto &listed : consumers_) {
      auto consumerResult = listed.consumer->prepare(format);
      if (consumerResult.is_err()) {
        return consumerResult;
      }
      listed.isPrepared = true;
    }
  }

  return startWorker(format);
}

Result<NoneType, std::string> RecorderConsumers::reprepare(const StreamFormat &format) {
  worker_.stop();

  std::string failures;
  const auto noteFailure = [&failures](const Result<NoneType, std::string> &result) {
    if (result.is_err()) {
      failures += (failures.empty() ? "" : "; ") + result.unwrap_err();
    }
  };

  {
    std::scoped_lock lock(consumersMutex_);

    if (file_.isPrepared) {
      auto fileResult = file_.consumer->reprepare(format);
      file_.isPrepared = fileResult.is_ok();
      if (fileResult.is_err()) {
        noteFailure(
            Err("Failed to continue the recording in the new input format: " +
                fileResult.unwrap_err()));
      }
    }

    for (auto &listed : consumers_) {
      if (listed.isPrepared) {
        auto consumerResult = listed.consumer->reprepare(format);
        listed.isPrepared = consumerResult.is_ok();
        noteFailure(consumerResult);
      }
    }
  }

  noteFailure(startWorker(format));

  if (failures.empty()) {
    return Ok(None);
  }
  return Err(failures);
}

RecorderConsumers::Detached RecorderConsumers::detach() {
  worker_.stop();

  std::scoped_lock lock(consumersMutex_);
  for (auto &listed : consumers_) {
    if (listed.isPrepared) {
      listed.consumer->close();
      listed.isPrepared = false;
    }
  }
  return Detached{.fileWriter = std::exchange(file_, {}).consumer};
}

Result<FileInfo, std::string> RecorderConsumers::finalize(Detached detached) {
  if (detached.fileWriter == nullptr) {
    return Ok(FileInfo{});
  }

  auto fileResult = detached.fileWriter->closeFile();
  if (fileResult.is_err()) {
    return Err("Failed to close file: " + fileResult.unwrap_err());
  }

  const auto &session = fileResult.unwrap();
  std::vector<std::string> fileUris;
  fileUris.reserve(session.filePaths.size());
  for (const auto &filePath : session.filePaths) {
    fileUris.push_back("file://" + filePath);
  }

  return Ok(
      FileInfo{
          .paths = std::move(fileUris),
          .size = session.sizeMB,
          .duration = session.durationSec,
      });
}

Result<NoneType, std::string> RecorderConsumers::enableFile(
    std::shared_ptr<AudioFileProperties> properties) {
  if (properties == nullptr) {
    return Err("File output requires file properties");
  }
  auto validationResult = properties->validate();
  if (validationResult.is_err()) {
    return validationResult;
  }
  auto outputSpecResult = fileBackend_.resolveOutputSpec(properties->encoding.format);
  if (outputSpecResult.is_err()) {
    return Err(outputSpecResult.unwrap_err());
  }

  std::scoped_lock lock(consumersMutex_);
  fileProperties_ = std::move(properties);
  return Ok(None);
}

RecorderConsumers::Detached RecorderConsumers::disableFile() {
  std::scoped_lock lock(consumersMutex_);
  fileProperties_ = nullptr;
  return Detached{.fileWriter = std::exchange(file_, {}).consumer};
}

Result<NoneType, std::string> RecorderConsumers::add(
    const std::shared_ptr<AudioConsumer> &consumer,
    const std::optional<StreamFormat> &currentFormat) {
  if (consumer == nullptr) {
    return Err("Recorder consumer is unavailable.");
  }

  std::scoped_lock lock(consumersMutex_);
  const bool isListed = std::ranges::any_of(consumers_, [&consumer](const ListedConsumer &listed) {
    return listed.consumer == consumer;
  });
  if (isListed) {
    return Err("Recorder consumer is already added.");
  }

  ListedConsumer listed{.consumer = consumer};
  if (currentFormat.has_value()) {
    auto prepareResult = consumer->prepare(*currentFormat);
    if (prepareResult.is_err()) {
      return prepareResult;
    }
    listed.isPrepared = true;
  }

  consumers_.push_back(std::move(listed));
  return Ok(None);
}

void RecorderConsumers::remove(const std::shared_ptr<AudioConsumer> &consumer) {
  bool wasPrepared = false;

  {
    std::scoped_lock lock(consumersMutex_);
    auto listed = std::ranges::find(consumers_, consumer, &ListedConsumer::consumer);
    if (listed == consumers_.end()) {
      return;
    }
    wasPrepared = listed->isPrepared;
    consumers_.erase(listed);
  }

  // Unlisted, so the worker no longer reaches it.
  if (wasPrepared) {
    consumer->close();
  }
}

Result<NoneType, std::string> RecorderConsumers::startWorker(const StreamFormat &format) {
  if (!worker_.start(format)) {
    return Err("Failed to preallocate recorder consumer buffers");
  }
  return Ok(None);
}

void RecorderConsumers::deliver(PendingFrames pending) {
  if (pending.buffer == nullptr) {
    return;
  }

  const auto feed = [this, &pending](AudioConsumer &consumer) {
    auto consumed = consumer.consume(*pending.buffer, pending.numFrames);
    if (consumed.is_err() && errorEvent_ != nullptr) {
      errorEvent_->dispatch(StringPayload{.name = "message", .reason = consumed.unwrap_err()});
    }
  };

  std::scoped_lock lock(consumersMutex_);
  if (file_.isPrepared) {
    feed(*file_.consumer);
  }
  for (const auto &listed : consumers_) {
    if (listed.isPrepared) {
      feed(*listed.consumer);
    }
  }
}

} // namespace audioapi
