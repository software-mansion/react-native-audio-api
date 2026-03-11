# AudioRecorder

AudioRecorder is a primary interface for capturing audio. It supports three main modes of operations:

* **File recording:** Writing audio data directly to the filesystem.
* **Data callback:** Emitting raw audio buffers, that can be used in either further processing or streamed.
* **Graph processing:** Connect the recorder with either `AudioContext` or `OfflineAudioContext` for further more advanced and/or realtime processing

## Configuration

To access microphone you need to make sure your app has required permission configuration - check [getting started permission section](/docs/fundamentals/getting-started#special-permissions) for more information.

Additionally to be able to record audio while application is in the background, you need to enable background mode on iOS and configure foreground service on android.

In an Expo application you can do so through `react-native-audio-api` expo plugin, e.g.

```json
{
  "plugins": [
    [
      "react-native-audio-api",
      {
        "iosBackgroundMode": true,
        "iosMicrophonePermission": "[YOUR_APP_NAME] requires access to the microphone to record audio.",
        "androidPermissions" : [
          "android.permission.RECORD_AUDIO",
          "android.permission.FOREGROUND_SERVICE",
          "android.permission.FOREGROUND_SERVICE_MICROPHONE",
        ],
        "androidForegroundService": true,
        "androidFSTypes": ["microphone"]
      }
    ]
  ]
}
```

For more configuration options, check out the [Expo plugin section](/docs/other/audio-api-plugin).

For bare react-native applications, background mode is configurable through `Signing & Capabilities` section of your app target config using XCode

![background mode configuration example](/docs/recorder/signing-and-capabilities.png)

Microphone permission can be created or modified through the `Info.plist` file

![Info plist example](/docs/recorder/info-plist.png)

Alternatively you can modify the `Info.plist` file directly in your editor of choice by adding those lines:

```xml
<key>NSMicrophoneUsageDescription</key>
<string>$(PRODUCT_NAME) wants to access your microphone in order to use voice memo recording</string>
<key>UIBackgroundModes</key>
<array>
  <string>audio</string>
</array>
```

To enable required permissions or foreground service you have to manually edit the `AndroidManifest.xml` file

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">

  
  <uses-permission android:name="android.permission.FOREGROUND_SERVICE"/>
  <uses-permission android:name="android.permission.FOREGROUND_SERVICE_MICROPHONE"/>

  
  <uses-permission android:name="android.permission.RECORD_AUDIO"/>

  
    <service android:stopWithTask="true" android:name="com.swmansion.audioapi.system.CentralizedForegroundService" android:foregroundServiceType="microphone" />
</manifest>

```

## Usage

```tsx
import React, { useState } from 'react';
import { View, Pressable, Text } from 'react-native';
import { AudioRecorder, AudioManager } from 'react-native-audio-api';

AudioManager.setAudioSessionOptions({
  iosCategory: 'record',
  iosMode: 'default',
  iosOptions: [],
});

const audioRecorder = new AudioRecorder();

// Enables recording to file with default configuration
audioRecorder.enableFileOutput();

const MyRecorder: React.FC = () => {
  const [isRecording, setIsRecording] = useState(false);

  const onStart = async () => {
    if (isRecording) {
      return;
    }

    // Make sure the permissions are granted
    const permissions = await AudioManager.requestRecordingPermissions();

    if (permissions !== 'Granted') {
      console.warn('Permissions are not granted');
      return;
    }

    // Activate audio session
    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      console.warn('Could not activate the audio session');
      return;
    }

    const result = audioRecorder.start();
    if (result.status === 'error') {
      console.warn(result.message);
      return;
    }

    console.log('Recording started to file:', result.path);
    setIsRecording(true);
  };

  const onStop = () => {
    if (!isRecording) {
      return;
    }

    const result = audioRecorder.stop();
    console.log(result);
    setIsRecording(false);
    AudioManager.setAudioSessionActivity(false);
  };

  return (
    <View>
      <Pressable onPress={isRecording ? onStop : onStart}>
        <Text>{isRecording ? 'Stop' : 'Record'}</Text>
      </Pressable>
    </View>
  );
};

export default MyRecorder;
```

```tsx
import React, { useState, useEffect } from 'react';
import { View, Pressable, Text } from 'react-native';
import { AudioRecorder, AudioManager } from 'react-native-audio-api';

AudioManager.setAudioSessionOptions({
  iosCategory: 'record',
  iosMode: 'default',
  iosOptions: [],
});

const audioRecorder = new AudioRecorder();
const sampleRate = 16000;

const MyRecorder: React.FC = () => {
  const [isRecording, setIsRecording] = useState(false);

  useEffect(() => {
    audioRecorder.onAudioReady(
      {
        sampleRate,
        bufferLength: sampleRate * 0.1, // 0.1s of audio each batch
        channelCount: 1,
      },
      ({ buffer, numFrames, when }) => {
        // do something with the data, i.e. stream it
      }
    );

    return () => {
      audioRecorder.clearOnAudioReady();
    };
  }, []);

  const onStart = async () => {
    if (isRecording) {
      return;
    }

    // Make sure the permissions are granted
    const permissions = await AudioManager.requestRecordingPermissions();

    if (permissions !== 'Granted') {
      console.warn('Permissions are not granted');
      return;
    }

    // Activate audio session
    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      console.warn('Could not activate the audio session');
      return;
    }

    const result = audioRecorder.start();

    if (result.status === 'error') {
      console.warn(result.message);
      return;
    }

    setIsRecording(true);
  };

  const onStop = () => {
    if (!isRecording) {
      return;
    }

    audioRecorder.stop();
    setIsRecording(false);
    AudioManager.setAudioSessionActivity(false);
  };

  return (
    <View>
      <Pressable onPress={isRecording ? onStop : onStart}>
        <Text>{isRecording ? 'Stop' : 'Record'}</Text>
      </Pressable>
    </View>
  );
};

export default MyRecorder;
```

```tsx
import React, { useState } from 'react';
import { View, Pressable, Text } from 'react-native';
import {
  AudioRecorder,
  AudioContext,
  AudioManager,
} from 'react-native-audio-api';

AudioManager.setAudioSessionOptions({
  iosCategory: 'playAndRecord',
  iosMode: 'default',
  iosOptions: [],
});

const audioRecorder = new AudioRecorder();
const audioContext = new AudioContext();

const MyRecorder: React.FC = () => {
  const [isRecording, setIsRecording] = useState(false);

  const onStart = async () => {
    if (isRecording) {
      return;
    }

    // Make sure the permissions are granted
    const permissions = await AudioManager.requestRecordingPermissions();

    if (permissions !== 'Granted') {
      console.warn('Permissions are not granted');
      return;
    }

    // Activate audio session
    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      console.warn('Could not activate the audio session');
      return;
    }

    const adapter = audioContext.createRecorderAdapter();
    adapter.connect(audioContext.destination);
    audioRecorder.connect(adapter);

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    const result = audioRecorder.start();

    if (result.status === 'error') {
      console.warn(result.message);
      return;
    }

    setIsRecording(true);
  };

  const onStop = () => {
    if (!isRecording) {
      return;
    }

    audioRecorder.stop();
    audioContext.suspend();
    setIsRecording(false);
    AudioManager.setAudioSessionActivity(false);
  };

  return (
    <View>
      <Pressable onPress={isRecording ? onStop : onStart}>
        <Text>{isRecording ? 'Stop' : 'Record'}</Text>
      </Pressable>
    </View>
  );
};

export default MyRecorder;
```

## API

MethodDescription

##### Constructor

Creates new instance of AudioRecorder. It is preferred to create only a single instance of the AudioRecorder class for the best performance, memory and battery consumption reasons. While the idle recorder has minimal impact on anything mentioned, switching between separate recorder instances might have a noticeable impact on the device.

```tsx
import { AudioRecorder } from 'react-native-audio-api';

const audioRecorder = new AudioRecorder();
```

##### start

Starts the stream from system audio input device.
You can pass optional object with `fileNameOverride` string, to provide your own fileName generation.

```tsx
const result = audioRecorder.start({
  fileNameOverride: `my_audio_${mySessionId}`
});

if (result.status === 'success') {
  const openedFilePath = result.path;
} else if (result.status === 'error') {
  console.error(result.message);
}
```

##### stop

Stops the input stream and cleans up each input access method.

```tsx
const result = audioRecorder.stop();

if (result.status === 'success') {
  const { path, duration, size } = result;
} else if (result.status === 'error') {
  console.error(result.message);
}
```

##### pause

Pauses the recording. This is useful when recording to file is active, but you don't want to finalize the file.

```tsx
  audioRecorder.pause();
```

##### resume

Resumes the recording if it was previously paused, otherwise does nothing.

```tsx
  audioRecorder.resume();
```

##### isRecording

Returns `true` if the recorder is in active/recording state

```tsx
  const isRecording = audioRecorder.isRecording();
```

##### isPaused

Returns `true` if the recorder is in paused state.

```tsx
  const isPaused = audioRecorder.isPaused();
```

##### onError

Sets an error callback for any possible internal error that might happen during file writing, callback invocation or adapter access.

For details check: [OnRecorderErrorEventType](#onrecordererroreventtype)

```tsx
  audioRecorder.onError((error: OnRecorderErrorEventType) => {
    console.log(error);
  });
```

##### clearOnError

Removes the error callback.

```tsx
  audioRecorder.clearOnError();
```

### Recording to file

MethodDescription

##### enableFileOutput

Configures and enables the file output with defined options and stream properties. Options property allows for configuration of the output file structure and quality. By default the recorder writes to cache directory using high-quality `M4A` file.

For further information check: [AudioRecorderFileOptions](#audiorecorderfileoptions)

```tsx
  audioRecorder.enableFileOutput();
```

##### disableFileOutput

Disables the file output and finalizes the currently recorded file if the recorder is active.

```tsx
  audioRecorder.disableFileOutput();
```

##### getCurrentDuration

Returns current recording duration if recording to file is enabled.

```tsx
  const duration = audioRecorder.getCurrentDuration();
```

### Data callback

MethodDescription

##### onAudioReady

The callback is periodically invoked with audio buffers that match the preferred configuration provided in `options`. These parameters (sample rate, buffer length, and channel count) guide how audio data is chunked and delivered, though the exact values may vary depending on device capabilities.

For further information check:

* [AudioRecorderCallbackOptions](#audiorecordercallbackoptions)
* [OnAudioReadyEventType](#onaudioreadyeventtype)

```tsx
  const sampleRate = 16000;

  audioRecorder.onAudioReady(
    {
      sampleRate,
      bufferLength: 0.1 * sampleRate, // 0.1s of data
      channelCount: 1,
    },
    ({ buffer, numFrames, when }) => {
      // do something with the data
    });
```

##### clearOnAudioReady

Disables and flushes the remaining audio data through `onAudioReady` callback as explained above.

```tsx
  audioRecorder.clearOnAudioReady();
```

#### Graph processing

MethodDescription

##### connect

Connects AudioRecorder with [RecorderAdapterNode](/docs/sources/recorder-adapter-node) instance that can be used for further audio processing.

```tsx
  const adapter = audioContext.createRecorderAdapter();
  audioRecorder.connect(adapter);
```

##### disconnect

Disconnects AudioRecorder from the audio graph.

```tsx
  audioRecorder.disconnect();
```

## Types

#### AudioRecorderCallbackOptions

```tsx
interface AudioRecorderCallbackOptions {
  sampleRate: number;
  bufferLength: number;
  channelCount: number;
}
```

* `sampleRate` - The desired sample rate (in Hz) for audio buffers delivered to the
  recording callback. Common values include 44100 or 48000 Hz. The actual
  sample rate may differ depending on hardware and system capabilities.

* `bufferLength` - The preferred size of each audio buffer, expressed as the number of samples per channel. Smaller buffers reduce latency but increase CPU load, while larger buffers improve efficiency at the cost of higher latency.

* `channelCount` - The desired number of audio channels per buffer. Typically 1 for mono or 2 for stereo recordings.

#### OnRecorderErrorEventType

```tsx
interface OnRecorderErrorEventType {
  message: string;
}
```

#### OnAudioReadyEventType

Represents the data payload received by the audio recorder callback each time a new audio buffer becomes available during recording.

```tsx
interface OnAudioReadyEventType {
  buffer: AudioBuffer;
  numFrames: number;
  when: number;
}
```

* `buffer` - The audio buffer containing the recorded PCM data. This buffer includes one or more channels of floating-point samples in the range of -1.0 to 1.0.
* `numFrames` - The number of audio frames contained in this buffer. A frame represents a single sample across all channels.
* `when` - The timestamp (in seconds) indicating when this buffer was captured, relative to the start of the recording session.

### File handling

#### AudioRecorderFileOptions

```tsx
interface AudioRecorderFileOptions {
  channelCount?: number;

  format?: FileFormat;
  preset?: FilePresetType;

  directory?: FileDirectory;
  subDirectory?: string;
  fileNamePrefix?: string;
  androidFlushIntervalMs?: number;
}
```

* `channelCount` - The desired channel count in the resulting file. not all file formats supports all possible channel counts.
* `format` - The desired extension and file format of the recorder file. Check: [FileFormat](#fileformat) below.
* `preset` - The desired recorder file properties, you can use either one of built-in properties or tweak low-level parameters yourself. Check [FilePresetType](#filepresettype) for more details.
* `directory` - Either `FileDirectory.Cache` or `FileDirectory.Document` (default: `FileDirectory.Cache`). Determines the system directory that the file will be saved to.
* `subDirectory` - If configured it will create the recording inside requested directory (default: `undefined`).
* `fileNamePrefix` - Prefix of the recording files without the unique ID (default: `recording`).
* `androidFlushIntervalMs` - How often the recorder should force the system to write data to the device storage (default: `500`).
  * Lower values are good for crash-resilience and are more memory friendly.
  * Higher values are more battery- and storage-efficient.

#### FileFormat

Describes desired file extension as well as codecs, containers (and muxers!) used to encode the file.

```tsx
enum FileFormat {
  Wav,
  Caf,
  M4A,
  Flac,
}
```

#### FilePresetType

Describes audio format that is used during writing to file as well as encoded final file properties. You can use one of predefined presets, or fully customize the result file, but be aware that the properties aren't limited to only valid configurations, you may find property pairs that will result in error result during recording start (or when enabling the file output during active input session)!

##### Built-in file presets

For convenience we have provided set of most basic file configurations that should cover most of the cases (or at least we hope they will, please raise an issue if you find something lacking or misconfigured!).

###### Usage

```tsx
import { AudioRecorder, FileFormat, FilePreset } from 'react-native-audio-api';

const audioRecorder = new AudioRecorder();

audioRecorder.enableFileOutput({
  format: FileFormat.M4A,
  preset: FilePreset.High,
});
```

PresetDescription

##### Lossless

Writes audio data directly to file without encoding, preserving the maximum audio quality supported by the device. This results in large file sizes, particularly for longer recordings. Available only when using WAV or CAF file formats.

```tsx
audioRecorder.enableFileOutput({
  format: FileFormat.CAF,
  preset: FilePreset.Lossless,
});
```

##### High Quality

Uses high-fidelity audio parameters with efficient encoding to deliver near-lossless perceptual quality while producing smaller files than fully uncompressed recordings. Suitable for music and high-quality voice capture.

```tsx
audioRecorder.enableFileOutput({
  format: FileFormat.Flac,
  preset: FilePreset.High,
});
```

##### Medium Quality

Uses balanced audio parameters that provide good perceptual quality while keeping file sizes moderate. Intended for everyday recording scenarios such as voice notes, podcasts, and general in-app audio, where efficiency and compatibility outweigh maximum fidelity.

```tsx
audioRecorder.enableFileOutput({
  format: FileFormat.M4A,
  preset: FilePreset.Medium,
});
```

##### Low Quality

Uses reduced audio parameters to minimize file size and processing overhead. Designed for cases where speech intelligibility is sufficient and audio fidelity is not critical, such as quick voice notes, background recording, or diagnostic capture.

```tsx
audioRecorder.enableFileOutput({
  format: FileFormat.M4A,
  preset: FilePreset.Low,
});
```

#### Preset customization

In addition to the predefined presets, you may supply a custom FilePresetType to fine-tune how audio data is written and encoded. This allows you to optimize for specific use cases such as speech-only recording, reduced storage footprint, or faster encoding.

```tsx
export interface FilePresetType {
  bitRate: number;
  sampleRate: number;
  bitDepth: BitDepth;
  iosQuality: IOSAudioQuality;
  flacCompressionLevel: FlacCompressionLevel;
}
```

PropertyDescription

##### bitRate

Defines the target bitrate for lossy encoders (for example AAC or M4A). Higher values generally improve perceptual quality at the cost of larger file sizes. This value may be ignored when using lossless formats.

| Use case | Bitrate (bps) | Notes |
| :- | - | :- |
| Very low quality / telemetry |	32000 |	Bare minimum for speech intelligibility |
| Low quality voice notes |	48000 |	Optimized for small files and fast encoding |
| Standard speech / podcasts |	64000 – 96000 |	Good balance of clarity and size |
| Medium quality general audio |	128000 |	Common default for consumer audio |
| High quality music / voice |	160000 – 192000 |	Near-transparent for most listeners |
| Very high quality |	256000 – 320000 |	Large files, minimal perceptual loss |

##### sampleRate

Specifies the sampling frequency used during recording. Higher sample rates capture a wider frequency range but increase processing and storage requirements.

##### bitDepth

Controls the PCM bit depth of the recorded audio. Higher bit depths increase dynamic range and precision, primarily affecting uncompressed or lossless output formats.

##### iosQuality

Maps the preset to the closest matching quality level provided by iOS native audio APIs, ensuring consistent behavior across Apple devices.

```tsx
enum IOSAudioQuality {
  Min,
  Low,
  Medium,
  High,
  Max,
}
```

##### flacCompressionLevel

Determines the compression level used when encoding FLAC files. Higher levels reduce file size at the cost of increased CPU usage, without affecting audio quality.

```tsx
enum FlacCompressionLevel {
  L0,
  L1,
  L2,
  L3,
  L4,
  L5,
  L6,
  L7,
  L8,
}
```

## Remarks & known issues
