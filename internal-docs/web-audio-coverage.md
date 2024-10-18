# Web Audio API coverage

## Completed Features (**5** out of 33 features)
![](https://geps.dev/progress/15?dangerColor=006600&warningColor=006600&successColor=006600)

- **AudioScheduledSourceNode**
- **AudioDestinationNode**
- **GainNode**
- **StereoPannerNode**
- **AudioNode**

## In Progress (**7** out of 33 features)
![](https://geps.dev/progress/21?dangerColor=ff9900&warningColor=ff9900&successColor=ff9900)

<details>
  <summary><b>AudioContext</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹 baseLatency                  |   ❌  |
| 🔹 outputLatency                |   ❌  |
| 🔹 sinkId                       |   ❌  |
| 🔘 close                        |   ✅  |
| 🔘 createMediaElementSource     |   ❌  |
| 🔘 createMediaStreamSource      |   ❌  |
| 🔘 createMediaStreamTrackSource |   ❌  |
| 🔘 resume                       |   ❌  |
| 🔘 setSinkId                    |   ❌  |
| 🔘 suspend                      |   ❌  |

</details>

<details>
  <summary><b>AudioBuffer</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹sampleRate                    |   ✅  |
| 🔹length                        |   ✅  |
| 🔹duration                      |   ✅  |
| 🔹numberOfChannels              |   ✅  |
| 🔘getChannelData                |   ✅  |
| 🔘getChannelData                |   ✅  |
| 🔘setChannelData                |   ✅  |
| 🔘copyFromChannel               |   ❌  |
| 🔘copyToChannel                 |   ❌  |

</details>

<details>
  <summary><b>AudioBufferSourceNode</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹buffer                        |   ✅  |
| 🔹detune                        |   ❌  |
| 🔹loop                          |   ✅  |
| 🔹loopStart                     |   ❌  |
| 🔹loopEnd                       |   ❌  |
| 🔹playBackRate                  |   ❌  |
| 🔘start(overridden)             |   ❌  |

</details>

<details>
  <summary><b>AudioParam</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹value                         |   ✅  |
| 🔹defaultValue                  |   ✅  |
| 🔹minValue                      |   ✅  |
| 🔹maxValue                      |   ✅  |
| 🔘setValueAtTime                |   ✅  |
| 🔘linearRampToValueAtTime       |   ✅  |
| 🔘setTargetAtTime               |   ❌  |
| 🔘setValueCurveAtTime           |   ❌  |
| 🔘cancelScheduledValues         |   ❌  |
| 🔘cancelAndHoldAtTime           |   ❌  |

</details>

<details>
  <summary><b>BaseAudioContext</b></summary>

|    Property 🔹/ Method 🔘   | state |
| --------------------------- | ----- |
| 🔹 audioWorklet             |   ❌  |
| 🔹 currentTime              |   ✅  |
| 🔹 destination              |   ✅  |
| 🔹 listener                 |   ❌  |
| 🔹 sampleRate               |   ✅  |
| 🔹 state                    |   ✅  |
| 🔘 createAnalyser           |   ❌  |
| 🔘 createBiquadFilter       |   ✅  |
| 🔘 createBuffer             |   ✅  |
| 🔘 createBufferSource       |   ✅  |
| 🔘 createConstantSource     |   ❌  |
| 🔘 createChannelMerger      |   ❌  |
| 🔘 createChannelSplitter    |   ❌  |
| 🔘 createConvolver          |   ❌  |
| 🔘 createDelay              |   ❌  |
| 🔘 createDynamicsCompressor |   ❌  |
| 🔘 createGain               |   ✅  |
| 🔘 createIIRFilter          |   ❌  |
| 🔘 createOscillator         |   ✅  |
| 🔘 createPanner             |   ❌  |
| 🔘 createPeriodicWave       |   ❌  |
| 🔘 createStereoPanner       |   ✅  |
| 🔘 createWaveShaper         |   ❌  |
| 🔘 decodeAudioData          |   ❌  |

</details>

<details>
  <summary><b>OscillatorNode</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹frequency                     |   ✅  |
| 🔹detune                        |   ✅  |
| 🔹type                          |   ✅  |
| 🔘setPeriodicWave               |   ❌  |

</details>

<details>
  <summary><b>BiquadFilterNodee</b></summary>

|      Property 🔹/ Method 🔘     | state |
| ------------------------------- | ----- |
| 🔹frequency                     |   ✅  |
| 🔹detune                        |   ✅  |
| 🔹Q                             |   ✅  |
| 🔹gain                          |   ✅  |
| 🔹type                          |   ✅  |
| 🔘getFrequencyRespons           |   ❌  |

</details>


## Planned Features (**21** out of 33 features)

![](https://geps.dev/progress/64?dangerColor=800000&warningColor=800000&successColor=800000)

- **AudioParamMap**
- **MediaElementAudioSourceNode**
- **MediaStremAudioSourceNode**
- **MediaStremTrackAudioSourceNode**
- **MediaElementAudioSourceNode**
- **ConvolverNode**
- **DelayNode**
- **DynamicsCompressorNode**
- **WaveShaperNode**
- **PeriodicWave**
- **IRRFilterNode**
- **MediaStreamAudioDestinationNode**
- **AnalyserNode**
- **ChannelSplitterNode**
- **MergerNode**
- **AudioListener**
- **PannerNode**
- **AudioWorkletNode**
- **AudioWorkletProcessor**
- **AudioWorkletGlobalScope**
- **OfflineAudioContext**
