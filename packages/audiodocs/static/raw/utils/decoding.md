# Decoding

You can decode audio data independently, without creating an AudioContext, using the exported functions [`decodeAudioData`](/docs/utils/decoding#decodeaudiodata) and
[`decodePCMInBase64`](/docs/utils/decoding#decodepcminbase64).

> **Warning**
>
> Decoding on the web has to be done via `AudioContext` only.

If you already have an audio context, you can decode audio data directly using its [`decodeAudioData`](/docs/core/base-audio-context#decodeaudiodata) function;
the decoded audio will then be automatically resampled to match the context's `sampleRate`.

> **Caution**
>
> Supported file formats:
>
> * flac
> * mp3
> * ogg
> * opus
> * wav
> * aac
> * m4a
> * mp4
>
> Last three formats are decoded with ffmpeg on the mobile, [see for more info](/docs/other/ffmpeg-info).

### `decodeAudioData`

Decodes audio data from either a file path or an ArrayBuffer. The optional `sampleRate` parameter lets you resample the decoded audio;
if not provided, the original sample rate from the file is used.

Parameter
Type
Description

input
ArrayBuffer
ArrayBuffer with audio data.

string
Path to remote or local audio file.

number
Asset module id. &#x20;

sampleRate
number
Target sample rate for the decoded audio.

fetchOptions
[RequestInit](https://github.com/facebook/react-native/blob/ac06f3bdc76a9fd7c65ab899e82bff5cad9b94b6/packages/react-native/src/types/globals.d.ts#L265)
Additional headers parameters when passing url to fetch.

#### Returns `Promise<AudioBuffer>`.

> **Caution**
>
> If you are passing number to decode function, bear in mind that it uses Image component provided
> by React Native internally. By default only support .mp3, .wav, .mp4, .m4a, .aac audio file formats.
> If you want to use other types, refer to [this section](https://reactnative.dev/docs/images#static-non-image-resources) for more info.

Example decoding remote URL

```tsx
import { decodeAudioData } from 'react-native-audio-api';

const url = ... // url to an audio

const buffer = await decodeAudioData(url);
```

### `decodePCMInBase64`

Decodes base64-encoded PCM audio data.

| Parameter | Type | Description |
|-----------|------|-------------|
| `base64String` | `string` | Base64-encoded PCM audio data. |
| `inputSampleRate` | `number` | Sample rate of the input PCM data. |
| `inputChannelCount` | `number` | Number of channels in the input PCM data. |
| `isInterleaved` | `boolean` | Whether the PCM data is interleaved. Default is `true`. |

#### Returns `Promise<AudioBuffer>`

Example decoding with data in base64 format&#x20;

```tsx
const data = ... // data encoded in base64 string
// data is interleaved (Channel1, Channel2, Channel1, Channel2, ...)
const buffer = await decodeAudioData(data, 4800, 2, true);
```
