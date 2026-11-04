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
    │   ├── components/       # Shared UI: AppIcon, Container, Button, Slider, Spacer, Switch, Select
    │   ├── utils/
    │   │   ├── soundEngines/ # Reusable synth classes (MetronomeSound, Kick, HiHat, Clap)
    │   │   └── usePlayer.tsx # Hook for pattern-based scheduled playback
    │   ├── singletons/       # Global audioContext and audioRecorder instances
    │   └── styles.ts         # Shared colors and layout constants
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

Examples and demos can move their options into an always-visible bottom sheet: set `controls` on the `Examples` / `demos` entry (a component rendered as the sheet content) and call `useControlsSheet('<Key>Controls')` in the screen. `RootStack` registers a `<Key>Controls` route as a non-dismissable `formSheet` (`fitToContents`, nothing dimmed, transparent content so the native glass shows), and `ControlsSheetLayout` pops the sheet once its owner screen leaves the stack — a native back press only pops the owner. The screen and its sheet are separate routes, so they share state through a small `useSyncExternalStore` store (see `examples/DrumMachine/drumMachineStore.ts`). References: Drum Machine; Recorder, where the screen keeps the recorder and publishes its state and handlers to the sheet through `demos/Record/recorderSheetStore.ts`.

On iOS 26 react-native-screens enables the system's content-wide back gesture by default (`fullScreenSwipeEnabled` → `interactiveContentPopGestureRecognizer`), so a rightward pan anywhere on a pushed screen can pop it. A screen with its own horizontal swipes sets `navigation.setOptions({ fullScreenGestureEnabled: false })`, which leaves the edge swipe and back button. Reference: `demos/Crossfade/Crossfade.tsx`.

Native tabs cannot render React elements as icons: `tabBarIcon` returns `nativeNanoSymbol('<svg name>')` from a symbol set (see Icons).

## Icons

Icons are [react-native-nano-icons](https://nano-icons.swmansion.com/): SVGs compiled into a font at build time. Sources are vendored Lucide SVGs (`assets/icons/LICENSE-lucide`). `LICENSE-lucide` names no version; `mic-off` and `x` came from `lucide-static` 1.68.0 (`https://unpkg.com/lucide-static@<version>/icons/<name>.svg`), so take new ones from that release or later.

| Folder | Set | Rendered with |
|---|---|---|
| `common-app/assets/icons/app/` | `iconSets` → `app.ttf` | `<AppIcon name="mic" size={24} color={...} />` (name typed from the glyphmap) |
| `common-app/assets/icons/tabs/` | `symbolSets` → SF Symbol `.symbolset` / Android VectorDrawable | `nativeNanoSymbol('gauge')` in `tabBarIcon` |

The config lives in `fabric-example/.nanoicons.json` because the CLI links fonts and symbols into the native project next to that file. After adding or editing an SVG, regenerate and relink, then rebuild native once:

```sh
cd apps/fabric-example
node ../../node_modules/react-native-nano-icons/lib/commonjs/scripts/cli.js
```

Commit the generated `assets/icons/nanoicons/*`, `ios/nanoicons-fonts/`, `Images.xcassets/nano.*.symbolset`, `android/app/src/main/assets/fonts/` and `res/drawable/nano_*` files. During development the `withNanoIcons` Metro plugin hot-reloads icon-font edits.

## Adding a New Example (Tests tab)

Examples are focused single-feature demos (Oscillator, Metronome, AudioFile...).

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
import React, { useEffect, useRef, FC } from 'react';
import { AudioContext, GainNode } from 'react-native-audio-api';
import { Container, Button, Slider } from '../../components';

const MyExample: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  useEffect(() => {
    audioContextRef.current = new AudioContext();
    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      {/* UI here */}
    </Container>
  );
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

  const audioContextRef = useRef<AudioContext | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  // Initialize audio context once
  useEffect(() => {
    audioContextRef.current = new AudioContext();
    return () => {
      audioContextRef.current?.close();
    };
  }, []);

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
        min={0} max={1} step={0.01}
        value={volume}
        onValueChange={handleVolumeChange}
      />
    </Container>
  );
};
```

### File playback pattern

Use a singleton helper class to avoid managing node lifecycle inside the component:

```tsx
// AudioPlayer.ts
class AudioPlayer {
  private ctx = new AudioContext();
  private buffer: AudioBuffer | null = null;

  async load(asset: string | number) {
    this.buffer = await this.ctx.decodeAudioData(asset);
  }

  play() {
    const src = this.ctx.createBufferSource();
    src.buffer = this.buffer;
    src.connect(this.ctx.destination);
    src.start();
  }
}
export default new AudioPlayer();
```

### Pattern-based scheduling (Metronome, DrumMachine)

Use the shared `usePlayer` hook + a `SoundEngine` class:

```tsx
import usePlayer from '../../utils/usePlayer';
import MetronomeSound from '../../utils/soundEngines/MetronomeSound';

function setupPlayer(ctx: AudioContext) {
  const sound = new MetronomeSound(ctx, 1000);
  return {
    playNote: (name: string, time: number) => sound.play(time),
  };
}

const player = usePlayer({ bpm, patterns, numBeats, setup: setupPlayer });
// player.play() / player.stop() / player.isPlaying
```

Use `react-native-background-timer` (not `setTimeout`) for scheduling — it continues running when the screen is locked.

### Long scheduled playback (Simple Piano score)

`examples/Piano/usePianoDevApi.ts` exposes `globalThis.__piano` in dev builds (`play(score)`, `stop`, `focus`, `isReady`) so a JSON score can be driven from the debugger (argent `debugger-evaluate`, or Metro CDP on a physical phone). While a score plays, `ScorePlayback` shows falling notes and slides the keyboard along a precomputed follow track (`followTrack.ts`). Two rules when scheduling many notes:

- Create voices just ahead of time (rolling ~1 s window topped up every 250 ms), not all up front: an 844-note score scheduled at once played silent.
- Disconnect each voice once its scheduled stop time has passed. Finished voices left connected to the destination made simulator playback decay into glitches after about a minute.

### Effects chain (PedalBoard pattern)

Sub-components receive `inputNode` / `outputNode` and wire their effect in between:

```tsx
// Parent
const inputNode = ctx.createGain();
const outputNode = ctx.createGain();
inputNode.connect(outputNode); // bypass by default

// Sub-component (pedal)
function MyPedal({ context, inputNode, outputNode }: PedalProps) {
  const effectRef = useRef<WaveShaperNode | null>(null);

  const initEffect = () => {
    if (effectRef.current) return;
    const ws = context.createWaveShaper();
    inputNode.disconnect(outputNode); // remove bypass
    inputNode.connect(ws);
    ws.connect(outputNode);
    effectRef.current = ws;
  };
  // ...
}
```

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

### Interruption handling

Enable emission with `AudioManager.observeAudioInterruptions(true)`, then listen.

**Playback:** native pause on `began`, but does not resume any audio contexts. The AudioFile example resumes on `ended` when it had been playing.

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

| Component | Props |
|---|---|
| `AppIcon` | `name` (typed SVG name), `size`, `color` |
| `Container` | `centered?`, `disablePadding?` — stack-screen root; content starts below the measured (transparent) header via `useHeaderHeight` |
| `Button` | `title`, `onPress`, `disabled?`, `minWidth?` (default 100) — liquid glass capsule sized to its title; plain `surface` capsule before iOS 26 |
| `Slider` | `label?`, `min`, `max`, `step`, `value`, `scale?` (`'linear'` \| `'logarithmic'`, needs `min > 0`), `onValueChange` (called on release, rounded to `step`) — system slider (`@react-native-community/slider`, UISlider with the glass thumb on iOS 26) under a label / live value row; stretches to the parent width, so place it in a column |
| `VerticalSlider` | `label`, `value` (0–1), `onValueChange`, `labelColor?`, `valueColor?`, `possibleValues?` (snap points, drawn as ticks) — hardware-style fader (thin slot, square cap) printed on the Pedal Board pedals; only the cap is draggable |
| `Spacer.Vertical` | `size` |
| `Switch` | `value`, `onValueChange` — native switch tinted with the accent |
| `Select` | `options`, `value`, `onChange` — native pull-down menu (`@react-native-menu/menu`; glass UIMenu on iOS 26) |
| `SegmentedControl` | `options` (`{ value, label }[]`), `value`, `onChange` — capsule row for 2–5 options |
| `BottomAccessory` | children — liquid-glass capsule above the home indicator, shaped like the iOS 26 tab bar accessory (the system accessory only exists on bottom-tab screens, and examples are pushed on the root stack). Pad content with `useBottomAccessorySpace()`. Reference: Audio Visualizer |
| `ListSection` / `ListRow` / `IconBadge` | inset grouped list primitives used by the tabs; section header is inset by `layout.cardRadius` |

Colours come from the audiodocs dark palette in `styles.ts` (`surface`, `textPrimary`/`textSecondary`/`textMuted`, `accent` + `onAccent`, `track`, hues `blue`/`green`/`purple`/`yellow`). Content drawn on an `accent` fill uses `onAccent` (navy) for contrast.

A tap gesture (`useTapGesture`, Gesture Handler 3) acts in `onActivate`, which fires only for a recognised tap. `onDeactivate` also fires for touches the tap never claimed: the pedals' footswitches toggled whenever a fader on the same pedal was dragged.

Liquid glass (`LiquidGlassView` from `@callstack/liquid-glass`) stops rendering its material whenever it or any ancestor has an animated (or zero) `opacity`, even at 1. To show and hide a glass surface, switch its native `effect` (`'regular'` / `'none'`) and fade only its contents; animate transforms, not opacity, on wrappers. Reference: `demos/Crossfade/CrossfadeCapsule.tsx`.

`@react-native-menu/menu` 2.0.0 is patched (`.yarn/patches/`) to import `<React/RCTBridge.h>` instead of the quoted `"RCTBridge.h"`, which RN 0.88's header layout no longer resolves.

## Metro Config Notes

`fabric-example/metro.config.js` sets `watchFolders: [monorepoRoot, appsRoot]` so Metro hot-reloads changes in `common-app` and `packages/react-native-audio-api` without restarting.

If you add a new workspace package that the app depends on, add its root to `watchFolders`.
