import {
  AudioEncoderOptions,
  BitDepth,
  FileDirectory,
  FileFormat,
  FilePresetType,
  FlacCompressionLevel,
  IOSAudioQuality,
} from '../types';

const LowQuality: FilePresetType = {
  sampleRate: 22050,
  bitRate: 48000,
  bitDepth: BitDepth.Bit16,
  flacCompressionLevel: FlacCompressionLevel.L0,
  iosQuality: IOSAudioQuality.Low,
};

const MediumQuality: FilePresetType = {
  sampleRate: 44100,
  bitRate: 128000,
  bitDepth: BitDepth.Bit16,
  flacCompressionLevel: FlacCompressionLevel.L3,
  iosQuality: IOSAudioQuality.Medium,
};

const HighQuality: FilePresetType = {
  sampleRate: 48000,
  bitRate: 192000,
  bitDepth: BitDepth.Bit24,
  flacCompressionLevel: FlacCompressionLevel.L5,
  iosQuality: IOSAudioQuality.High,
};

const LosslessQuality: FilePresetType = {
  sampleRate: 48000,
  bitRate: 320000,
  bitDepth: BitDepth.Bit24,
  flacCompressionLevel: FlacCompressionLevel.L8,
  iosQuality: IOSAudioQuality.High,
};

const FilePreset = {
  Low: LowQuality,
  Medium: MediumQuality,
  High: HighQuality,
  Lossless: LosslessQuality,
} as const;

export default FilePreset;

// Enforces default file output options, making sure that all properties are
// defined for the contract with native code.
export function withDefaultEncoderOptions(
  inOptions: AudioEncoderOptions
): Required<AudioEncoderOptions> {
  return {
    directory: inOptions.directory ?? FileDirectory.Cache,
    subDirectory: inOptions.subDirectory ?? 'AudioAPI',
    fileName: inOptions.fileName ?? '',
    channelCount: inOptions.channelCount ?? 2,
    format: inOptions.format ?? FileFormat.M4A,
    preset: inOptions.preset ?? FilePreset.High,
  };
}
