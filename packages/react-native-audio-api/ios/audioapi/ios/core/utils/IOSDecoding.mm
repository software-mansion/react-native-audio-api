#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/IOSDecoding.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace audioapi::ios_decoder {
namespace {

template <auto DisposeFn>
struct FnDeleter {
  void operator()(auto *file) const
  {
    if (file != nullptr) {
      DisposeFn(file);
    }
  }
};

using ExtAudioFilePtr =
    std::unique_ptr<std::remove_pointer_t<ExtAudioFileRef>, FnDeleter<ExtAudioFileDispose>>;
using AudioFilePtr = std::unique_ptr<std::remove_pointer_t<AudioFileID>, FnDeleter<AudioFileClose>>;

} // namespace

// Native reader state kept out of the shared header so Core Audio types never
// leak into cross-platform code. Owned by IOSDecoder::impl_.
struct IosDecoderState {
  // Declaration order matters (members destroy in reverse order): ExtAudioFile
  // wraps AudioFile for openMemory(), so extFile must be destroyed first, and
  // both may read through memoryReadProc while closing — so the buffer that
  // callback serves has to outlive them and must be declared first.
  std::vector<uint8_t> memory;
  AudioFilePtr audioFile;
  ExtAudioFilePtr extFile;
  // File-native sample rate, used to translate seek time into file frames
  // (ExtAudioFileSeek positions in the file's rate, not the client rate).
  double fileSampleRate = 0.0;
};

namespace {

// AudioFile read callback over the in-memory buffer owned by IosDecoderState.
OSStatus memoryReadProc(
    void *inClientData,
    SInt64 inPosition,
    UInt32 requestCount,
    void *buffer,
    UInt32 *actualCount)
{
  auto *state = static_cast<IosDecoderState *>(inClientData);
  const auto total = static_cast<SInt64>(state->memory.size());
  if (inPosition < 0 || inPosition > total) {
    *actualCount = 0;
    return kAudioFileInvalidPacketOffsetError;
  }
  const SInt64 available = total - inPosition;
  const auto toCopy =
      static_cast<UInt32>(std::min<SInt64>(static_cast<SInt64>(requestCount), available));
  if (toCopy > 0) {
    std::memcpy(buffer, state->memory.data() + inPosition, toCopy);
  }
  *actualCount = toCopy;
  return noErr;
}

SInt64 memoryGetSizeProc(void *inClientData)
{
  auto *state = static_cast<IosDecoderState *>(inClientData);
  return static_cast<SInt64>(state->memory.size());
}

// Sniffs a container hint from the leading magic bytes so AudioFile can pick the
// right parser for in-memory data. Returns 0 when the container is unrecognized.
// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
AudioFileTypeID guessFileTypeHint(const void *data, size_t size)
{
  if (data == nullptr || size < 12) {
    return 0;
  }
  const auto *bytes = static_cast<const unsigned char *>(data);
  if (std::memcmp(bytes, "RIFF", 4) == 0 && std::memcmp(bytes + 8, "WAVE", 4) == 0) {
    return kAudioFileWAVEType;
  }
  if (std::memcmp(bytes, "fLaC", 4) == 0) {
    return kAudioFileFLACType;
  }
  if (bytes[0] == 0xFF && (bytes[1] & 0xF6) == 0xF0) {
    return kAudioFileAAC_ADTSType;
  }
  if (std::memcmp(bytes, "ID3", 3) == 0 || (bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0)) {
    return kAudioFileMP3Type;
  }
  if (std::memcmp(bytes + 4, "ftyp", 4) == 0) {
    return kAudioFileM4AType;
  }
  if (std::memcmp(bytes, "caff", 4) == 0) {
    return kAudioFileCAFType;
  }
  if (std::memcmp(bytes, "FORM", 4) == 0 &&
      (std::memcmp(bytes + 8, "AIFF", 4) == 0 || std::memcmp(bytes + 8, "AIFC", 4) == 0)) {
    return kAudioFileAIFFType;
  }
  return 0;
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

} // namespace

IOSDecoder::IOSDecoder() = default;

IOSDecoder::~IOSDecoder()
{
  close();
}

decoding::DecoderResult IOSDecoder::finishOpen(
    std::unique_ptr<IosDecoderState> state,
    int requestedSampleRate)
{
  ExtAudioFileRef extFile = state->extFile.get();

  AudioStreamBasicDescription fileFormat{};
  UInt32 propSize = sizeof(fileFormat);
  OSStatus status = ExtAudioFileGetProperty(
      extFile, kExtAudioFileProperty_FileDataFormat, &propSize, &fileFormat);
  if (status != noErr) {
    return Err(
        "IOSDecoder: ExtAudioFileGetProperty(FileDataFormat) failed: " + std::to_string(status));
  }

  const int channels = static_cast<int>(fileFormat.mChannelsPerFrame);
  if (channels <= 0) {
    return Err("IOSDecoder: invalid channel count");
  }

  const int outRate =
      requestedSampleRate > 0 ? requestedSampleRate : static_cast<int>(fileFormat.mSampleRate);

  AudioStreamBasicDescription clientFormat{};
  clientFormat.mSampleRate = outRate;
  clientFormat.mFormatID = kAudioFormatLinearPCM;
  clientFormat.mFormatFlags =
      kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
  clientFormat.mBitsPerChannel = sizeof(float) * CHAR_BIT;
  clientFormat.mChannelsPerFrame = static_cast<UInt32>(channels);
  clientFormat.mFramesPerPacket = 1;
  clientFormat.mBytesPerFrame = sizeof(float) * static_cast<UInt32>(channels);
  clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame;

  status = ExtAudioFileSetProperty(
      extFile, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
  if (status != noErr) {
    return Err(
        "IOSDecoder: ExtAudioFileSetProperty(ClientDataFormat) failed: " + std::to_string(status));
  }

  SInt64 fileLengthFrames = 0;
  propSize = sizeof(fileLengthFrames);
  status = ExtAudioFileGetProperty(
      extFile, kExtAudioFileProperty_FileLengthFrames, &propSize, &fileLengthFrames);
  if (status != noErr || fileLengthFrames < 0) {
    fileLengthFrames = 0; // Unknown (e.g. raw ADTS AAC) — report duration 0.
  }

  const double durationSeconds = fileFormat.mSampleRate > 0
      ? static_cast<double>(fileLengthFrames) / static_cast<double>(fileFormat.mSampleRate)
      : 0.0;

  state->fileSampleRate = fileFormat.mSampleRate;
  outputChannels_ = channels;
  outputSampleRate_ = outRate;
  framePosition_ = 0;
  setTotalPcmFramesFromDuration(durationSeconds);
  impl_ = std::move(state);
  open_ = true;
  return Ok(None);
}

decoding::DecoderResult IOSDecoder::open(const decoding::LocalFileSource &source)
{
  close();
  if (source.path.empty()) {
    return Err("IOSDecoder::open failed: path is empty");
  }

  @autoreleasepool {
    NSString *nsPath = [NSString stringWithUTF8String:source.path.c_str()];
    if (nsPath == nil) {
      return Err("IOSDecoder::open failed: invalid path encoding");
    }
    NSURL *url = [NSURL fileURLWithPath:nsPath];
    ExtAudioFileRef extFile = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    OSStatus status = ExtAudioFileOpenURL((__bridge CFURLRef)url, &extFile);
    if (status != noErr || extFile == nullptr) {
      return Err("IOSDecoder::open ExtAudioFileOpenURL failed: " + std::to_string(status));
    }

    auto state = std::make_unique<IosDecoderState>();
    state->extFile.reset(extFile);
    return finishOpen(std::move(state), source.sampleRate);
  }
}

decoding::DecoderResult IOSDecoder::open(const decoding::EncodedMemorySource &source)
{
  close();
  if (source.data.empty()) {
    return Err("IOSDecoder::open failed: empty input");
  }

  auto state = std::make_unique<IosDecoderState>();
  state->memory = source.data;

  // Wrap the owned bytes with AudioFile callbacks, then wrap that with
  // ExtAudioFile so we get the same read/seek/convert path as open without
  // touching the filesystem.
  const AudioFileTypeID hint = guessFileTypeHint(source.data.data(), source.data.size());

  AudioFileID audioFile = nullptr;
  OSStatus status = AudioFileOpenWithCallbacks(
      state.get(), memoryReadProc, nullptr, memoryGetSizeProc, nullptr, hint, &audioFile);
  if (status != noErr || audioFile == nullptr) {
    return Err("IOSDecoder::open AudioFileOpenWithCallbacks failed: " + std::to_string(status));
  }
  state->audioFile.reset(audioFile);

  ExtAudioFileRef extFile = nullptr;
  status =
      ExtAudioFileWrapAudioFileID(state->audioFile.get(), static_cast<Boolean>(false), &extFile);
  if (status != noErr || extFile == nullptr) {
    return Err("IOSDecoder::open ExtAudioFileWrapAudioFileID failed: " + std::to_string(status));
  }
  state->extFile.reset(extFile);

  return finishOpen(std::move(state), source.sampleRate);
}

size_t IOSDecoder::readPcmFrames(float *outInterleaved, size_t frameCount)
{
  if (!open_ || impl_ == nullptr || impl_->extFile == nullptr || outInterleaved == nullptr ||
      frameCount == 0 || outputChannels_ <= 0) {
    return 0;
  }

  AudioBufferList bufferList{};
  bufferList.mNumberBuffers = 1;
  bufferList.mBuffers[0].mNumberChannels = static_cast<UInt32>(outputChannels_);
  bufferList.mBuffers[0].mDataByteSize =
      static_cast<UInt32>(frameCount * static_cast<size_t>(outputChannels_) * sizeof(float));
  bufferList.mBuffers[0].mData = outInterleaved;

  auto framesToRead = static_cast<UInt32>(frameCount);
  const OSStatus status = ExtAudioFileRead(impl_->extFile.get(), &framesToRead, &bufferList);
  if (status != noErr) {
    return 0;
  }
  framePosition_ += framesToRead;
  return framesToRead;
}

decoding::DecoderResult IOSDecoder::seekToTime(double seconds)
{
  if (!open_ || impl_ == nullptr || impl_->extFile == nullptr) {
    return Err("IOSDecoder::seekToTime failed: decoder is not open");
  }
  if (!std::isfinite(seconds) || seconds < 0.0) {
    return Err("IOSDecoder::seekToTime failed: seconds is not finite");
  }

  const auto fileFrame = static_cast<SInt64>(std::llround(seconds * impl_->fileSampleRate));
  const OSStatus status = ExtAudioFileSeek(impl_->extFile.get(), fileFrame);
  if (status != noErr) {
    return Err("IOSDecoder::seekToTime ExtAudioFileSeek failed: " + std::to_string(status));
  }
  framePosition_ =
      static_cast<int64_t>(std::llround(seconds * static_cast<double>(outputSampleRate_)));
  return Ok(None);
}

void IOSDecoder::releaseImpl()
{
  impl_.reset();
}

} // namespace audioapi::ios_decoder
