#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

namespace audioapi {

using AudioFileConcatResult = Result<std::string, std::string>;

class AudioFileConcatenator {
 public:
  AudioFileConcatenator() = delete;

  [[nodiscard]] static AudioFileConcatResult concatAudioFiles(
      const std::vector<std::string> &inputPaths,
      const std::string &outputPath);

  [[nodiscard]] static std::string normalizeFilePath(const std::string &path);
};

} // namespace audioapi
