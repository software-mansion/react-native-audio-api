#include <audioapi/core/inputs/FileInfo.h>

#include <jsi/jsi.h>
#include <utility>

namespace audioapi {

namespace jsi = facebook::jsi;

namespace {

/// The JS `AudioFileStats` part both file infos share.
jsi::Object createFileStats(jsi::Runtime &runtime, double size, double duration) {
  auto stats = jsi::Object(runtime);
  stats.setProperty(runtime, "size", size);
  stats.setProperty(runtime, "duration", duration);
  return stats;
}

} // namespace

jsi::Value FileInfo::toJSIValue(jsi::Runtime &runtime) const {
  auto pathsArray = jsi::Array(runtime, paths.size());
  for (size_t i = 0; i < paths.size(); ++i) {
    pathsArray.setValueAtIndex(runtime, i, jsi::String::createFromUtf8(runtime, paths[i]));
  }

  auto fileInfo = createFileStats(runtime, size, duration);
  fileInfo.setProperty(runtime, "paths", pathsArray);
  return {std::move(fileInfo)};
}

jsi::Value EncodedFileInfo::toJSIValue(jsi::Runtime &runtime) const {
  auto fileInfo = createFileStats(runtime, size, duration);
  fileInfo.setProperty(runtime, "path", jsi::String::createFromUtf8(runtime, path));
  return {std::move(fileInfo)};
}

} // namespace audioapi
