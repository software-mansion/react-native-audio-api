# Signalsmith Stretch Web

This is an official release of the Signalsmith Stretch library for Web Audio, using WASM/AudioWorklet. It includes both plain `.js` (UMD), and ES6 `.mjs` versions.

## How to use it

Call `SignalsmithStretch(audioContext, ?channelOptions)` from the main thread. This returns a Promise which resolves to an `AudioNode`, with extra methods attached to it.

It can operate either on live/streaming input, or on a sample buffer you load into it.

### `stretch.inputTime`

The current input time, within the sample buffer. You can change how often this is updated, with an optional callback function, using `stretch.setUpdateInterval(seconds, ?callback)`.

### `stretch.schedule({...})`

This adds a scheduled change, removing any scheduled changes occurring after this one. The object properties are:

- `output` (seconds): audio context time for this change. The node compensates for its own latency, but this means you might want to schedule some things ahead of time, otherwise you'll have a softer transition as it catches up.
- `active` (bool): processing audio
- `input` (seconds): position in input buffer
- `rate` (number): playback rate, e.g. 0.5 == half speed
- `semitones` (number): pitch shift
- `loopStart` / `loopEnd`: sets a section of the input to auto-loop. Disabled if both are set to the same value.

### `stretch.start(?when)` / `stretch.stop(?when)`

Starts/stops playback or processing, immediately or at some future time. These are convenience methods which call `.schedule(...)` under the hood.

`.start()` actually has more parameters, presenting a similar interface to [AudioBufferSourceNode](https://developer.mozilla.org/en-US/docs/Web/API/AudioBufferSourceNode/start).

### `stretch.addBuffers([...])`

This adds buffers to the end of the current input. Buffers should be typed arrays of equal length, one per channel.

It can be called multiple times, and the new buffers are inserted after the existing ones, which lets you start playback before the entire audio is loaded. It returns (as a Promise) the new end time for the stored input, in seconds.

### `stretch.dropBuffers()`

This drops all input buffers, and resets the input buffer start time to 0.

### `stretch.dropBuffers(toSeconds)`

This drops all input buffers before the given time. It returns (as a Promise) the an object with the current input buffer extent: `{start: ..., end: ...}`.

This can be useful when processing streams or very long audio files, letting the Stretch node release old buffers once that section of the input will no longer be played back.
