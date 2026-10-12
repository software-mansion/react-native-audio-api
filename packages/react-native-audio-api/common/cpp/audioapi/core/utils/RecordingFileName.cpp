#include <audioapi/core/utils/RecordingFileName.h>

#include <audioapi/utils/AudioFileProperties.h>

#include <array>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>

namespace audioapi::recording_file_name {

namespace {

constexpr size_t MAX_FILE_NAME_LENGTH = 128;
constexpr size_t SEGMENT_INDEX_WIDTH = 3;
constexpr std::string_view GENERATED_NAME_PREFIX = "recording";

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
  if (!properties->path.fileName.empty()) {
    return properties->path.fileName;
  }

  return std::string(GENERATED_NAME_PREFIX) + "_" + sessionTimestamp();
}

std::string segmentStem(const std::string &sessionStem, size_t segmentIndex) {
  std::string index = std::to_string(segmentIndex);
  if (index.size() < SEGMENT_INDEX_WIDTH) {
    index.insert(0, SEGMENT_INDEX_WIDTH - index.size(), '0');
  }
  return sessionStem + "_" + index;
}

Result<NoneType, std::string> validateFileName(const std::string &fileName) {
  using ValidationResult = Result<NoneType, std::string>;

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

  if (fileName.size() > MAX_FILE_NAME_LENGTH) {
    return ValidationResult::Err(
        "fileName is longer than the " + std::to_string(MAX_FILE_NAME_LENGTH) +
        " characters a file name can spare.");
  }

  return ValidationResult::Ok(None);
}

} // namespace audioapi::recording_file_name
