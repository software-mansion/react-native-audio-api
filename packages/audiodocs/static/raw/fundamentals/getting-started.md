# Getting started

The goal of *Fundamentals* is to guide you through the setup process of the Audio API, as well as to show the basic concepts behind audio programming using a web audio framework, giving you the confidence to explore more advanced use cases on your own. This section is packed with interactive examples, code snippets, and explanations. Are you ready? Let's make some noise!

## Installation

It takes only a few steps to add Audio API to your project:

### Step 1: Install the package

Install the `react-native-audio-api` package from npm:

```sh
npx expo install react-native-audio-api
```

```sh
npm install react-native-audio-api
```

```sh
yarn add react-native-audio-api
```

### Step 2: Add Audio API expo plugin (optional)

Add `react-native-audio-api` expo plugin to your `app.json` or `app.config.js`.

app.json

```javascript
{
  "plugins": [
    [
      "react-native-audio-api",
      {
        "iosBackgroundMode": true,
        "iosMicrophonePermission": "This app requires access to the microphone to record audio.",
        "androidPermissions" : [
          "android.permission.MODIFY_AUDIO_SETTINGS",
          "android.permission.FOREGROUND_SERVICE",
          "android.permission.FOREGROUND_SERVICE_MEDIA_PLAYBACK"
        ],
        "androidForegroundService": true,
        "androidFSTypes": [
          "mediaPlayback"
        ]
      }
    ]
  ]
}
```

app.config.js

```javascript
export default {
  ...
  "plugins": [
    [
      "react-native-audio-api",
      {
        "iosBackgroundMode": true,
        "iosMicrophonePermission": "This app requires access to the microphone to record audio.",
        "androidPermissions" : [
          "android.permission.MODIFY_AUDIO_SETTINGS",
          "android.permission.FOREGROUND_SERVICE",
          "android.permission.FOREGROUND_SERVICE_MEDIA_PLAYBACK"
        ],
        "androidForegroundService": true,
        "androidFSTypes": [
          "mediaPlayback"
        ]
      }
    ]
  ]
};
```

#### Special permissions

If you plan to use [`AudioRecorder`](/docs/inputs/audio-recorder) entry `iosMicrophonePermission` and `android.permission.RECORD_AUDIO` in `androidPermissions` section is **MANDATORY**.

> **Info**
>
> If your app is not managed by expo, see [non-expo-permissions page](/docs/other/non-expo-permissions) how to handle permissions.

Read more about plugin [here](/docs/other/audio-api-plugin)!

### Step 3: Install system-wide bash (only Windows OS)

There are many ways to do that f.e. using git bash. To make sure just test if any unix command works.

```bash
bash -c 'echo Hello World!'
```

### Possible additional dependencies

If you plan to use any of [`WorkletNode`](/docs/worklets/worklet-node), [`WorkletSourceNode`](/docs/worklets/worklet-source-node), [`WorkletProcessingNode`](/docs/worklets/worklet-processing-node), it is required to have
`react-native-worklets` library set up with version 0.6.0 or higher. See [worklets getting-started page](https://docs.swmansion.com/react-native-worklets/docs/) for info how to do it.

> **Info**
>
> If you are not planning to use any of mentioned nodes, `react-native-worklets` dependency is **OPTIONAL** and your app will build successfully without them.

### Usage with expo

`react-native-audio-api` contains native custom code and isn't part of the Expo Go application. In order to be available in expo managed builds, you have to use Expo development build. Simplest way on starting local expo dev builds, is to use:

```sh
npx expo run:ios
```

```sh
npx expo run:android
```

To learn more about expo development builds, please check out [Development Builds Documentation](https://docs.expo.dev/develop/development-builds/introduction/).

#### Android

No further steps are necessary.

#### iOS

While developing for iOS, make sure to install [pods](https://cocoapods.org) first before running the app:

```sh
cd ios && pod install && cd ..
```

#### Web

No further steps are necessary.

> **Caution**
>
> `react-native-audio-api` on the web exposes the browser's built-in Web Audio API, but for compatibility between platforms, it limits the available interfaces to APIs that are implemented on iOS and Android.

### Clear Metro bundler cache (recommended)

```sh
npx expo start -c
```

```sh
npm start -- --reset-cache
```

```sh
yarn start --reset-cache
```

## What's next?

In [the next section](/docs/guides/lets-make-some-noise), we will learn how to prepare Audio API and to play some sound!.
