<img src="./assets/react-native-audio-api-gh-cover.png?v0.0.1" alt="React Native Audio API" width="100%">

### React Native Audio API

`react-native-audio-api` provides system for controlling audio in React Native environment compatible with Web Audio API specification,
allowing developers to generate and modify audio in exact same way it is possible in browsers.

## Installation

1. Install `react-native-audio-api` library

   ```bash
   # using npm
   npm install react-native-audio-api

   # or using yarn
   yarn add react-native-audio-api
   ```

## Usage with expo

`react-native-audio-api` contains native custom code and isn't part of the Expo Go application. In order to be available in expo managed builds, you have to use Expo development build. Simplest way on starting local expo dev builds, is to use:

  ```bash
    # Build native iOS project
    npx expo run:ios
    # Build native Android project
    npx expo run:android
  ```

To learn more about expo development builds, please check out [Development Builds Documentation](https://docs.expo.dev/develop/development-builds/introduction/).

## Documentation

Check out our dedicated documentation page for info about this library, API reference and more:
[https://software-mansion.github.io/react-native-audio-api/](https://software-mansion.github.io/react-native-audio-api/)

You can also check out [MDN Web Docs](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API) for fully detailed description of the audio api.

## Coverage

Our current coverage of Web Audio API specification can be found here: [Web Audio API coverage](https://software-mansion.github.io/react-native-audio-api/other/web-audio-api-coverage).

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
