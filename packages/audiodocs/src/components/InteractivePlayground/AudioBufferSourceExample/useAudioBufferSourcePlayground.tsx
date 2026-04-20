import React, { useState } from "react";
import AudioBufferSourceExample from "./AudioBufferSourceExample";
import SliderInput from "@site/src/ui/SliderInput";
import Switch from "@site/src/ui/Switch";
import styles from "../styles.module.css";

const initialState = {
  playbackRate: 1.0,
  detune: 0,
  loop: false,
  loopStart: 0,
  loopEnd: 5,
  pitchCorrection: false,
};

export function useAudioBufferSourcePlayground() {
  const [playbackRate, setPlaybackRate] = useState(initialState.playbackRate);
  const [detune, setDetune] = useState(initialState.detune);
  const [loop, setLoop] = useState(initialState.loop);
  const [loopStart, setLoopStart] = useState(initialState.loopStart);
  const [loopEnd, setLoopEnd] = useState(initialState.loopEnd);
  const [pitchCorrection, setPitchCorrection] = useState(
    initialState.pitchCorrection
  );
  const [bufferDuration, setBufferDuration] = useState(10);

  const code = `import { AudioContext } from 'react-native-audio-api';

const ctx = new AudioContext();
const source = await ctx.createBufferSource({ pitchCorrection: ${pitchCorrection} });
source.buffer = audioBuffer;
source.playbackRate.value = ${playbackRate.toFixed(2)};
source.detune.value = ${detune.toFixed(0)};
source.loop = ${loop};
source.loopStart = ${loopStart.toFixed(2)};
source.loopEnd = ${loopEnd.toFixed(2)};
source.connect(ctx.destination);
source.start();`;

  const controls = (
    <div className={styles.controlsPanel}>
      <Switch
        ariaLabel="Enable pitch correction"
        checked={pitchCorrection}
        onChange={setPitchCorrection}
        rightLabel="Enable pitch correction"
      />

      <SliderInput
        label="Playback Rate"
        value={playbackRate}
        min={0.5}
        max={2}
        step={0.01}
        unit="x"
        onChange={setPlaybackRate}
      />

      <SliderInput
        label="Detune"
        value={detune}
        min={-1200}
        max={1200}
        step={1}
        unit="cents"
        onChange={setDetune}
      />

      <Switch
        ariaLabel="Toggle loop playback"
        checked={loop}
        onChange={setLoop}
        rightLabel="Loop"
      />

      {loop && (
        <div className={styles.loopRows}>
          <SliderInput
            label="Loop start"
            value={loopStart}
            min={0}
            max={bufferDuration}
            step={0.01}
            unit="s"
            onChange={(value) => {
              const safe = Math.min(value, loopEnd - 0.01);
              setLoopStart(Math.max(0, safe));
            }}
          />

          <SliderInput
            label="Loop stop"
            value={loopEnd}
            min={0.01}
            max={bufferDuration}
            step={0.01}
            unit="s"
            onChange={(value) => {
              const safe = Math.max(value, loopStart + 0.01);
              setLoopEnd(safe);
            }}
          />
        </div>
      )}
    </div>
  );

  return {
    code,
    controls,
    example: AudioBufferSourceExample,
    title: "Now Playing: example-music-01.mp3",
    props: {
      playbackRate,
      detune,
      loop,
      loopStart,
      loopEnd,
      pitchCorrection,
      onBufferLoad: setBufferDuration,
    },
  };
}
