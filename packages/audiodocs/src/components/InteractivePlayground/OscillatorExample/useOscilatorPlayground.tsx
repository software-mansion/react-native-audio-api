import React, { useState } from "react";
import OscillatorExample from "./OscillatorExample";
import { OscillatorType } from "react-native-audio-api";
import LocalMuiTheme from "../interactivePlaygroundMuiTheme";

import Box from "@mui/material/Box";
import Typography from "@mui/material/Typography";
import Slider from "@mui/material/Slider";
import Select, { SelectChangeEvent } from "@mui/material/Select";
import MenuItem from "@mui/material/MenuItem";
import FormControl from "@mui/material/FormControl";
import InputLabel from "@mui/material/InputLabel";

const initialState = {
  type: "sine" as OscillatorType,
  frequency: 440,
  detune: 0,
  volume: 0.5,
};

const oscillatorTypes: OscillatorType[] = [
  "sine",
  "square",
  "sawtooth",
  "triangle",
];

export function useOscillatorPlayground() {
  const [type, setType] = useState<OscillatorType>(initialState.type);
  const [frequency, setFrequency] = useState(initialState.frequency);
  const [detune, setDetune] = useState(initialState.detune);
  const [volume, setVolume] = useState(initialState.volume);

  const code = `import { AudioContext } from 'react-native-audio-api';

const ctx = new AudioContext();
const oscillator = ctx.createOscillator();
const gain = ctx.createGain();

oscillator.type = '${type}';

oscillator.frequency.value = ${frequency};
oscillator.detune.value = ${detune};
gain.gain.value = ${volume.toFixed(2)};

oscillator.connect(gain);
gain.connect(ctx.destination);
oscillator.start();`;

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
        <FormControl fullWidth sx={{ mb: 2 }}>
          <InputLabel>Type</InputLabel>
          <Select
            label="Type"
            value={type}
            onChange={(event) => setType(event.target.value as OscillatorType)}
          >
            {oscillatorTypes.map((oscType) => (
              <MenuItem key={oscType} value={oscType}>
                {oscType.charAt(0).toUpperCase() + oscType.slice(1)}
              </MenuItem>
            ))}
          </Select>
        </FormControl>

        <Typography gutterBottom>Frequency: {frequency} Hz</Typography>
        <Slider
          value={frequency}
          min={20}
          max={2000}
          onChange={(e, v) => setFrequency(v as number)}
          valueLabelDisplay="auto"
        />

        <Typography gutterBottom>Detune: {detune} cents</Typography>
        <Slider
          value={detune}
          min={-1200}
          max={1200}
          onChange={(e, v) => setDetune(v as number)}
          valueLabelDisplay="auto"
        />

        <Typography gutterBottom>Volume: {volume.toFixed(2)}</Typography>
        <Slider
          value={volume}
          min={0}
          max={1}
          step={0.01}
          onChange={(e, v) => setVolume(v as number)}
          valueLabelDisplay="auto"
        />
      </Box>
    </LocalMuiTheme>
  );

  return {
    code,
    controls,
    example: OscillatorExample,
    title: "Oscillator Node",
    props: {
      type,
      frequency,
      detune,
      volume,
    },
  };
}
