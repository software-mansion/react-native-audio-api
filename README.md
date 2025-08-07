<img src="./assets/react-native-audio-api-gh-cover.png?v0.0.1" alt="React Native Audio API" width="100%">

### High-performance audio engine for React Native based on web audio api specification

[![NPM latest](https://img.shields.io/npm/v/react-native-audio-api/latest)](https://www.npmjs.com/package/react-native-audio-api)
[![NPM nightly](https://img.shields.io/npm/v/react-native-audio-api/audio-api-nightly)](https://www.npmjs.com/package/react-native-audio-api?activeTab=versions)
[![github ci](https://github.com/software-mansion/react-native-audio-api/actions/workflows/ci.yml/badge.svg)](https://github.com/software-mansion/react-native-audio-api/actions/workflows/ci.yml)
[![NPM Audio Api publish nightly](https://github.com/software-mansion/react-native-audio-api/actions/workflows/npm-publish-nightly.yml/badge.svg)](https://github.com/software-mansion/react-native-audio-api/actions/workflows/npm-publish-nightly.yml)
[![Audio Api tests](https://github.com/software-mansion/react-native-audio-api/actions/workflows/tests.yml/badge.svg)](https://github.com/software-mansion/react-native-audio-api/actions/workflows/tests.yml)

`react-native-audio-api` provides system for controlling audio in React Native environment compatible with Web Audio API specification,
allowing developers to generate and modify audio in exact same way it is possible in browsers.

## Installation

check out the [Getting Started](https://docs.swmansion.com/react-native-audio-api/docs/fundamentals/getting-started) section of our documentation for detailed instructions!

## Roadmap

- <sub>[![Released in 0.1.0](https://img.shields.io/badge/Released_in-0.1.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.1.0)</sub>  <br />
**Sound synthesis and system audio** 🐣 <br />
  Access to devices audio engines and threads, basic nodes for sound synthesis, simple effects and audio graph implementation
  <br />

- <sub>[![Released in 0.2.0](https://img.shields.io/badge/Released_in-0.2.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.2.0)</sub>  <br />
**Graph Processing** 🛎️ <br />
  Support for multi-channel audio processing, audio-graph route optimizations, improved react-native layer for managing audio nodes
  <br />

- <sub>[![Released in 0.3.2](https://img.shields.io/badge/Released_in-0.3.2-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.3.2)</sub>  <br />
  **Audio Files** 🎸 <br />
    Support for local and remote audio file resources: MP3, WAV, FLAC.
    <br />

- <sub>[![Released in 0.4.0](https://img.shields.io/badge/Released_in-0.4.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.4.0)</sub>  <br />
  **Analyser Node** 🌊 <br />
    Ability to draw, animate or simply debug audio data in time or frequency domain.
    <br />

- <sub>[![Released in 0.5.0](https://img.shields.io/badge/Released_in-0.5.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.5.0)</sub>  <br />
  **Pitch correction, extending the web** 💥 <br />
    Ability to modify playback speed without affecting pitch of the sound
    <br />

- <sub>[![Released in 0.6.0](https://img.shields.io/badge/Released_in-0.6.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.6.0)</sub> <br />
  **System configuration** 🛠️ <br />
    Full control of system audio settings, remote controls, lock screen integration and most importantly configurable background modes

  **Connect audio param** 🤞  <br />
    Ability to connect Audio nodes to audio params, which will allow for powerful and efficient modulation of audio parameters, creating effects like tremolo, vibrato or complex envelope followers. <br />

- <sub>[![Released in 0.7.0](https://img.shields.io/badge/Released_in-0.7.0-green)](https://github.com/software-mansion/react-native-audio-api/releases/tag/0.7.0)</sub> <br />
  **Microphone support** 🎙️ <br />
    Grab audio data from device microphone or connected device, connect it to the audio graph or stream through the internet <br />

  **Custom Audio Processor** 🎙️ <br />
    Write your own processing AudioNode <br />

- **Decoding support for m4a/mp4/aac/ogg/opus** 🐎 <br />
  Ability to decode m4a/mp4/aac/ogg/opus audio files, allowing for playback of these formats in the audio graph.
  <br />

- **HLS streaming support** 🐎 <br />
  Ability to stream audio from HLS sources, allowing for playback of live audio streams or pre-recorded audio files.
  <br />

- **JS Audio Worklets** 🐎 <br />
  Ability to run JS functions connected to the audio graph running on audio thread allowing for full customization of what happens to the audio signal.
  <br />

## Web Audio API Specification Coverage

Our current coverage of Web Audio API specification can be found here: [Web Audio API coverage](https://docs.swmansion.com/react-native-audio-api/docs/other/web-audio-api-coverage).

## Examples

The source code for the example application is under the [`/apps/common-app`](./apps/common-app/) directory. If you want to play with the API but don't feel like trying it on a real app, you can run the example project. Check [Example README](./apps/fabric-example/README.md) for installation instructions.

## Your feedback

We are open to new ideas and general feedback. If you want to share your opinion about `react-native-audio-api` or have some thoughts about how it could be further developed, don't hesitate to create an issue or contact the maintainers directly.

## License

react-native-audio-api library is licensed under [The MIT License](./LICENSE). Some of the source code uses implementation directly copied from Webkit and copyrights are held by respective organizations, check [COPYING](./COPYING) file for further details

## Credits

This project has been built and is maintained thanks to the support from [Software Mansion](https://swmansion.com)

[![swm](https://logo.swmansion.com/logo?color=white&variant=desktop&width=150&tag=react-native-reanimated-github 'Software Mansion')](https://swmansion.com)

## Community Discord

<a href="https://discord.swmansion.com" target="_blank" rel="noopener noreferrer">Join the Software Mansion Community Discord</a> to chat about React Native Audio API or other Software Mansion libraries.

## react-native-audio-api is created by Software Mansion

Since 2012 [Software Mansion](https://swmansion.com) is a software agency with experience in building web and mobile apps. We are Core React Native Contributors and experts in dealing with all kinds of React Native issues. We can help you build your next dream product – [Hire us](https://swmansion.com/contact/projects?utm_source=audioapi&utm_medium=readme).
