#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/IOSRemux.h>

#include <cctype>
#include <string>

namespace audioapi::ios_remux {
namespace {

struct AudioFormatFingerprint {
  AudioFormatID formatId{0};
  double sampleRate{0.0};
  UInt32 channelCount{0};
};

[[nodiscard]] NSString *nsStringFromPath(const std::string &path)
{
  return [[NSString alloc] initWithBytes:path.data()
                                  length:path.size()
                                encoding:NSUTF8StringEncoding];
}

[[nodiscard]] std::string lowercaseExtension(const std::string &path)
{
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos) {
    return "";
  }
  std::string extension = path.substr(dot + 1);
  for (char &c : extension) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return extension;
}

[[nodiscard]] bool isAacFormatId(AudioFormatID formatId)
{
  return formatId == kAudioFormatMPEG4AAC || formatId == kAudioFormatMPEG4AAC_HE ||
      formatId == kAudioFormatMPEG4AAC_HE_V2 || formatId == kAudioFormatMPEG4AAC_ELD;
}

[[nodiscard]] AVFileType fileTypeForExtension(const std::string &extension)
{
  if (extension == "mp4") {
    return AVFileTypeMPEG4;
  }
  return AVFileTypeAppleM4A;
}

[[nodiscard]] IOSRemuxResult
extractFingerprint(AVAssetTrack *track, AudioFormatFingerprint &out, const std::string &filePath)
{
  NSArray *descriptions = track.formatDescriptions;
  if (descriptions == nil || descriptions.count == 0) {
    return Err("Input file '" + filePath + "' is missing audio format descriptions.");
  }

  const auto *formatDescription = (__bridge CMAudioFormatDescriptionRef)descriptions[0];
  const AudioStreamBasicDescription *asbd =
      CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);
  if (asbd == nullptr) {
    return Err("Input file '" + filePath + "' is missing audio stream basic description.");
  }

  out.formatId = asbd->mFormatID;
  out.sampleRate = asbd->mSampleRate;
  out.channelCount = asbd->mChannelsPerFrame;
  return Ok(filePath);
}

[[nodiscard]] IOSRemuxResult validateCompatible(
    const AudioFormatFingerprint &candidate,
    const AudioFormatFingerprint &reference,
    const std::string &filePath)
{
  if (candidate.formatId != reference.formatId) {
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

} // namespace

IOSRemuxResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath)
{
  @autoreleasepool {
    if (inputPaths.empty()) {
      return Err("concatAudioFiles requires at least one input path.");
    }

    NSDictionary *assetOptions = @{AVURLAssetPreferPreciseDurationAndTimingKey : @YES};
    AVMutableComposition *composition = [AVMutableComposition composition];
    AVMutableCompositionTrack *compositionTrack =
        [composition addMutableTrackWithMediaType:AVMediaTypeAudio
                                 preferredTrackID:kCMPersistentTrackID_Invalid];
    if (compositionTrack == nil) {
      return Err("Failed to create AVMutableComposition audio track.");
    }

    AudioFormatFingerprint referenceFingerprint;
    bool hasReference = false;
    CMTime cursor = kCMTimeZero;

    for (const auto &path : inputPaths) {
      NSURL *url = [NSURL fileURLWithPath:nsStringFromPath(path)];
      if (url == nil) {
        return Err("Failed to create file URL for '" + path + "'.");
      }

      AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:assetOptions];
      NSArray<AVAssetTrack *> *audioTracks = [asset tracksWithMediaType:AVMediaTypeAudio];
      if (audioTracks.count == 0) {
        return Err("Input file '" + path + "' does not contain an audio stream.");
      }

      AVAssetTrack *audioTrack = audioTracks.firstObject;
      AudioFormatFingerprint fingerprint;
      auto fingerprintResult = extractFingerprint(audioTrack, fingerprint, path);
      if (fingerprintResult.is_err()) {
        return fingerprintResult;
      }

      if (!isAacFormatId(fingerprint.formatId)) {
        return Err(
            "Input file '" + path + "' is not AAC-in-M4A/MP4; only AAC concat is supported.");
      }

      if (!hasReference) {
        referenceFingerprint = fingerprint;
        hasReference = true;
      } else {
        auto validation = validateCompatible(fingerprint, referenceFingerprint, path);
        if (validation.is_err()) {
          return validation;
        }
      }

      NSError *insertError = nil;
      const CMTimeRange timeRange = CMTimeRangeMake(kCMTimeZero, asset.duration);
      const BOOL inserted = [compositionTrack insertTimeRange:timeRange
                                                      ofTrack:audioTrack
                                                       atTime:cursor
                                                        error:&insertError];
      if (!inserted) {
        NSString *message = insertError.localizedDescription ?: @"unknown error";
        return Err(
            "Failed to insert '" + path + "' into composition: " + std::string(message.UTF8String));
      }

      cursor = CMTimeAdd(cursor, asset.duration);
    }

    NSURL *outputURL = [NSURL fileURLWithPath:nsStringFromPath(outputPath)];
    if (outputURL == nil) {
      return Err("Failed to create output file URL for '" + outputPath + "'.");
    }
    [[NSFileManager defaultManager] removeItemAtURL:outputURL error:nil];

    // AAC-in-M4A is re-encoded once through AVAssetExportPresetAppleM4A. Passthrough
    // export and raw compressed-sample remux both fail on rotated recorder segments
    // (different format descriptions / priming), so this is the reliable path. It
    // stays FFmpeg-free — encoding is done by AVFoundation.
    AVAssetExportSession *exporter =
        [[AVAssetExportSession alloc] initWithAsset:composition
                                         presetName:AVAssetExportPresetAppleM4A];
    if (exporter == nil) {
      return Err("Failed to create AVAssetExportSession for AAC concat.");
    }

    exporter.outputURL = outputURL;
    exporter.outputFileType = fileTypeForExtension(lowercaseExtension(outputPath));

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block AVAssetExportSessionStatus exportStatus = AVAssetExportSessionStatusUnknown;
    __block NSString *exportErrorMessage = nil;

    [exporter exportAsynchronouslyWithCompletionHandler:^{
      exportStatus = exporter.status;
      if (exporter.error != nil) {
        exportErrorMessage = exporter.error.localizedDescription;
      }
      dispatch_semaphore_signal(semaphore);
    }];

    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

    if (exportStatus != AVAssetExportSessionStatusCompleted) {
      std::string message = "Failed to export concatenated audio";
      if (exportErrorMessage != nil) {
        message += ": " + std::string(exportErrorMessage.UTF8String);
      }
      return Err(message);
    }

    return Ok(outputPath);
  }
}

} // namespace audioapi::ios_remux
