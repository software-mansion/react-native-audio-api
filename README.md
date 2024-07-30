# React Native Audio Context

## Internal Documentation

[Basic interfaces description](./internal-docs/basic-interfaces.md)

## Installation

```sh
npm install react-native-audio-context
```

## API Coverage

#### Android

- [ ] AudioContext

  properties:
  - [x] destination
  - [x] sampleRate
  - [x] state

  methods:
  - [x] createGain()
  - [x] createOscillator()
  - [x] createStereoPanner()
  - [ ] createBiquadFilter()
  - [ ] getCurrentTime()
  - [ ] close()

- [x] AudioNode

  properties:
  - [x] context
  - [x] numberOfInputs
  - [x] numberOfOutputs

  methods:
  - [x] connect()
  - [x] disconnect()

- [x] AudioScheduledSourceNode

  methods:
  - [x] start(time: number)
  - [x] stop(time: number)

- [x] AudioDestinationNode

- [x] AudioParam

  properties:
  - [x] value
  - [x] defaultValue
  - [x] minValue
  - [x] maxValue

  methods:
  - [x] setValueAtTime(value: number, startTime: number)
  - [x] linearRampToValueAtTime(value: number, endTime: number)
  - [x] exponentialRampToValueAtTime(value: number, endTime: number)

- [x] OscillatorNode

  properties:
  - [x] frequency
  - [x] detune
  - [x] type

- [x] GainNode

  properties:
  - [x] gain

- [x] StereoPannerNode

  properties:
  - [x] pan

- [ ] BiquadFilterNode

  properties:
  - [ ] frequency
  - [ ] detune
  - [ ] Q
  - [ ] gain
  - [ ] type

#### IOS

- [ ] AudioContext

  properties:
  - [ ] destination
  - [ ] sampleRate
  - [ ] state

  methods:
  - [ ] createGain()
  - [ ] createOscillator()
  - [ ] createStereoPanner()
  - [ ] createBiquadFilter()
  - [ ] getCurrentTime()
  - [ ] close()

- [ ] AudioNode

  properties:
  - [ ] context
  - [ ] numberOfInputs
  - [ ] numberOfOutputs

  methods:
  - [ ] connect()
  - [ ] disconnect()

- [ ] AudioScheduledSourceNode

  methods:
  - [ ] start(time: number)
  - [ ] stop(time: number)

- [ ] AudioDestinationNode

- [ ] AudioParam

  properties:
  - [ ] value
  - [ ] defaultValue
  - [ ] minValue
  - [ ] maxValue

  methods:
  - [ ] setValueAtTime(value: number, startTime: number)
  - [ ] linearRampToValueAtTime(value: number, endTime: number)
  - [ ] exponentialRampToValueAtTime(value: number, endTime: number)

- [ ] OscillatorNode

  properties:
  - [ ] frequency
  - [ ] detune
  - [ ] type

- [ ] GainNode

  properties:
  - [ ] gain

- [ ] StereoPannerNode

  properties:
  - [ ] pan

- [ ] BiquadFilterNode

  properties:
  - [ ] frequency
  - [ ] detune
  - [ ] Q
  - [ ] gain
  - [ ] type

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
