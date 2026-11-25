# apps/ — Example Applications

The main example app is `fabric-example` — it uses the New Architecture (TurboModules/Fabric) and serves as the primary manual smoke-test environment for new features.

## Architecture

`fabric-example` is a **thin wrapper**. Almost all code lives in the shared `common-app` workspace:

```
apps/
├── fabric-example/          # RN app shell (native + metro config)
│   ├── App.tsx              # Re-exports App from common-app
│   ├── metro.config.js      # Monorepo-aware Metro (watches workspaces)
│   └── android/ ios/        # Native projects
└── common-app/              # Actual app code — edit this
    ├── src/
    │   ├── App.tsx           # NavigationContainer (dark theme, deep links)
    │   ├── navigation/       # RootStack.tsx (native stack, root param list) + MainTabs.tsx (native tabs)
    │   ├── screens/          # Tab contents: TabScreen (large title), ExampleList (Tests / Other), DemoList (Demo Apps)
    │   ├── examples/         # "Tests" tab — focused feature examples
    │   │   └── index.ts      # Exports Examples array — register new screens here
    │   ├── demos/            # "Demo Apps" tab — full mini-apps
    │   │   └── index.ts      # Exports demos array — register new screens here
    │   ├── components/       # Shared UI (table below)
    │   ├── testComponents/   # Test-report UI for the Other tab: TestLayout, useRunLog, StepRow, ScenarioResults, LiveLog
    │   ├── utils/
    │   │   ├── audio.ts      # useAudioContext, useDecodedAudio, activatePlaybackSession
    │   │   ├── formatClock.ts # m:ss / mm:ss, worklet-safe
    │   │   ├── soundEngines/ # Reusable synth classes (Kick, HiHat, Clap); each voice disconnects itself on `onended`
    │   │   ├── systemOptions.ts # Runtime lists of every IOSCategory/IOSMode/IOSOption/AndroidAudioMode/CommunicationDevice/AndroidInputPreset/AndroidOutputProfile/AudioFocusType (the library exports only the types)
    │   │   └── usePlayer.tsx # Hook for pattern-based scheduled playback
    │   ├── singletons/       # Global audioContext and audioRecorder instances
    │   └── styles.ts         # Shared colors, layout constants and typography roles
    └── assets/icons/         # SVG sources for icons (see Icons below)
```

## Navigation

React Navigation 8 (pre-release) on `react-native-screens`: a native stack (`navigation/RootStack.tsx`) whose first screen is native bottom tabs (`navigation/MainTabs.tsx`). On iOS 26+ both render the system liquid glass header and tab bar; `@callstack/liquid-glass` is the glass peer `@react-navigation/elements` requires.

```
Bottom Tabs (native)
├── Tests       → Inset grouped list of Examples, one section per `exampleCategories` entry
├── Demo Apps   → Cards for the Demos (full mini-apps)
└── Other       → One grouped section of internal tooling / repro screens, registered in `other/index.ts` (`otherScreens`); add the key to `ExampleRoutes` in `examples/index.ts`
```

Each item navigates into a Stack screen. No manual stack registration needed — `examples/index.ts`, `other/index.ts` and `demos/index.ts` drive it automatically. The root param list is declared once in `RootStack.tsx` (`RootNavigator` module augmentation), so `useNavigation()` needs no generic or cast: `navigation.navigate(item.key)` is typed against the route keys (`ExampleKey`, `DemoKey`).

Screens with an always-visible control panel render `ControlsSheet` (`components/ControlsSheet.tsx`, built on `@swmansion/react-native-bottom-sheet`) as the last child of their `Container`. It is part of the screen, so it shares the screen's state directly; pad content with the height its `onCoveredHeightChange` reports. References: Drum Machine (`useDrumMachine` holds pattern, tempo and transport), Recorder, Audio Visualizer. These were once separate react-native-screens `formSheet` routes, which needed module stores to share state and a layout that popped the sheet when its owner screen left the stack.

On iOS the `MainTabs` route keeps an empty, transparent stack header (`title: ''`) instead of `headerShown: false`. Popping to a screen with no navigation bar makes UIKit slide the whole bar (back button, title, header items) out with the leaving screen during a back swipe. Like the system apps, bar buttons then stay put and fully opaque until the swipe completes or cancels.

On iOS 26 react-native-screens enables the system's content-wide back gesture by default (`fullScreenSwipeEnabled` → `interactiveContentPopGestureRecognizer`), so a rightward pan anywhere on a pushed screen can pop it. A screen with its own horizontal swipes sets `navigation.setOptions({ fullScreenGestureEnabled: false })`, which leaves the edge swipe and back button. Reference: `demos/Crossfade/Crossfade.tsx`.

React Navigation's native tabs report a tab bar height of 0 (`useBottomTabBarHeight`), so on Android, where the opaque tab bar overlays the scene, `TabScreen` wraps its scroll view in `SafeAreaView` from `react-native-screens/experimental` with the bottom edge and `insetType="interface"`, which reads the tab container's real bar height.

Native tabs cannot render React elements as icons: `tabBarIcon` returns `nativeNanoSymbol('<svg name>')` from a symbol set (see Icons).

## Icons

Icons are [react-native-nano-icons](https://nano-icons.swmansion.com/): SVGs compiled into a font at build time. Sources are vendored Lucide SVGs (`assets/icons/LICENSE-lucide`). `LICENSE-lucide` names no version; `mic-off` and `x` came from `lucide-static` 1.68.0 (`https://unpkg.com/lucide-static@<version>/icons/<name>.svg`), so take new ones from that release or later.

| Folder                          | Set                                                            | Rendered with                                                                 |
| ------------------------------- | -------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| `common-app/assets/icons/app/`  | `iconSets` → `app.ttf`                                         | `<AppIcon name="mic" size={24} color={...} />` (name typed from the glyphmap) |
| `common-app/assets/icons/tabs/` | `symbolSets` → SF Symbol `.symbolset` / Android VectorDrawable | `nativeNanoSymbol('gauge')` in `tabBarIcon`                                   |

The config lives in `fabric-example/.nanoicons.json` because the CLI links fonts and symbols into the native project next to that file. After adding or editing an SVG, regenerate and relink, then rebuild native once:

```sh
cd apps/fabric-example
node ../../node_modules/react-native-nano-icons/lib/commonjs/scripts/cli.js
```

Commit the generated `assets/icons/nanoicons/*`, `ios/nanoicons-fonts/`, `Images.xcassets/nano.*.symbolset`, `android/app/src/main/assets/fonts/` and `res/drawable/nano_*` files. During development the `withNanoIcons` Metro plugin hot-reloads icon-font edits.

## Adding a New Example (Tests tab)

Examples are focused single-feature demos (Oscillator, AudioPlayback, PannerNode...).

### 1. Create the screen

```
apps/common-app/src/examples/MyExample/
├── index.ts          # re-export default
└── MyExample.tsx     # the screen component
```

```ts
// index.ts
export { default } from './MyExample';
```

```tsx
// MyExample.tsx
import React, { FC } from 'react';
import { Container, Button, Slider } from '../../components';
import { useAudioContext } from '../../utils/audio';

const MyExample: FC = () => {
  const audioContext = useAudioContext(); // closed on unmount

  return <Container centered>{/* UI here */}</Container>;
};

export default MyExample;
```

### 2. Register in the index

```ts
// apps/common-app/src/examples/index.ts
import MyExample from './MyExample';

export const Examples = [
  // ... existing
  {
    key: 'MyExample',
    title: 'My Example',
    icon: 'zap', // SVG name in common-app/assets/icons/app/
    category: 'sources', // Tests tab section, see `exampleCategories`
    screen: MyExample,
  },
];
```

Also add `MyExample: undefined;` to `ExampleRoutes` in the same file. Done — the screen appears in the Tests tab automatically.

## Adding a New Demo (Demo Apps tab)

Demos are full mini-apps (PedalBoard, voice memo recorder...).

### 1. Create the screen (same pattern as above)

```
apps/common-app/src/demos/MyDemo/
└── MyDemo.tsx
```

### 2. Register in the index

Add `'MyDemo'` to the `DemoKey` union, then:

```ts
// apps/common-app/src/demos/index.ts
import { colors } from '../styles';
import MyDemo from './MyDemo/MyDemo';

export const demos = [
  // ... existing
  {
    key: 'MyDemo',
    title: 'My Demo',
    subtitle: 'One-line description shown in the list.',
    icon: 'guitar',
    color: colors.yellow, // icon badge colour
    screen: MyDemo,
  },
];
```

A demo that draws its own header sets `headerShown: false` on its entry; `RootStack` passes it to the screen's options. Reference: Player.

## Code Patterns

### Core rule: use `useRef` for audio nodes, `useState` for UI

Audio nodes are C++ objects — they must not be re-created on every render.

```tsx
// ✅ Correct
const gainRef = useRef<GainNode | null>(null);

// ❌ Wrong — re-creates the node on every render
const [gain, setGain] = useState<GainNode | null>(null);
```

### Standard screen skeleton

```tsx
const MyExample: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [volume, setVolume] = useState(1.0);

  const audioContext = useAudioContext();
  const gainRef = useRef<GainNode | null>(null);

  // Real-time parameter update
  const handleVolumeChange = (value: number) => {
    setVolume(value);
    if (gainRef.current) {
      gainRef.current.gain.value = value;
    }
  };

  return (
    <Container centered>
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={() => setIsPlaying((p) => !p)}
      />
      <Slider
        label="Volume"
        min={0}
        max={1}
        step={0.01}
        value={volume}
        onValueChange={handleVolumeChange}
      />
    </Container>
  );
};
```

### Audio lifetime rules

- A screen owns its context through `useAudioContext()` (`utils/audio.ts`). Never create one at module scope (`export default new Player()` opened a context at app launch that was never closed) or in a `useRef(new AudioContext())` initializer (runs every render).
- Load buffers with `useDecodedAudio(ctx, source)`: it drops a decode that finishes after unmount. Hand-rolled loads need the same cancelled flag, or they re-create nodes and notifications for a screen that is gone.
- Disconnect nodes when playback stops, not only on unmount. The engine frees a node by itself only once its JS handle is garbage-collected, it has no inputs and (for a source) it has ended (`AudioGraph::sortAndCompact`, cascading downstream). Until Hermes collects the handles, a finished voice's gain stays connected to the destination and renders every quantum, so every Play or drum hit adds load (finished Piano voices left connected turned simulator playback glitchy after about a minute). Use the no-argument `disconnect()`: it never throws, even when nothing is connected or after `close()`, whereas `disconnect(target)` throws `InvalidAccessError` when that edge is missing. `onended` fires even for a source nothing pulls (`advanceWithoutRender`), so `source.onended = () => gain.disconnect()` is reliable. Clear a source's `onended` before stopping it by hand, or a late `onended` tears down the graph that replaced it.
- `activatePlaybackSession()` is the iOS `playback` category plus `setAudioSessionActivity(true)`.
- The `singletons` recorder and context are shared with the Recorder demo, whose take keeps running in the background. A screen cleans up only what it started (`examples/Record` tracks `ownsRecorderRef`); Latency Validation opens its own context and recorder per run and refuses to start while `AudioRecorder.isRecordingOngoing()`.

### File playback pattern

`utils/AudioPlayer.ts` plays one decoded track at a time on a context the caller owns: `new AudioPlayer(ctx, { onPositionChanged, onEnded })`, released with `dispose()`. `load(asset)` swaps the track and resolves `false` when a later `load` overtook it (a track picked while another still decodes); it also takes a decode the caller already started, which is how the Player plays preloaded tracks. `seekTo(seconds)` jumps within the track. `setOutput(node)` routes it through an effects chain the caller owns instead of straight to the speakers. It's shared by the Audio Playback example and the Player demo. The Audio Playback example also covers playback speed: `setPlaybackRate` changes the live source's `playbackRate`, while `setPitchCorrection` restarts the source from the last reported position, because `pitchCorrection` is fixed when a buffer source is created.

### Music player (Player demo)

`demos/Player/` plays the Crossfade demo's six tracks, listed twice (12 rows, keyed by index since titles repeat), on the app-wide `singletons` context (never closed, so the player lives in `useState`; `AudioPlayer.pause` suspends that shared context). `PlayerSheet` is an inline `@swmansion/react-native-bottom-sheet` with two detents: a floating mini player (side and bottom insets) and `'100%'`; the library interpolates the insets, so the card grows into the full-width player. A `useEvent` worklet on `onPositionChange` (with `wrapNativeView={Animated.createAnimatedComponent}`) turns the fractional detent index into an `expansion` shared value that crossfades the mini and full layers. The queue itself carries the current cover between the mini player and the full player: it is drawn in its own layer at the place the full layout holds for it (measured with `onLayout`) and scaled and moved by `expansion` so that its current cover sits on the mini player's cover spot; the neighbours fade in late (`neighbourRevealStart`). One cover all the way, so nothing is handed over on arrival. The queue follows `expansion` through per-frame exponential smoothing (`coverProgress`, time constant `coverFollowTime`, 60 ms; 30 ms looked no different): the sheet moves its card natively and reports the position in events that don't land in step with the card's frames, and a cover scaled straight from them shook against the card. A follow spring restarted on every event didn't work: it kept its speed when the sheet stopped short and bounced, and with `overshootClamping` it snapped onto the raw position and felt raw. The smoothing trails the card a little, so on a fast open the cover rides up with the card before sliding across. (Moving the title and artist the same way was tried and removed.) A separate moving copy (tried first) jumped where it handed over to the queue, and drifted apart from the neighbours already in place. Corner radii are laid out before that scale, so they are divided by it to stay 8 pt on screen; blending the pre-scale radius between the two ends bulged mid-way. Reanimated's shared element transitions don't fit here: they animate between two screens on a navigation push or pop, on a fixed timeline, while this follows the sheet drag on one screen. The Player decodes through a cache that holds the current track and its left and right neighbours (`preloadOffsets`, wrapping), started as soon as a track is picked (before any await, or a quick second swipe arrived before its track was queued), so a swipe or Previous / Next either way usually plays without waiting; everything else is evicted, and a failed decode is forgotten so it retries. The loading spinner appears only after `spinnerDelayMs` (250 ms): a preloaded track starts within a few frames, and showing the spinner for those flashed it on every swipe. Mood filters (`moodFilters.ts`): one fixed biquad chain on the shared context (highpass → low shelf → peak → high shelf → lowpass → pump gain → trim gain, plus one oscillator that sweeps the peak's frequency and the pump's gain through their own depth gains: Club's auto-wah and sidechain-style pump, the AudioParam-modulation pattern of the Pedal board's auto-wah) that the player outputs into; a mood is only a set of targets for it, glided with `setTargetAtTime` so switching never clicks, and boosting moods carry a trim so they don't clip. The screen disconnects the chain on unmount, since the singleton context outlives it. Picked from a pull-down menu (`@react-native-menu/menu`) in the full player's top bar. Keep the 250 ms playback position out of the sheet's render path: it lives in a subscribable `playbackPosition` source that only the two progress bars read, and `PlayerSheet` is memoized. Held in the screen's state, each report re-rendered the whole sheet, and those commits mid-drag made its animated views jitter. The full player's progress is a seekable `ProgressBar`; while a drag is on it shows the finger's position rather than the 250 ms position updates, and the track seeks on release. The full player covers the whole screen (`topInset={0}`, content padded by the safe area). The native header draws over a screen's content, where no inline sheet can reach, so the Player's registry entry sets `headerShown: false` and the screen draws its own floating glass back button. While the full player is open it turns off `gestureEnabled` and `fullScreenGestureEnabled`, so it closes only by dragging it down, its chevron, or Android's back button. `QueueCarousel` and the blurred backdrop share one `queuePosition` shared value: every cover is blurred once (`demos/Crossfade/CoverBackdrop`) and the layers blend with the position, because blurring a new full-screen image when a track settles stalls that frame. The queue shows the current cover with the previous and next slightly behind it, peeking in at the sides. Don't stack the covers with an animated `zIndex`: it goes through a shadow-tree commit, unlike transforms and opacity, and lagged behind them, so a fast swipe briefly showed the wrong cover on top. Every cover is drawn twice instead, in a back layer and in a touch-transparent front layer where only the cover nearest the centre is opaque. A swipe moves one track, rubber-bands past it and hands its release velocity to a critically damped spring, capped (`maxGlideSpeed`) so a hard flick overshoots by under a fifth of a track. The track is selected when the swipe is released, not when the glide settles (a spring only reports finished once fully at rest, which reads as lag); the covers take no per-track props, so that re-render doesn't touch them mid-glide.

### Pattern-based scheduling (DrumMachine)

Use the shared `usePlayer` hook + a `SoundEngine` class:

```tsx
import usePlayer, { type InstrumentName } from '../../utils/usePlayer';
import Kick from '../../utils/soundEngines/Kick';

// Keep `setup` stable (module scope or useCallback): usePlayer recreates its
// context whenever it changes.
function setupPlayer(ctx: AudioContext) {
  const kick = new Kick(ctx);
  return {
    playNote: (name: InstrumentName, time: number) => kick.play(time),
  };
}

const player = usePlayer({ bpm, patterns, numBeats, setup: setupPlayer });
// player.play() / player.stop() / player.isPlaying
```

`usePlayer` resumes its context before the loop starts: a new `AudioContext` stays suspended (clock frozen at 0) until a source starts, and a scheduler that waits for the clock to approach the first note then never schedules it (the Drum Machine stopped starting on Android). `usePlayer` keeps two clocks. Notes are handed to the audio thread 100 ms ahead of `audioContext.currentTime` (a queue of scheduled steps, not one), because `currentTime` only moves once per output buffer (10–21 ms steps on the iOS simulator, up to ~50 ms on the Android emulator) and a one-frame lookahead misses notes, which then start late. The playhead and hit highlights run on an _audible_ clock: `currentTime` carried forward with wall time between buffer jumps, minus `audioContext.outputLatency` (≈3 ms on the simulator, ~170 ms on the Android emulator), so the spoke touches a dot exactly when its hit is heard.

Use `react-native-background-timer` (not `setTimeout`) for scheduling — it continues running when the screen is locked.

### Long scheduled playback (Simple Piano score)

`examples/Piano/usePianoDevApi.ts` exposes `globalThis.__piano` in dev builds (`play(score)`, `stop`, `focus`, `isReady`) so a JSON score can be driven from the debugger (argent `debugger-evaluate`, or Metro CDP on a physical phone). While a score plays, `ScorePlayback` shows falling notes and slides the keyboard along a precomputed follow track (`followTrack.ts`). Rules for voices:

- Create voices just ahead of time (rolling ~1 s window topped up every 250 ms), not all up front: an 844-note score scheduled at once played silent.
- Disconnect each voice once its scheduled stop time has passed. Finished voices left connected to the destination made simulator playback decay into glitches after about a minute.
- Live key presses follow the same rule through `onended`, damp a still-ringing voice of the same key when it is struck again, and skip the lowpass when its cutoff is above hearing (every live tap is full velocity). On a Samsung A26 debug build, eight ringing voices took ~2.5 ms of the 2.67 ms render budget per 128-frame callback and the stream underran hundreds of times a second, which is heard as buzzing; the core engine compiles unoptimised in debug (only `dsp/` sources get `-O3`).

### Moving AudioParams from an animation loop

Never assign `param.value` once per frame for continuous motion: each assignment is a step, and 60 steps a second are heard as a buzz at the frame rate. Glide with `param.setTargetAtTime(value, ctx.currentTime, ~0.03)` instead. Reference: `examples/PannerNode/PannerNode.tsx` (`applyPose`). Screens that never react to a tap can open their context with `latencyHint: 'playback'` (Oboe power-saving mode, a 20 ms mixer period instead of a 2 ms exclusive stream) for more render headroom. With that hint `currentTime` freezes for whole 20–40 ms buffers, so drive animation from `currentTime` carried forward with wall time between jumps (as `usePlayer` does), not from raw `currentTime` deltas. When the animation also drives the audio (it computes a pose and schedules it at that clock time), schedule at the clock but show what is heard: keep the recent frames and display the newest one at least `outputLatency` plus the glide time constant old (~200 ms with the playback hint on Android); otherwise the picture runs ahead of the sound. Keep per-frame `useState` in a small child component: re-rendering the whole PannerNode screen every frame held the JS thread at ~20 fps on a Samsung A26, against ~50 fps with only the live readouts re-rendering.

### Effects chain (PedalBoard pattern)

Each pedal gets a board slot (`PedalProps` from `demos/PedalBoard/PedalShell.tsx`: `context`, `inputNode`, `outputNode`) and hands its nodes to `usePedalRouting`, which bypasses `input → output` while off, routes through the chain while on, applies makeup gain, and disconnects everything on unmount:

```tsx
export default function MyPedal(props: PedalProps) {
  // useState initializer: the chain must keep its identity across renders.
  const [chain] = useState(() => [props.context.createWaveShaper()] as const);
  const power = usePedalRouting(props, chain, /* makeupGain */ 1.5);
  return (
    <PedalShell name="MY PEDAL" color={colors.green} {...power}>
      …
    </PedalShell>
  );
}
```

Knob values reach their params through `glide()` (`setTargetAtTime`), because a knob reports every drag frame.

### iOS audio session (required on device)

```tsx
import { AudioManager } from 'react-native-audio-api';

AudioManager.setSystemOptions({
  ios: {
    category: 'playback', // or 'playAndRecord' for mic input
    mode: 'default',
    options: [],
  },
});
await AudioManager.setAudioSessionActivity(true);
```

### Recording permission

```tsx
const status = await AudioManager.requestRecordingPermissions();
if (status !== 'Granted') return;
```

### Voice Chat token server

The Voice Chat demo never holds the OpenAI key. `pnpm server` (repository root) runs `apps/fabric-example/server/realtime-token-server.mjs`, which reads `OPENAI_API_KEY` from the git-ignored `apps/fabric-example/.env.local` and serves short-lived secrets on port 8787. The app requests `/token` from the host that served its bundle. A USB-connected Android device loads the bundle from `localhost` through `adb reverse`, so it also needs `adb reverse tcp:8787 tcp:8787`; re-run it after reconnecting the cable.

### Interruption handling

Enable emission with `AudioManager.observeAudioInterruptions(true)`, then listen.

**Playback:** native pause on `began`, but does not resume any audio contexts. The Audio Playback example resumes on `ended` when it had been playing.

```tsx
useEffect(() => {
  const sub = AudioManager.addSystemEventListener('interruption', (event) => {
    if (event.type === 'began') pause();
  });
  return () => sub?.remove();
}, []);
```

**Recording:** native always resumes the engine. Do not `Recorder.pause()` on `began`. iOS has already stopped I/O; the engine is `Interrupted`, not `Paused`. Only `Interrupted` is retried on `ended` / foreground — `Recorder.pause()` would move the engine to `Paused` and disable that retry. JS only freezes UI that still looks live (the Record demo's scrolling waveform). Unfreeze only on `ended` (failed resume does not emit `ended`).

## Shared UI Components

All in `apps/common-app/src/components/`:

| Component                               | Props                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `AppIcon`                               | `name` (typed SVG name), `size`, `color`                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `Container`                             | `centered?`, `disablePadding?` — stack-screen root; content starts below the measured (transparent) header via `useHeaderHeight`                                                                                                                                                                                                                                                                                                                                                                           |
| `Button`                                | `title`, `onPress`, `disabled?`, `minWidth?` (default 100), `testID?` — liquid glass capsule sized to its title; plain `surface` capsule before iOS 26                                                                                                                                                                                                                                                                                                                                                     |
| `Slider`                                | `label?`, `min`, `max`, `step`, `value`, `scale?` (`'linear'` \| `'logarithmic'`, needs `min > 0`), `onValueChange` (called on release, rounded to `step`) — system slider (`@react-native-community/slider`, UISlider with the glass thumb on iOS 26) under a label / live value row; stretches to the parent width, so place it in a column                                                                                                                                                              |
| `Knob`                                  | `label`, `value` (0–1), `onValueChange`, `color?`, `size?` (default 64), `formatValue?` (worklet) — Skia rotary control turned by a vertical pan (160 px = full range); the readout is an animated `TextInput` so dragging does not re-render. It ignores `value` changes while dragging (they echo the gesture a few frames late and springing to them pulled the pointer back). Reference: Pedal Board pedals                                                                                            |
| `RoundButton`                           | `icon` (`AppIconName` or `'stop'`, a filled square the font lacks), `onPress`, `label?` (caption under the circle), `accessibilityLabel?`, `size?` (default 64), `iconSize?`, `color?`, `iconColor?`, `variant?` (`'solid'` glowing fill \| `'outline'` surface circle with tinted glyph \| `'glass'` liquid glass circle, tinted with `color` while `selected`), `selected?`, `loading?`, `disabled?` — every circular icon button. References: Pedal Board transport, Recorder, Voice Chat call controls |
| `GlassSurface`                          | `LiquidGlassView` props + `pressed?`, `fallbackColor?` — dark liquid glass with the `surface` fallback before iOS 26 / on Android. Use it instead of `LiquidGlassView` directly                                                                                                                                                                                                                                                                                                                            |
| `Card`                                  | `View` props — `surface` panel with `layout.cardRadius`, 16 padding, 8 gap                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `ProgressBar`                           | `progress` (0–1), `color?`, `height?`, `style?`, `onSeek?`, `onScrub?` — playback position and level meters. With `onSeek` it gains a thumb and follows a drag or tap (one `minDistance: 0` pan), calling `onScrub` with the progress under the finger and `onSeek` on release; VoiceOver adjusts it in 5% steps. Reference: Player                                                                                                                                                                        |
| `Spacer.Vertical`                       | `size`                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `Switch`                                | `value`, `onValueChange`, `testID?` (no label; render one beside it) — native switch tinted with the accent                                                                                                                                                                                                                                                                                                                                                                                                |
| `Select`                                | `options`, `value`, `onChange`, `testID?` (single choice, no label; the id goes on the trigger, menu items are found by their title) — native pull-down menu (`@react-native-menu/menu`; glass UIMenu on iOS 26)                                                                                                                                                                                                                                                                                           |
| `SegmentedControl`                      | `options` (`{ value, label }[]`), `value`, `onChange`, `color?` (fills the selected segment) — `UISegmentedControl` on iOS via `@react-native-segmented-control/segmented-control` (its pre-Fabric view runs through React Native's interop layer; the package's class typing is cast once in the wrapper), the package's JS rendering on Android                                                                                                                                                          |
| `ControlsSheet`                         | children, `onCoveredHeightChange?` — always-visible liquid glass card floating above the home indicator, sized to its content (`@swmansion/react-native-bottom-sheet` with the single detent `'content'`: no closed position, nothing to drag, no handle). It overlays its parent and passes touches outside the card through. The reported height excludes the bottom safe area, which `Container` already keeps clear                                                                                    |
| `ListSection` / `ListRow` / `IconBadge` | inset grouped list primitives used by the tabs; section header is inset by `layout.cardRadius`                                                                                                                                                                                                                                                                                                                                                                                                             |

`testID` is what E2E tools query: the accessibility identifier on iOS, the view tag on Android. The Audio I/O Playground (`other/AudioIOPlayground/`) sets one on every control.

Colours come from the audiodocs dark palette in `styles.ts` (`surface`/`surfacePressed`/`recess`, `textPrimary`/`textSecondary`/`textMuted`, `accent` + `onAccent`, `track`, `danger`, hues `blue`/`green`/`purple`/`yellow`/`navy`). Content drawn on an `accent` fill uses `onAccent` (navy) for contrast. Don't write hex values that already have a token; the pre-redesign keys (`white`, `main`, `gray`, `backgroundLight`, `border`, `layout.radius`) are gone. Repeated text roles live in `typography` (`caption`, `hint`, `sectionTitle`); spread them into a StyleSheet entry.

Other-tab test screens build on `testComponents/`: `TestLayout` (description, summary badges, current step, controls, scrolling body; no title, the native header has it) and `useRunLog` (mounted guard, current step, live log, runner state).

A tap gesture (`useTapGesture`, Gesture Handler 3) acts in `onActivate`, which fires only for a recognised tap. `onDeactivate` also fires for touches the tap never claimed: the pedals' footswitches once toggled whenever a fader on the same pedal was dragged (they are plain `Pressable`s now, in `demos/PedalBoard/PedalShell.tsx`).

The icon font has no `power` glyph; `PedalShell` draws Lucide's power symbol inline with `react-native-svg` instead of regenerating the font. Add the SVG to `assets/icons/app/` and regenerate when a second screen needs it.

Liquid glass (`LiquidGlassView` from `@callstack/liquid-glass`) stops rendering its material whenever it or any ancestor has an animated (or zero) `opacity`, even at 1. To show and hide a glass surface, switch its native `effect` (`'regular'` / `'none'`) and fade only its contents, or unmount it; animate transforms, not opacity, on wrappers. That is why a disabled glass `RoundButton` dims its glyph instead of its opacity.

## Type Checking

`common-app` resolves `react-native-audio-api` types from the library's built `lib/typescript`, not from `src`. After switching branches, rebuild before checking the app, or `tsc` checks against another branch's API:

```bash
pnpm --filter react-native-audio-api build
pnpm exec tsc --noEmit -p apps/common-app
```

`common-app/tsconfig.json` overrides the root `NodeNext` resolution with `Bundler`, which is how Metro resolves imports. Under `NodeNext`, packages that ship extensionless ESM `.d.ts` re-exports, such as Reanimated, resolve every export to `any`. The root `skipLibCheck` hides those errors, so the only symptom is missing types.

## Metro Config Notes

`fabric-example/metro.config.js` sets `watchFolders: [monorepoRoot, appsRoot]` so Metro hot-reloads changes in `common-app` and `packages/react-native-audio-api` without restarting.

Its `resolveRequest` also resolves `@babel/runtime/*` from the app. The Babel preset injects those helper imports into workspace packages Metro compiles from source (`packages/*`), which sit outside the app's `node_modules` and don't declare `@babel/runtime`; under pnpm's isolated layout they would otherwise fail to resolve. Don't add `@babel/runtime` to the root or to those packages instead.

If you add a new workspace package that the app depends on, add its root to `watchFolders`.
