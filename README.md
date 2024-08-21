# React Native Audio Context

## Internal Documentation

[Basic interfaces description](./internal-docs/basic-interfaces.md)

## Installation

```sh
npm install react-native-audio-context
```

## WebAudio API roadmap and coverage

<details>
  <summary><b>AudioContext</b><br/> Android: ✅ <br/>iOS: ⏳</summary>

| Method / Property     | iOS | Android |
| --------------------- | --- | ------- |
| 🔹 destination        | ✅  | ✅      |
| 🔹 sampleRate         | ✅  | ✅      |
| 🔹 state              | ✅  | ✅      |
| 🔹 currentTime        | ✅  | ✅      |
| 🔘 createGain         | ✅  | ✅      |
| 🔘 createOscillator   | ✅  | ✅      |
| 🔘 createStereoPanner | ✅  | ✅      |
| 🔘 createBiquadFilter | ⏳  | ✅      |
| 🔘 close              | ✅  | ✅      |

</details>

<details>
  <summary><b>AudioNode</b><br/> Android: ✅ <br />iOS: ✅</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔹context         | ✅  | ✅      |
| 🔹numberOfInputs  | ✅  | ✅      |
| 🔹numberOfOutputs | ✅  | ✅      |
| 🔘connect         | ✅  | ✅      |
| 🔘disconnect      | ✅  | ✅      |

</details>

<details>
  <summary><b>AudioScheduledSourceNode</b><br/> Android: ✅ <br />iOS: ✅</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔘start           | ✅  | ✅      |
| 🔘stop            | ✅  | ✅      |

</details>

<details>
  <summary><b>AudioDestinationNode</b><br/> Android: ✅ <br />iOS: ⏳</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |

</details>

<details>
  <summary><b>AudioParam</b><br/> Android: ✅ <br />iOS: ✅</summary>

| Method / Property              | iOS | Android |
| ------------------------------ | --- | ------- |
| 🔹value                        | ✅  | ✅      |
| 🔹defaultValue                 | ✅  | ✅      |
| 🔹minValue                     | ✅  | ✅      |
| 🔹maxValue                     | ✅  | ✅      |
| 🔘setValueAtTime               | ✅  | ✅      |
| 🔘linearRampToValueAtTime      | ✅  | ✅      |
| 🔘exponentialRampToValueAtTime | ✅  | ✅      |

</details>

<details>
  <summary><b>OscillatorNode</b><br/> Android: ✅ <br />iOS: ✅</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔹frequency       | ✅  | ✅      |
| 🔹detune          | ✅  | ✅      |
| 🔹type            | ✅  | ✅      |

</details>

<details>
  <summary><b>GainNode</b><br/> Android: ✅ <br />iOS: ✅</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔹gain            | ✅  | ✅      |

</details>

<details>
  <summary><b>StereoPannerNode</b><br/> Android: ✅ <br />iOS: ⏳</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔹pan             | ⏳  | ✅      |

</details>

<details>
  <summary><b>BiquadFilterNode</b><br/> Android: ✅ <br />iOS: ⏳</summary>

| Method / Property | iOS | Android |
| ----------------- | --- | ------- |
| 🔹frequency       | ⏳  | ✅      |
| 🔹detune          | ⏳  | ✅      |
| 🔹Q               | ⏳  | ✅      |
| 🔹gain            | ⏳  | ✅      |
| 🔹type            | ⏳  | ✅      |

</details>

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
