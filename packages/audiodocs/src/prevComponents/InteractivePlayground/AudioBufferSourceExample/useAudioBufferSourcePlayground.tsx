import React, { useState } from "react";
import AudioBufferSourceExample from "./AudioBufferSourceExample";
import LocalMuiTheme from "../interactivePlaygroundMuiTheme";

import Box from "@mui/material/Box";
import Typography from "@mui/material/Typography";
import Slider from "@mui/material/Slider";
import Switch from "@mui/material/Switch";
import FormControlLabel from "@mui/material/FormControlLabel";

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
    <LocalMuiTheme>
      <Box
        sx={{
          p: 2,
          borderRadius: 2,
          bgcolor: "background.paper",
          border: "1px solid",
          borderColor: "divider",
        }}
      >
        <FormControlLabel
          control={
            <Switch
              checked={pitchCorrection}
              onChange={(e) => setPitchCorrection(e.target.checked)}
            />
          }
          label="Enable pitch correction"
        />

        <Typography gutterBottom>
          Playback Rate: {playbackRate.toFixed(2)}x
        </Typography>
        <Slider
          value={playbackRate}
          min={0.5}
          max={2.0}
          step={0.01}
          onChange={(e, v) => setPlaybackRate(v as number)}
          valueLabelDisplay="auto"
        />

        <Typography gutterBottom>Detune: {detune} cents</Typography>
        <Slider
          value={detune}
          min={-1200}
          max={1200}
          step={1}
          onChange={(e, v) => setDetune(v as number)}
          valueLabelDisplay="auto"
        />

        <FormControlLabel
          control={
            <Switch
              checked={loop}
              onChange={(e) => setLoop(e.target.checked)}
            />
          }
          label="Loop"
        />

        {loop && (
          <Box sx={{ pl: 2, borderLeft: "2px solid", borderColor: "divider" }}>
            <Typography gutterBottom>
              Loop start: {loopStart.toFixed(2)}s
            </Typography>
            <Slider
              value={loopStart}
              min={0}
              max={bufferDuration}
              step={0.01}
              onChange={(e, v) => {
                const safe = Math.min(v as number, loopEnd - 0.01);
                setLoopStart(Math.max(0, safe));
              }}
              valueLabelDisplay="auto"
            />

            <Typography gutterBottom>
              Loop stop: {loopEnd.toFixed(2)}s
            </Typography>
            <Slider
              value={loopEnd}
              min={0.01}
              max={bufferDuration}
              step={0.01}
              onChange={(e, v) => {
                const safe = Math.max(v as number, loopStart + 0.01);
                setLoopEnd(safe);
              }}
              valueLabelDisplay="auto"
            />
          </Box>
        )}
      </Box>
    </LocalMuiTheme>
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
