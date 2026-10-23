This is a new [**React Native**](https://reactnative.dev) project, bootstrapped using [`@react-native-community/cli`](https://github.com/react-native-community/cli).

# Getting Started

## Voice Chat demo

Open **Demo Apps → Voice Chat** for a full-duplex OpenAI Realtime conversation
using the workspace's upstream RNAA source. No local library patches or overlay
are required. Use a physical iPhone or an Android 12+ (API 31+) device when
evaluating acoustic echo cancellation (AEC).

From the repository root, copy the environment template once and set your key:

```sh
cp -n apps/fabric-example/.env.example apps/fabric-example/.env.local
# Edit apps/fabric-example/.env.local: OPENAI_API_KEY=...
```

Start these commands in three terminals, all from the repository root:

```sh
# Terminal 1: local token server (loads .env.local automatically)
yarn server

# Terminal 2: one Metro instance
yarn start --reset-cache

# Terminal 3: build and install on the USB-connected Android device
adb devices
adb -s <device-id> reverse tcp:8081 tcp:8081
adb -s <device-id> reverse tcp:8787 tcp:8787
yarn android --device <device-id>
```

For iOS, install Pods from `apps/fabric-example/ios` with
`bundle exec pod install`, then use `yarn workspace fabric-example ios` or run
the FabricExample scheme on your device from Xcode. The device must reach Metro
and the token server on the development machine; use the same LAN and allow
ports 8081 and 8787 through the local firewall. The app derives the token-server
host from Metro's bundle URL. `curl http://localhost:8787/health` checks the server.

### Implementation

- [VoiceChat.tsx](../common-app/src/demos/VoiceChat/VoiceChat.tsx): existing
  FabricExample styling, connection status, and call controls;
  [components/Transcript.tsx](../common-app/src/demos/VoiceChat/components/Transcript.tsx)
  renders the conversation.
- [hooks/useVoiceChat.ts](../common-app/src/demos/VoiceChat/hooks/useVoiceChat.ts):
  UI state, microphone permission prompting before startup, and cleanup on
  navigation blur, backgrounding, or unmount.
- [voiceSession.ts](../common-app/src/demos/VoiceChat/voiceSession.ts): WebSocket
  events, transcripts, barge-in, and serialized session/interruption lifecycle.
- [audio/](../common-app/src/demos/VoiceChat/audio): one context and recorder,
  platform communication settings, serial queued playback, and 24 kHz PCM encoding.
- [backend/token.ts](../common-app/src/demos/VoiceChat/backend/token.ts): fetches
  the client secret from the development server using Metro's hostname.
- [realtime-token-server.mjs](./server/realtime-token-server.mjs): the only
  process that reads `OPENAI_API_KEY`. It mints a short-lived client secret,
  fixes the model/voice/session policy, and never sends the standard key to the app.

The fixed policy uses `gpt-realtime-2.1-mini`, `marin`, English transcription and
instructions, near-field input noise reduction, and low-eagerness semantic VAD.
The client requests a response after a nonempty transcription and clears playback
immediately when speech starts. Reply requests wait for the current speech turn
to end and any outstanding response to finish; late transcripts from older turns
still appear in the UI but do not trigger replies. An interrupted assistant item
is truncated at the last whole RNAA buffer confirmed rendered; progress is not sample-accurate
and may exclude the audible portion of a partially played buffer.

The recorder requests `androidInputPreset: 'voiceCommunication'` and
`iosVoiceProcessing: true`. Android also uses a `voiceCommunication` output
profile, a balanced output latency hint, and an `inCommunication` session with
initial speaker routing. These settings request platform processing; AEC, noise
suppression, and gain control still depend on the device and route. Test without
headphones at several speaker volumes, with quiet speech, barge-in, background
noise, and in a reflective room. Verify audio focus recovery and route restoration
after ending a call. On iOS, Siri with Bluetooth HFP can leave recording paused
without an interruption-end event; end and reconnect when necessary.
During normal iOS interruption recovery, RNAA retains and resumes the recorder;
the app gates capture callbacks rather than stopping and restarting that recorder.

This server is for trusted local development. Before deploying it, add HTTPS,
authentication, and rate limiting. Audio is sent to OpenAI, so an application
using this example should obtain appropriate user consent. The demo ends calls
in the background and does not implement background-call notifications.

### Checks

```sh
yarn workspace common-app typecheck:voice
yarn workspace fabric-example test --runInBand
yarn workspace fabric-example test:server
```

The server tests mock OpenAI; they do not use an API key or make paid requests.

> **Note**: Make sure you have completed the [Set Up Your Environment](https://reactnative.dev/docs/set-up-your-environment) guide before proceeding.

## Step 1: Start Metro

First, you will need to run **Metro**, the JavaScript build tool for React Native.

To start the Metro dev server, run the following command from the root of your React Native project:

```sh
# Using npm
npm start

# OR using Yarn
yarn start
```

## Step 2: Build and run your app

With Metro running, open a new terminal window/pane from the root of your React Native project, and use one of the following commands to build and run your Android or iOS app:

### Android

```sh
# Using npm
npm run android

# OR using Yarn
yarn android
```

### iOS

For iOS, remember to install CocoaPods dependencies (this only needs to be run on first clone or after updating native deps).

The first time you create a new project, run the Ruby bundler to install CocoaPods itself:

```sh
bundle install
```

Then, and every time you update your native dependencies, run:

```sh
bundle exec pod install
```

For more information, please visit [CocoaPods Getting Started guide](https://guides.cocoapods.org/using/getting-started.html).

```sh
# Using npm
npm run ios

# OR using Yarn
yarn ios
```

## Run iOS Tests

Native unit tests live in `FabricExampleTests` and run inside a minimal host app,
`FabricExampleTestHost`, which links only `RNAudioAPI` and React Native core.

From `apps/fabric-example`, make sure Pods are installed:

```sh
bundle exec pod install
```

Run all native tests (preferred — skips building the example app):

```sh
xcodebuild test \
  -workspace ios/FabricExample.xcworkspace \
  -scheme FabricExampleTests \
  -destination 'platform=iOS Simulator,name=iPhone 17'
```

Run a single test class or method:

```sh
xcodebuild test \
  -workspace ios/FabricExample.xcworkspace \
  -scheme FabricExampleTests \
  -destination 'platform=iOS Simulator,name=iPhone 17' \
  -only-testing:FabricExampleTests/IOSAudioPlayerTests/testRenderBlockRendersInQuantumSizedChunksWhenRunning
```

The `FabricExample` scheme still lists the same tests, but uses the lightweight host
at test time instead of booting Metro / the React Native example app.

If that simulator is not available on your machine, list installed simulators and swap the destination:

```sh
xcrun simctl list devices available
```

Xcode script phases also require a valid `NODE_BINARY`. If Hermes or React Native pod scripts fail before tests start, update `ios/.xcode.env.local` to point at your current `node` executable.

If everything is set up correctly, you should see your new app running in the Android Emulator, iOS Simulator, or your connected device.

This is one way to run your app — you can also build it directly from Android Studio or Xcode.

## Step 3: Modify your app

Now that you have successfully run the app, let's make changes!

Open `App.tsx` in your text editor of choice and make some changes. When you save, your app will automatically update and reflect these changes — this is powered by [Fast Refresh](https://reactnative.dev/docs/fast-refresh).

When you want to forcefully reload, for example to reset the state of your app, you can perform a full reload:

- **Android**: Press the <kbd>R</kbd> key twice or select **"Reload"** from the **Dev Menu**, accessed via <kbd>Ctrl</kbd> + <kbd>M</kbd> (Windows/Linux) or <kbd>Cmd ⌘</kbd> + <kbd>M</kbd> (macOS).
- **iOS**: Press <kbd>R</kbd> in iOS Simulator.

## Congratulations! :tada:

You've successfully run and modified your React Native App. :partying_face:

### Now what?

- If you want to add this new React Native code to an existing application, check out the [Integration guide](https://reactnative.dev/docs/integration-with-existing-apps).
- If you're curious to learn more about React Native, check out the [docs](https://reactnative.dev/docs/getting-started).

# Troubleshooting

If you're having issues getting the above steps to work, see the [Troubleshooting](https://reactnative.dev/docs/troubleshooting) page.

# Learn More

To learn more about React Native, take a look at the following resources:

- [React Native Website](https://reactnative.dev) - learn more about React Native.
- [Getting Started](https://reactnative.dev/docs/environment-setup) - an **overview** of React Native and how setup your environment.
- [Learn the Basics](https://reactnative.dev/docs/getting-started) - a **guided tour** of the React Native **basics**.
- [Blog](https://reactnative.dev/blog) - read the latest official React Native **Blog** posts.
- [`@facebook/react-native`](https://github.com/facebook/react-native) - the Open Source; GitHub **repository** for React Native.
