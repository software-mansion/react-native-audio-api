#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/EncoderFactory.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/PlatformFileBackend.h>
#include <audioapi/utils/FileSystem.hpp>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace audioapi {

const PlatformFileBackend &osFileBackend() {
  static const PlatformFileBackend backend{
      .resolveOutputSpec = &encoder_capabilities::resolveOutputSpec,
      .resolvePath = &resolveOsFilePath,
      .createEncoder = &encoding::createEncoder,
  };
  return backend;
}

namespace {

void warnAboutOverwrite(const std::string &path) {
#ifdef ANDROID
  __android_log_print(
      ANDROID_LOG_WARN, "RN_AUDIOAPI", "recording overwrites an existing file: %s", path.c_str());
#else
  printf("[RN_AUDIOAPI WARN] recording overwrites an existing file: %s\n", path.c_str());
#endif
}

Result<std::string, std::string> resolveFilePath(
    const PlatformFileBackend &backend,
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &stem,
    const std::string &extension) {
  auto path = backend.resolvePath(properties, stem + "." + extension);
  if (path.is_err()) {
    return path;
  }

  if (!properties->path.fileName.empty()) {
    if (file_system::fileExists(path.unwrap())) {
      warnAboutOverwrite(path.unwrap());
    }
    return path;
  }

  for (size_t suffix = 1; path.is_ok() && file_system::fileExists(path.unwrap()); ++suffix) {
    path = backend.resolvePath(properties, stem + "_" + std::to_string(suffix) + "." + extension);
  }
  return path;
}

} // namespace

CreateEncoderResult openFileEncoder(
    const PlatformFileBackend &backend,
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &stem,
    const StreamFormat &inputFormat) {
  // Calling an empty std::function throws, which on the worker thread would terminate.
  if (!backend.resolveOutputSpec || !backend.resolvePath || !backend.createEncoder) {
    return Err("File output has no platform backend");
  }

  auto specResult = backend.resolveOutputSpec(properties->encoding.format);
  if (specResult.is_err()) {
    return Err(std::move(specResult).unwrap_err());
  }
  const auto &outputSpec = specResult.unwrap();

  auto encoderResult = backend.createEncoder(
      EncoderSettings{.fileLayout = properties->fileLayout, .encoding = properties->encoding});
  if (encoderResult.is_err()) {
    return encoderResult;
  }
  auto encoder = std::move(encoderResult).unwrap();

  // Picking a free name and creating its file is one step: two opens in the same second would
  // otherwise both find the generated name free and write the same file.
  static std::mutex nameSelectionMutex;
  std::scoped_lock nameSelectionLock(nameSelectionMutex);

  auto pathResult = resolveFilePath(backend, properties, stem, std::string(outputSpec.extension));
  if (pathResult.is_err()) {
    return Err(std::move(pathResult).unwrap_err());
  }

  auto openResult = encoder->open(inputFormat, outputSpec, pathResult.unwrap());
  if (openResult.is_err()) {
    return Err(std::move(openResult).unwrap_err());
  }
  return Ok(std::move(encoder));
}

} // namespace audioapi
