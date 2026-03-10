# StreamerNode

> **Caution**
>
> Mobile only.

The `StreamerNode` is an [`AudioScheduledSourceNode`](/docs/sources/audio-scheduled-source-node) which represents a node that can decode and play [Http Live Streaming](https://developer.apple.com/streaming/) data.
Similar to all of `AudioScheduledSourceNodes`, it can be started only once. If you want to play the same sound again you have to create a new one.

#### [`AudioNode`](/docs/core/audio-node#read-only-properties) properties

## Constructor

```tsx
constructor(context: BaseAudioContext, options: StreamerOptions)
```

### `StreamerOptions`

| Parameter | Type | Default | |
| :---: | :---: | :----: | :---- |
| `streamPath` | `string` | - | Value for [`streamPath`](/docs/sources/streamer-node#properties) |

Or by using `BaseAudioContext` factory method:

[`BaseAudioContext.createStreamer()`](/docs/core/base-audio-context#createstreamer).

## Example

```tsx
import React, { useRef } from 'react';
import {
  AudioContext,
  StreamerNode,
} from 'react-native-audio-api';

function App() {
  const audioContextRef = useRef<AudioContext | null>(null);
  if (!audioContextRef.current) {
    audioContextRef.current = new AudioContext();
  }
  const streamer = audioContextRef.current.createStreamer();
  streamer.initialize('link/to/your/hls/source');
  streamer.connect(audioContextRef.current.destination);
  streamer.start(audioContextRef.current.currentTime);
}
```

## Properties

It inherits all properties from [`AudioScheduledSourceNode`](/docs/sources/audio-scheduled-source-node#properties).

| Name | Type | Description |
| :----: | :----: | :------- |
| `streamPath`  | `string` | String value representing url to stream. |

## Methods

It inherits all methods from [`AudioScheduledSourceNode`](/docs/sources/audio-scheduled-source-node#methods).
