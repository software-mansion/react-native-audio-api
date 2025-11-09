<img src="./assets/react-native-audio-api-gh-cover.png?v0.0.1" alt="React Native Audio API" width="100%">

### High-performance audio engine for React Native based on web audio api specification

[![NPM latest](https://img.shields.io/npm/v/react-native-audio-api/latest)](https://www.npmjs.com/package/react-native-audio-api)
[![NPM nightly](https://img.shields.io/npm/v/react-native-audio-api/audio-api-nightly)](https://www.npmjs.com/package/react-native-audio-api?activeTab=versions)
<br />
[![github ci](https://github.com/software-mansion/react-native-audio-api/actions/workflows/ci.yml/badge.svg)](https://github.com/software-mansion/react-native-audio-api/actions/workflows/ci.yml)
[![Audio Api tests](https://github.com/software-mansion/react-native-audio-api/actions/workflows/tests.yml/badge.svg)](https://github.com/software-mansion/react-native-audio-api/actions/workflows/tests.yml)

`react-native-audio-api` provides system for controlling audio in React Native environment compatible with Web Audio API specification,
allowing developers to generate and modify audio in exact same way it is possible in browsers.

## Installation

check out the [Getting Started](https://docs.swmansion.com/react-native-audio-api/docs/fundamentals/getting-started) section of our documentation for detailed instructions!

## Roadmap

### Planned

### <img src="https://img.shields.io/badge/Coming_in-x.x.x-orange" />

- **DynamicCompressorNode 〽️**<br />
  Reduce the volume of loud sounds and boost quieter nodes to balance the audio signal, avoid clipping or distorted sounds

- **Audio tag 🏷️**<br />
  Simple ability to play and buffer audio, with all of the most commonly used functions, same as on the web, without the need to create and manipulate an audio graph.

- **MIDI support 🎸**<br />
  Complementary lib for react-native-audio-api, that will allow to communicate with MIDI devices or read/write MIDI files.

- **Spatial Audio 🛢️**<br />
  manipulate audio in 3D space

- **Noise Cancellation 🦇**<br />
  System-based active noise and echo cancellation support

### <a href="https://github.com/software-mansion/react-native-audio-api/releases/tag/0.10.0"><img src="https://img.shields.io/badge/Released_in-0.10.0-green" /></a>

- **Decoding and utility modules 🔧**<br />
  Decode and modify audio data without the need to create AudioContext first through a set of utility classes

- **Convolver Node 🐐**<br />
  Realistic reverb and spatial effects in the browser by applying impulse responses. It makes audio sound like it’s being played in real spaces, from small rooms to cathedrals, or through hardware like amps and speakers

- **JS Audio Worklets V2 🐎**<br />
  Customize the process pipeline with JS functions running on audio thread.

### <a href="https://github.com/software-mansion/react-native-audio-api/releases/tag/0.9.0"><img src="https://img.shields.io/badge/Released_in-0.9.0-green" /></a>

- **JS Audio Worklets V1 🐎**<br />
  Receive events and data callbacks from audio thread to synchronize with UI on UI thread.

### <a href="https://github.com/software-mansion/react-native-audio-api/releases/tag/0.8.0"><img src="https://img.shields.io/badge/Released_in-0.8.0-green" /></a>

- **Decoding support for m4a/mp4/aac/ogg/opus 📁** <br />
  Decode m4a/mp4/aac/ogg/opus audio files, allowing for playback of these formats in the audio graph.

- **HLS streaming support** 🌊 <br />
  Stream audio from HLS sources, allowing for playback of live audio streams or pre-recorded audio files.

### <a href="https://github.com/software-mansion/react-native-audio-api/releases/tag/0.7.0"><img src="https://img.shields.io/badge/Released_in-0.7.0-green" /></a>

- **Microphone support 🎙️** <br />
  Grab audio data from device microphone or connected device, connect it to the audio graph or stream through the internet.

- **Custom Audio Processor ⚙️** <br />
  Write your own processing AudioNode.

See more in [Changelog](./ghdocs/changelog.md)

## Web Audio API Specification Coverage

Our current coverage of Web Audio API specification can be found here: [Web Audio API coverage](https://docs.swmansion.com/react-native-audio-api/docs/other/web-audio-api-coverage).

## Examples

The source code for the example application is under the [`/apps/common-app`](./apps/common-app/) directory. If you want to play with the API but don't feel like trying it on a real app, you can run the example project. Check [Example README](./apps/fabric-example/README.md) for installation instructions.

## Your feedback

We are open to new ideas and general feedback. If you want to share your opinion about `react-native-audio-api` or have some thoughts about how it could be further developed, don't hesitate to create an issue or contact the maintainers directly.

## License

react-native-audio-api library is licensed under [The MIT License](./LICENSE). Some of the source code uses implementation directly copied from Webkit and utilizes FFmpeg binaries. Copyrights are held by respective organizations, check [THIRD_PARTY](./THIRD_PARTY.md) file for further details.

## Credits

This project has been built and is maintained thanks to the support from [Software Mansion](https://swmansion.com)

[![swm](https://logo.swmansion.com/logo?color=white&variant=desktop&width=150&tag=react-native-reanimated-github 'Software Mansion')](https://swmansion.com)

## Community Discord

<a href="https://discord.swmansion.com" target="_blank" rel="noopener noreferrer">Join the Software Mansion Community Discord</a> to chat about React Native Audio API or other Software Mansion libraries.

## react-native-audio-api is created by Software Mansion

Since 2012 [Software Mansion](https://swmansion.com) is a software agency with experience in building web and mobile apps. We are Core React Native Contributors and experts in dealing with all kinds of React Native issues. We can help you build your next dream product – [Hire us](https://swmansion.com/contact/projects?utm_source=audioapi&utm_medium=readme).
