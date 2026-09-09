#include <audioapi/core/utils/RecordingFileName.h>

#include <audioapi/utils/AudioFileProperties.h>

#include <array>
#include <chrono>
#include <ctime>
#include <format>
#include <memory>
#include <string>

namespace audioapi::recordingfilename {

namespace {

constexpr size_t kMaxFileNameLength = 128;
constexpr std::string_view kGeneratedNamePrefix = "recording";

bool containsPathSeparator(const std::string &name) {
  return name.find('/') != std::string::npos || name.find('\\') != std::string::npos;
}

} // namespace

std::string sessionTimestamp() {
  const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::tm localTime{};
  localtime_r(&now, &localTime);

  std::array<char, 32> timestamp{};
  std::strftime(timestamp.data(), timestamp.size(), "%Y%m%d_%H%M%S", &localTime);
  return {timestamp.data()};
}

std::string sessionStem(const std::shared_ptr<AudioFileProperties> &properties) {
  if (!properties->fileName.empty()) {
    return properties->fileName;
  }

  return std::format("{}_{}", kGeneratedNamePrefix, sessionTimestamp());
}

std::string segmentStem(const std::string &sessionStem, size_t segmentIndex) {
  return std::format("{}_{:03}", sessionStem, segmentIndex);
}

Result<NoneType, std::string> validate(const std::shared_ptr<AudioFileProperties> &properties) {
  using ValidationResult = Result<NoneType, std::string>;

  const std::string &fileName = properties->fileName;
  if (fileName.empty()) {
    return ValidationResult::Ok(None);
  }

  if (containsPathSeparator(fileName) || fileName.find("..") != std::string::npos) {
    return ValidationResult::Err("fileName must be a file name, without path separators or '..'.");
  }

  if (fileName.find('.') != std::string::npos) {
    return ValidationResult::Err(
        "fileName must not carry an extension — it follows from the chosen format.");
  }

  if (fileName.size() > kMaxFileNameLength) {
    return ValidationResult::Err(
        std::format(
            "fileName is longer than the {} characters a file name can spare.",
            kMaxFileNameLength));
  }

  return ValidationResult::Ok(None);
}

} // namespace audioapi::recordingfilename
