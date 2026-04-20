---
name: web-audio-api
description: >
  Web Audio API spec reference and browser passthrough layer (src/web-core/). Use when implementing
  a node that must match the Web Audio API spec, checking parameter default values and ranges,
  adding or modifying src/web-core/ wrapper classes, deciding whether a feature belongs to the spec
  or is an RN-specific extension, or updating the coverage table.
  Trigger phrases: "web-core", "spec compliance", "coverage table", "api.web.ts", "Web Audio spec",
  "parameter default", "browser passthrough", "not in spec", "spec deviation",
  "webaudio.github.io".
---

# Skill: Web Audio API

## Spec Reference

Everything that has a counterpart in the Web Audio API specification must match it:

- **Spec**: https://webaudio.github.io/web-audio-api/
- **MDN reference**: https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API

Key spec sections to check when implementing or reviewing a node:
- Processing model and render quantum (128 frames)
- `AudioNode` channel count rules and `channelCountMode` / `channelInterpretation`
- `AudioParam` automation methods, value clamping, and timing
- Each node's constructor options, property defaults, and valid ranges

---

## Platform Routing

The library ships **one TypeScript API** that runs on two platforms. The entry point swap happens via package.json field resolution:

```
index.ts          # re-exports from api.ts (or api.web.ts on web)
├── api.ts        # React Native — re-exports from src/core/*
└── api.web.ts    # Browser — re-exports from src/web-core/*
```

On **React Native**: classes in `src/core/` hold a reference to a C++ JSI HostObject. All method calls go to native.

On **Web (browser)**: classes in `src/web-core/` wrap the corresponding `globalThis.*` (browser Web Audio API) object. All method calls delegate directly to the browser engine.

Both sides share:
- The same TypeScript interfaces (`src/interfaces.ts`)
- The same types (`src/types.ts`)
- The same error classes (`src/errors/`)
- The same hooks (`src/hooks/`)

---

## `src/web-core/` — Browser Passthrough Layer

Each file in `src/web-core/` is a thin wrapper around the corresponding browser API object. The pattern is:

**Constructor**: instantiate the browser node via `globalThis.XxxNode` (or `new window.AudioContext`), store it as `this.node` (or `this.context`), read readonly properties from it.

**Getters/setters/methods**: delegate directly to `this.node`.

**AudioParam**: wrapped in the local `AudioParam` class (which stores `this.param: globalThis.AudioParam` and delegates all automation calls).

```ts
// Example: GainNode.tsx (minimal)
export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gainOptions?: GainOptions) {
    const gain = new globalThis.GainNode(context.context, gainOptions);
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}

// Example: OscillatorNode.tsx (with extra validation)
public set type(value: OscillatorType) {
  if (value === 'custom') {
    throw new InvalidStateError("'type' cannot be set to 'custom' directly...");
  }
  (this.node as globalThis.OscillatorNode).type = value;
}
```

**`AudioContext`** (web) wraps `window.AudioContext`. It also adds validation matching the RN side (e.g. sampleRate range check) so error behaviour is consistent across platforms.

**`decodeAudioData`** on web additionally supports a `string` URL source (fetches the file, then decodes). This is a deliberate extension beyond the browser spec signature (which only takes `ArrayBuffer`), mirroring the RN native implementation.

### Rules for web-core code

- Every public method/property must have a direct counterpart in the spec (or be in `custom/`)
- Extra validation (range checks, length checks) is fine — it makes error messages consistent with the RN side
- No business logic — the browser engine is the source of truth for audio processing
- If a node does not exist in the browser, it goes in `src/web-core/custom/`

---

## `src/web-core/custom/` — RN Extensions on Web

Nodes or features that don't exist in the Web Audio API spec but are in this library as mobile extensions. The `custom/index.ts` re-exports them and `api.web.ts` re-exports the custom barrel.

Currently: signalsmith-stretch WASM wrapper (`LoadCustomWasm.ts`, `signalsmithStretch/`) for time-stretch on web.

When adding a new RN-specific feature that should also work on web, implement the web version here.

---

## Implementation Coverage

Current status (from `packages/audiodocs/docs/other/web-audio-api-coverage.mdx`):

### Fully implemented ✅
`AnalyserNode`, `AudioBuffer`, `AudioBufferSourceNode`, `AudioDestinationNode`, `AudioNode`, `AudioParam`, `AudioScheduledSourceNode`, `BiquadFilterNode`, `ConstantSourceNode`, `ConvolverNode`, `DelayNode`, `GainNode`, `IIRFilterNode`, `OfflineAudioContext`, `OscillatorNode`, `PeriodicWave`, `StereoPannerNode`, `WaveShaperNode`

### Partially implemented 🚧
| Interface | What's available |
|---|---|
| `AudioContext` | `close`, `suspend`, `resume`, `currentTime`, `destination`, `sampleRate`, `state` |
| `BaseAudioContext` | `currentTime`, `destination`, `sampleRate`, `state`, `decodeAudioData`, all `create*` for available nodes |

### Not yet implemented ❌
`AudioListener`, `AudioSinkInfo`, `AudioWorklet`, `AudioWorkletGlobalScope`, `AudioWorkletNode`, `AudioWorkletProcessor`, `ChannelMergerNode`, `ChannelSplitterNode`, `DynamicsCompressorNode`, `MediaElementAudioSourceNode`, `MediaStreamAudioDestinationNode`, `MediaStreamAudioSourceNode`, `PannerNode`

**Goal**: everything in the Web Audio API spec should eventually be in this library. If you implement a node from the ❌ list, update the coverage table in `packages/audiodocs/docs/other/web-audio-api-coverage.mdx`.

---

## RN-Specific Extensions (beyond spec)

These are exported from `api.ts` but **not** from `api.web.ts` (or have a stub/custom web implementation):

| Class | Purpose |
|---|---|
| `AudioBufferQueueSourceNode` | Queue of audio buffers, plays them sequentially — no Web Audio spec equivalent |
| `StreamerNode` | FFmpeg-backed streaming decoder — no Web Audio spec equivalent |
| `AudioRecorder` | Microphone input recording — no Web Audio spec equivalent |
| `RecorderAdapterNode` | Connects recorder to the audio graph |
| `WorkletNode` / `WorkletSourceNode` / `WorkletProcessingNode` | JS-on-audio-thread via React Native Worklets — different from browser `AudioWorkletNode` |
| `AudioManager` | iOS/Android audio session management (permissions, routing, interruption handling) |
| `changePlaybackSpeed` (`AudioStretcher`) | Time-stretch without pitch change |
| `decodeAudioData` (standalone) | Standalone decode utility (not on context) |
| `decodePCMInBase64` | Decode raw PCM from base64 |

When implementing these on the RN side, a web stub or polyfill in `src/web-core/custom/` should be considered if the feature can be reasonably approximated in a browser.

---

## Adding a New Spec Node — Web Layer Checklist

When adding a new Web Audio API node that is in the spec:

1. **Implement `src/web-core/MyNode.tsx`**
   - Extend the right base class (`AudioNode`, `AudioScheduledSourceNode`)
   - Constructor: `new globalThis.MyNode(context.context, options)`
   - Wrap all `AudioParam` properties in `new AudioParam(node.myParam, context)`
   - Delegate all getters/setters/methods to `this.node`
   - Add any validation that matches the RN side's error behaviour

2. **Export from `src/api.web.ts`**
   ```ts
   export { default as MyNode } from './web-core/MyNode';
   ```

3. **Ensure the interface in `src/interfaces.ts`** (or a dedicated interface file) is shared between both paths.

4. **Update the coverage table** in `packages/audiodocs/docs/other/web-audio-api-coverage.mdx` — move the node from ❌ to ✅.

If the node **does not exist in the browser** (e.g. `AudioBufferQueueSourceNode`):
- Add a stub or alternative implementation in `src/web-core/custom/`
- Export it from `src/web-core/custom/index.ts`
- It will be picked up automatically by the `export * from './web-core/custom'` line in `api.web.ts`

---

## Spec Compliance Notes

When in doubt, cross-check the spec. Key invariants that are easy to get wrong:

- **`feedback[0]` must not be 0** for `IIRFilterNode` — the spec requires it; we validate in TypeScript and in C++.
- **`feedforward` all-zeros** must throw `InvalidStateError` — the spec requires at least one non-zero coefficient.
- **`OscillatorNode.type = 'custom'`** must throw `InvalidStateError` — use `setPeriodicWave()` instead.
- **`AudioParam` min/max** — must match the spec's exact values; do not invent ranges.
- **`createBuffer` / `createDelay` / `createPeriodicWave`** — the spec defines when these throw and what error type. Match it.
- **`decodeAudioData`** — on RN side, we extend it to accept a URL string in addition to `ArrayBuffer`. This is intentional and documented.
- **`sampleRate` range** — spec says [8000, 96000]; enforced in `AudioContext` constructor on both platforms.

---

*Maintenance: see [maintenance.md](maintenance.md).*
