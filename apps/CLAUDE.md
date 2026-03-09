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
    └── src/
        ├── App.tsx           # Navigation root (Stack + Bottom Tabs)
        ├── examples/         # "Tests" tab — focused feature examples
        │   └── index.ts      # Exports Examples array — register new screens here
        ├── demos/            # "Demo Apps" tab — full mini-apps
        │   └── index.ts      # Exports demos array — register new screens here
        ├── components/       # Shared UI: Container, Button, Slider, Spacer, Switch, Select
        ├── utils/
        │   ├── soundEngines/ # Reusable synth classes (MetronomeSound, Kick, HiHat, Clap)
        │   └── usePlayer.tsx # Hook for pattern-based scheduled playback
        ├── singletons/       # Global audioContext and audioRecorder instances
        └── styles.ts         # Shared colors and layout constants
```

## Navigation

```
Bottom Tabs
├── Tests       → 2-column grid of Examples (simple feature demos)
├── Demo Apps   → Single-column list of Demos (full mini-apps)
└── Other       → Placeholder
```

Each item navigates into a Stack screen. No manual stack registration needed — `examples/index.ts` and `demos/index.ts` drive it automatically.

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
import { icons } from 'lucide-react-native';  // pick any icon

export const Examples = [
  // ... existing
  {
    key: 'MyExample',
    title: 'My Example',
    Icon: icons.Zap,
    screen: MyExample,
  },
];
```

Done — the screen appears in the Tests tab automatically.

## Adding a New Demo (Demo Apps tab)

Demos are full mini-apps (PedalBoard, voice memo recorder...).

### 1. Create the screen (same pattern as above)

```
apps/common-app/src/demos/MyDemo/
└── MyDemo.tsx
```

### 2. Register in the index

```ts
// apps/common-app/src/demos/index.ts
import MyDemo from './MyDemo/MyDemo';

export const demos = [
  // ... existing
  {
    key: 'MyDemo',
    title: 'My Demo',
    subtitle: 'One-line description shown in the list.',
    icon: icons.Guitar,
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

AudioManager.setAudioSessionOptions({
  iosCategory: 'playback',       // or 'playAndRecord' for mic input
  iosMode: 'default',
  iosOptions: [],
});
await AudioManager.setAudioSessionActivity(true);
```

### Recording permission

```tsx
const status = await AudioManager.requestRecordingPermissions();
if (status !== 'Granted') return;
```

### Interruption handling

```tsx
useEffect(() => {
  const sub = AudioManager.addSystemEventListener('interruption', (event) => {
    if (event.type === 'began') pause();
  });
  return () => sub?.remove();
}, []);
```

## Shared UI Components

All in `apps/common-app/src/components/`:

| Component | Props |
|---|---|
| `Container` | `centered?: boolean` — wraps screen in `SafeAreaView` |
| `Button` | `title`, `onPress`, `disabled?` |
| `Slider` | `label`, `min`, `max`, `step`, `value`, `onValueChange` |
| `VerticalSlider` | same as Slider, vertical orientation |
| `Spacer.Vertical` | `size` |
| `Switch` | `label`, `value`, `onValueChange` |
| `Select` | `label`, `options`, `value`, `onValueChange` |

## Metro Config Notes

`fabric-example/metro.config.js` sets `watchFolders: [monorepoRoot, appsRoot]` so Metro hot-reloads changes in `common-app` and `packages/react-native-audio-api` without restarting.

If you add a new workspace package that the app depends on, add its root to `watchFolders`.
