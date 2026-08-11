#include <audioapi/android/AndroidRemux.h>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaMuxer.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::android_remux {
namespace {

constexpr size_t kSampleBufferBytes = 256 * 1024;

struct TrackInfo {
  std::string mime;
  int32_t sampleRate{0};
  int32_t channelCount{0};
  std::vector<uint8_t> csd0;
};

class ExtractorGuard {
 public:
  ExtractorGuard() = default;
  ExtractorGuard(const ExtractorGuard &) = delete;
  ExtractorGuard &operator=(const ExtractorGuard &) = delete;

  ~ExtractorGuard() {
    reset();
  }

  void reset() {
    if (extractor_ != nullptr) {
      AMediaExtractor_delete(extractor_);
      extractor_ = nullptr;
    }
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  [[nodiscard]] AMediaExtractor *get() const {
    return extractor_;
  }

  [[nodiscard]] AndroidRemuxResult open(const std::string &path) {
    reset();

    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
      return Err("Failed to open input file '" + path + "': " + std::strerror(errno));
    }

    struct stat st{};
    if (::fstat(fd_, &st) != 0) {
      reset();
      return Err("Failed to stat input file '" + path + "': " + std::strerror(errno));
    }

    extractor_ = AMediaExtractor_new();
    if (extractor_ == nullptr) {
      reset();
      return Err("Failed to create MediaExtractor for '" + path + "'.");
    }

    const media_status_t status =
        AMediaExtractor_setDataSourceFd(extractor_, fd_, 0, static_cast<off64_t>(st.st_size));
    if (status != AMEDIA_OK) {
      reset();
      return Err("Failed to set MediaExtractor data source for '" + path + "'.");
    }

    return Ok(path);
  }

 private:
  AMediaExtractor *extractor_{nullptr};
  int fd_{-1};
};

class MuxerGuard {
 public:
  MuxerGuard() = default;
  MuxerGuard(const MuxerGuard &) = delete;
  MuxerGuard &operator=(const MuxerGuard &) = delete;

  ~MuxerGuard() {
    reset();
  }

  void reset() {
    if (muxer_ != nullptr) {
      if (started_) {
        AMediaMuxer_stop(muxer_);
        started_ = false;
      }
      AMediaMuxer_delete(muxer_);
      muxer_ = nullptr;
    }
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  [[nodiscard]] AMediaMuxer *get() const {
    return muxer_;
  }

  [[nodiscard]] ssize_t trackIndex() const {
    return trackIndex_;
  }

  [[nodiscard]] AndroidRemuxResult open(const std::string &path) {
    reset();

    fd_ = ::open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd_ < 0) {
      return Err("Failed to open output file '" + path + "': " + std::strerror(errno));
    }

    muxer_ = AMediaMuxer_new(fd_, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    if (muxer_ == nullptr) {
      reset();
      return Err("Failed to create MediaMuxer for '" + path + "'.");
    }

    return Ok(path);
  }

  [[nodiscard]] AndroidRemuxResult addTrackAndStart(AMediaFormat *format) {
    trackIndex_ = AMediaMuxer_addTrack(muxer_, format);
    if (trackIndex_ < 0) {
      return Err("Failed to add audio track to MediaMuxer.");
    }
    if (AMediaMuxer_start(muxer_) != AMEDIA_OK) {
      return Err("Failed to start MediaMuxer.");
    }
    started_ = true;
    return Ok(std::string());
  }

 private:
  AMediaMuxer *muxer_{nullptr};
  int fd_{-1};
  ssize_t trackIndex_{-1};
  bool started_{false};
};

[[nodiscard]] AndroidRemuxResult
findAudioTrack(AMediaExtractor *extractor, size_t &trackIndex, TrackInfo &info) {
  const size_t trackCount = AMediaExtractor_getTrackCount(extractor);
  for (size_t i = 0; i < trackCount; ++i) {
    AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, i);
    if (format == nullptr) {
      continue;
    }

    const char *mime = nullptr;
    if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime) || mime == nullptr) {
      AMediaFormat_delete(format);
      continue;
    }

    if (std::strncmp(mime, "audio/", 6) != 0) {
      AMediaFormat_delete(format);
      continue;
    }

    TrackInfo candidate;
    candidate.mime = mime;
    if (!AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &candidate.sampleRate) ||
        candidate.sampleRate <= 0) {
      AMediaFormat_delete(format);
      return Err("Input audio track is missing a valid sample rate.");
    }
    if (!AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &candidate.channelCount) ||
        candidate.channelCount <= 0) {
      AMediaFormat_delete(format);
      return Err("Input audio track is missing a valid channel count.");
    }

    void *csd = nullptr;
    size_t csdSize = 0;
    if (AMediaFormat_getBuffer(format, "csd-0", &csd, &csdSize) && csd != nullptr && csdSize > 0) {
      const auto *bytes = static_cast<const uint8_t *>(csd);
      candidate.csd0.assign(bytes, bytes + csdSize);
    }

    AMediaFormat_delete(format);
    trackIndex = i;
    info = std::move(candidate);
    return Ok(std::string());
  }

  return Err("Input file does not contain an audio stream.");
}

// Opens an extractor for `path` and locates its audio track, prefixing any
// track error with the file path for consistent messages.
[[nodiscard]] AndroidRemuxResult openAndFindAudioTrack(
    const std::string &path,
    ExtractorGuard &extractor,
    size_t &trackIndex,
    TrackInfo &info) {
  auto openResult = extractor.open(path);
  if (openResult.is_err()) {
    return openResult;
  }

  auto found = findAudioTrack(extractor.get(), trackIndex, info);
  if (found.is_err()) {
    return Err("Input file '" + path + "': " + found.unwrap_err());
  }

  return Ok(path);
}

[[nodiscard]] AndroidRemuxResult validateCompatible(
    const TrackInfo &candidate,
    const TrackInfo &reference,
    const std::string &filePath) {
  if (candidate.mime != reference.mime) {
    return Err("Input file '" + filePath + "' uses a different audio codec.");
  }
  if (candidate.sampleRate != reference.sampleRate) {
    return Err("Input file '" + filePath + "' uses a different sample rate.");
  }
  if (candidate.channelCount != reference.channelCount) {
    return Err("Input file '" + filePath + "' uses a different channel layout.");
  }
  return Ok(filePath);
}

[[nodiscard]] AMediaFormat *buildOutputFormat(const TrackInfo &info) {
  AMediaFormat *format = AMediaFormat_new();
  AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, info.mime.c_str());
  AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, info.sampleRate);
  AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, info.channelCount);
  if (!info.csd0.empty()) {
    AMediaFormat_setBuffer(format, "csd-0", info.csd0.data(), info.csd0.size());
  }
  return format;
}

[[nodiscard]] AndroidRemuxResult appendSamples(
    AMediaExtractor *extractor,
    size_t extractorTrackIndex,
    AMediaMuxer *muxer,
    size_t muxerTrackIndex,
    int64_t &timeOffsetUs) {
  if (AMediaExtractor_selectTrack(extractor, extractorTrackIndex) != AMEDIA_OK) {
    return Err("Failed to select audio track.");
  }

  std::vector<uint8_t> buffer(kSampleBufferBytes);
  int64_t baseTimeUs = -1;
  int64_t segmentEndUs = timeOffsetUs;

  while (true) {
    const ssize_t sampleSize =
        AMediaExtractor_readSampleData(extractor, buffer.data(), buffer.size());
    if (sampleSize < 0) {
      break;
    }

    const int64_t sampleTimeUs = AMediaExtractor_getSampleTime(extractor);
    const uint32_t flags = AMediaExtractor_getSampleFlags(extractor);

    if ((flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0) {
      if (!AMediaExtractor_advance(extractor)) {
        break;
      }
      continue;
    }

    if (baseTimeUs < 0) {
      baseTimeUs = sampleTimeUs >= 0 ? sampleTimeUs : 0;
    }

    const int64_t presentationTimeUs =
        (sampleTimeUs >= 0 ? sampleTimeUs - baseTimeUs : 0) + timeOffsetUs;

    AMediaCodecBufferInfo writeInfo{};
    writeInfo.offset = 0;
    writeInfo.size = static_cast<int32_t>(sampleSize);
    writeInfo.presentationTimeUs = presentationTimeUs;
    writeInfo.flags = static_cast<uint32_t>(flags);

    if (AMediaMuxer_writeSampleData(muxer, muxerTrackIndex, buffer.data(), &writeInfo) !=
        AMEDIA_OK) {
      return Err("Failed to write sample data to MediaMuxer.");
    }

    if (presentationTimeUs >= segmentEndUs) {
      segmentEndUs = presentationTimeUs + 1;
    }

    if (!AMediaExtractor_advance(extractor)) {
      break;
    }
  }

  timeOffsetUs = segmentEndUs;
  return Ok(std::string());
}

} // namespace

AndroidRemuxResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  if (inputPaths.empty()) {
    return Err("concatAudioFiles requires at least one input path.");
  }

  ExtractorGuard firstExtractor;
  size_t firstTrackIndex = 0;
  TrackInfo referenceInfo;
  auto refResult =
      openAndFindAudioTrack(inputPaths.front(), firstExtractor, firstTrackIndex, referenceInfo);
  if (refResult.is_err()) {
    return refResult;
  }

  if (referenceInfo.mime != "audio/mp4a-latm") {
    return Err(
        "Input file '" + inputPaths.front() +
        "' is not AAC-in-M4A/MP4; only AAC remux is supported.");
  }

  for (size_t i = 1; i < inputPaths.size(); ++i) {
    ExtractorGuard extractor;
    size_t trackIndex = 0;
    TrackInfo info;
    auto result = openAndFindAudioTrack(inputPaths[i], extractor, trackIndex, info);
    if (result.is_err()) {
      return result;
    }

    auto validation = validateCompatible(info, referenceInfo, inputPaths[i]);
    if (validation.is_err()) {
      return validation;
    }
  }

  MuxerGuard muxer;
  auto muxerOpen = muxer.open(outputPath);
  if (muxerOpen.is_err()) {
    return muxerOpen;
  }

  AMediaFormat *outputFormat = buildOutputFormat(referenceInfo);
  auto startResult = muxer.addTrackAndStart(outputFormat);
  AMediaFormat_delete(outputFormat);
  if (startResult.is_err()) {
    return startResult;
  }

  int64_t timeOffsetUs = 0;
  const size_t muxerTrackIndex = static_cast<size_t>(muxer.trackIndex());

  firstExtractor.reset();
  for (const auto &inputPath : inputPaths) {
    ExtractorGuard extractor;
    size_t extractorTrackIndex = 0;
    TrackInfo info;
    auto result = openAndFindAudioTrack(inputPath, extractor, extractorTrackIndex, info);
    if (result.is_err()) {
      return result;
    }

    auto appendResult = appendSamples(
        extractor.get(), extractorTrackIndex, muxer.get(), muxerTrackIndex, timeOffsetUs);
    if (appendResult.is_err()) {
      return Err("Failed while remuxing '" + inputPath + "': " + appendResult.unwrap_err());
    }
  }

  muxer.reset();
  return Ok(outputPath);
}

} // namespace audioapi::android_remux
