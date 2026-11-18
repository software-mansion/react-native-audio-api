#include <audioapi/core/inputs/RecorderOutputs.h>

#include <audioapi/core/inputs/RecorderOutput.h>
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

RecorderOutputs::RecorderOutputs(
    std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
    const PlatformFileBackend &fileBackend)
    : errorEvent_(std::move(errorEvent)), fileBackend_(fileBackend) {}

RecorderOutputs::~RecorderOutputs() = default;

void RecorderOutputs::write(const float *const *channels, int numFrames) {
  worker_.push(channels, static_cast<size_t>(numFrames));
}

Result<NoneType, std::string> RecorderOutputs::prepare(const StreamFormat &format) {
  {
    std::scoped_lock lock(outputsMutex_);

    if (fileProperties_ != nullptr) {
      auto writer = std::make_shared<AudioFileWriter>(errorEvent_, fileProperties_, fileBackend_);
      auto fileResult = writer->prepare(format);
      if (fileResult.is_err()) {
        return Err("Failed to open file for writing: " + fileResult.unwrap_err());
      }
      file_ = {.output = std::move(writer), .isPrepared = true};
    }

    for (auto &listed : outputs_) {
      auto outputResult = listed.output->prepare(format);
      if (outputResult.is_err()) {
        return outputResult;
      }
      listed.isPrepared = true;
    }
  }

  return startWorker(format);
}

Result<NoneType, std::string> RecorderOutputs::reprepare(const StreamFormat &format) {
  worker_.stop();

  std::string failures;
  const auto noteFailure = [&failures](const Result<NoneType, std::string> &result) {
    if (result.is_err()) {
      failures += (failures.empty() ? "" : "; ") + result.unwrap_err();
    }
  };

  {
    std::scoped_lock lock(outputsMutex_);

    if (file_.isPrepared) {
      auto fileResult = file_.output->reprepare(format);
      file_.isPrepared = fileResult.is_ok();
      if (fileResult.is_err()) {
        noteFailure(
            Err("Failed to continue the recording in the new input format: " +
                fileResult.unwrap_err()));
      }
    }

    for (auto &listed : outputs_) {
      if (listed.isPrepared) {
        auto outputResult = listed.output->reprepare(format);
        listed.isPrepared = outputResult.is_ok();
        noteFailure(outputResult);
      }
    }
  }

  noteFailure(startWorker(format));

  if (failures.empty()) {
    return Ok(None);
  }
  return Err(failures);
}

RecorderOutputs::Detached RecorderOutputs::detach() {
  worker_.stop();

  std::scoped_lock lock(outputsMutex_);
  for (auto &listed : outputs_) {
    if (listed.isPrepared) {
      listed.output->close();
      listed.isPrepared = false;
    }
  }
  return Detached{.fileWriter = std::exchange(file_, {}).output};
}

Result<FileInfo, std::string> RecorderOutputs::finalize(Detached detached) {
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

Result<NoneType, std::string> RecorderOutputs::enableFile(
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

  std::scoped_lock lock(outputsMutex_);
  fileProperties_ = std::move(properties);
  return Ok(None);
}

RecorderOutputs::Detached RecorderOutputs::disableFile() {
  std::scoped_lock lock(outputsMutex_);
  fileProperties_ = nullptr;
  return Detached{.fileWriter = std::exchange(file_, {}).output};
}

Result<NoneType, std::string> RecorderOutputs::add(
    const std::shared_ptr<RecorderOutput> &output,
    const std::optional<StreamFormat> &currentFormat) {
  if (output == nullptr) {
    return Err("Recorder output is unavailable.");
  }

  std::scoped_lock lock(outputsMutex_);
  const bool isListed = std::ranges::any_of(
      outputs_, [&output](const ListedOutput &listed) { return listed.output == output; });
  if (isListed) {
    return Err("Recorder output is already added.");
  }

  ListedOutput listed{.output = output};
  if (currentFormat.has_value()) {
    auto prepareResult = output->prepare(*currentFormat);
    if (prepareResult.is_err()) {
      return prepareResult;
    }
    listed.isPrepared = true;
  }

  outputs_.push_back(std::move(listed));
  return Ok(None);
}

void RecorderOutputs::remove(const std::shared_ptr<RecorderOutput> &output) {
  bool wasPrepared = false;

  {
    std::scoped_lock lock(outputsMutex_);
    auto listed = std::ranges::find(outputs_, output, &ListedOutput::output);
    if (listed == outputs_.end()) {
      return;
    }
    wasPrepared = listed->isPrepared;
    outputs_.erase(listed);
  }

  // Unlisted, so the worker no longer reaches it.
  if (wasPrepared) {
    output->close();
  }
}

Result<NoneType, std::string> RecorderOutputs::startWorker(const StreamFormat &format) {
  if (!worker_.start(format)) {
    return Err("Failed to preallocate recorder output buffers");
  }
  return Ok(None);
}

void RecorderOutputs::deliver(PendingFrames pending) {
  if (pending.buffer == nullptr) {
    return;
  }

  const auto writeTo = [this, &pending](RecorderOutput &output) {
    auto written = output.write(*pending.buffer, pending.numFrames);
    if (written.is_err() && errorEvent_ != nullptr) {
      errorEvent_->dispatch(StringPayload{.name = "message", .reason = written.unwrap_err()});
    }
  };

  std::scoped_lock lock(outputsMutex_);
  if (file_.isPrepared) {
    writeTo(*file_.output);
  }
  for (const auto &listed : outputs_) {
    if (listed.isPrepared) {
      writeTo(*listed.output);
    }
  }
}

} // namespace audioapi
