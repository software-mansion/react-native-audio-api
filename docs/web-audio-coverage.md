# Web Audio API coverage

List below shows which interfaces has been completed, are in progress or not yet started. Status of each interface is based on availability of each method and property that should be available to the API user. Progress on internal features and implementation that are not part of user-facing apis, are not yet covered here.

Some of the noticeable implementation details that are still in progress or not yet started are:

- Support of different number of channels (current approach in most of the audio-graph nodes assumes working with two channel audio)
- Multi-input for each node and input mixing (Although specification suggests that most of the nodes can cave only one input or output, common use-cases proves otherwise). Only node that mixes multiple inputs is `DestinationNode`.

## ✅ Completed (**10** out of 32)

<details>
  <summary><b>AudioBuffer</b></summary>
</details>
<details>
 <summary><b>AudioDestinationNode</b></summary>
</details>
<details>
 <summary><b>AudioNode</b></summary>
</details>
<details>
 <summary><b>AudioScheduledSourceNode</b></summary>
</details>
<details>
  <summary><b>BiquadFilterNode</b></summary>
</details>
<details>
 <summary><b>GainNode</b></summary>
</details>
<details>
 <summary><b>OscillatorNode</b></summary>
</details>
<details>
 <summary><b>PeriodicWave</b></summary>
</details>
<details>
 <summary><b>StereoPannerNode</b></summary>
</details>
<details>
 <summary><b>AudioParam</b></summary>
</details>

## 🚧 In Progress (**3** out of 32)

<details>
  <summary><b>AudioContext</b></summary>

<div style="padding: 16px; padding-left: 42px;">

| Property 🔹/ Method 🔘          | state |
| ------------------------------- | ----- |
| 🔹 baseLatency                  | ❌    |
| 🔹 outputLatency                | ❌    |
| 🔹 sinkId                       | ❌    |
| 🔘 close                        | ✅    |
| 🔘 createMediaElementSource     | ❌    |
| 🔘 createMediaStreamSource      | ❌    |
| 🔘 createMediaStreamTrackSource | ❌    |
| 🔘 resume                       | ❌    |
| 🔘 setSinkId                    | ❌    |
| 🔘 suspend                      | ❌    |

</div>

</details>

<details>
  <summary><b>AudioBufferSourceNode</b></summary>

<div style="padding: 16px; padding-left: 42px;">

| Property 🔹/ Method 🔘 | state |
| ---------------------- | ----- |
| 🔹 buffer              | ✅    |
| 🔹 detune              | ✅    |
| 🔹 loop                | ✅    |
| 🔹 loopStart           | ✅    |
| 🔹 loopEnd             | ✅    |
| 🔹 playBackRate        | ✅    |
| 🔘 start(overridden)   | ❌    |

</div>

</details>

<details>
  <summary><b>BaseAudioContext</b></summary>

<div style="padding: 16px; padding-left: 42px;">

| Property 🔹/ Method 🔘      | state |
| --------------------------- | ----- |
| 🔹 audioWorklet             | ❌    |
| 🔹 currentTime              | ✅    |
| 🔹 destination              | ✅    |
| 🔹 listener                 | ❌    |
| 🔹 sampleRate               | ✅    |
| 🔹 state                    | ✅    |
| 🔘 createAnalyser           | ❌    |
| 🔘 createBiquadFilter       | ✅    |
| 🔘 createBuffer             | ✅    |
| 🔘 createBufferSource       | ✅    |
| 🔘 createConstantSource     | ❌    |
| 🔘 createChannelMerger      | ❌    |
| 🔘 createChannelSplitter    | ❌    |
| 🔘 createConvolver          | ❌    |
| 🔘 createDelay              | ❌    |
| 🔘 createDynamicsCompressor | ❌    |
| 🔘 createGain               | ✅    |
| 🔘 createIIRFilter          | ❌    |
| 🔘 createOscillator         | ✅    |
| 🔘 createPanner             | ❌    |
| 🔘 createPeriodicWave       | ✅    |
| 🔘 createStereoPanner       | ✅    |
| 🔘 createWaveShaper         | ❌    |
| 🔘 decodeAudioData          | ❌    |

</div>

</details>

## ❌ Not yet available (**19** out of 32)

<details>
 <summary><b>AudioParamMap</b></summary>
</details>
<details>
 <summary><b>MediaElementAudioSourceNode</b></summary>
</details>
<details>
 <summary><b>MediaStreamAudioSourceNode</b></summary>
</details>
<details>
 <summary><b>MediaStreamTrackAudioSourceNode</b></summary>
</details>
<details>
 <summary><b>ConvolverNode</b></summary>
</details>
<details>
 <summary><b>DelayNode</b></summary>
</details>
<details>
 <summary><b>DynamicsCompressorNode</b></summary>
</details>
<details>
 <summary><b>WaveShaperNode</b></summary>
</details>
<details>
 <summary><b>IIRFilterNode</b></summary>
</details>
<details>
 <summary><b>MediaStreamAudioDestinationNode</b></summary>
</details>
<details>
 <summary><b>AnalyserNode</b></summary>
</details>
<details>
 <summary><b>ChannelSplitterNode</b></summary>
</details>
<details>
 <summary><b>MergerNode</b></summary>
</details>
<details>
 <summary><b>AudioListener</b></summary>
</details>
<details>
 <summary><b>PannerNode</b></summary>
</details>
<details>
 <summary><b>AudioWorkletNode</b></summary>
</details>
<details>
 <summary><b>AudioWorkletProcessor</b></summary>
</details>
<details>
 <summary><b>AudioWorkletGlobalScope</b></summary>
</details>
<details>
 <summary><b>OfflineAudioContext</b></summary>
</details>
