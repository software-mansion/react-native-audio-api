# ChannelInterpretation

`ChannelInterpretation` type specifies how input channels are mapped out to output channels when the number of them are different.

**Acceptable values:**

* `speakers`

Use set of standard mapping rules for all combinations of common input and output setups.

* `discrete`

Covers all other cases. Mapping depends on relationship between number of input channels and number of output channels.

## Channels mapping table

### `speakers`

| Number of input channels | Number of output channels | Mixing rules |
| :------------------------: | :------------------------- | :------------ |
| 1 (Mono) | 2 (Stereo) | output.L = input.M  output.R = input.M |
| 1 (Mono) | 4 (Quad) | output.L = input.M  output.R = input.M  output.SL = 0  output.SR = 0 |
| 1 (Mono) | 6 (5.1) | output.L = 0  output.R = 0  output.C = input.M  output.LFE = 0  output.SL = 0  output.SR = 0 |
| 2 (Stereo) | 1 (Mono) | output.M = 0.5 \* (input.L + input.R) |
| 2 (Stereo) | 4 (Quad) | output.L = input.L  output.R = input.R  output.SL = 0  output.SR = 0 |
| 2 (Stereo) | 6 (5.1) | output.L = input.L  output.R = input.R  output.C = 0  output.LFE = 0  output.SL = 0  output.SR = 0 |
| 4 (Quad) | 1 (Mono) | output.M = 0.25 \* (input.L + input.R + input.SL + input.SR) |
| 4 (Quad) | 2 (Stereo) | output.L = 0.5 \* (input.L + input.SL)  output.R = 0.5 \* (input.R + input.SR) |
| 4 (Quad) | 6 (5.1) | output.L = input.L  output.R = input.R  output.C = 0  output.LFE = 0  output.SL = input.SL  output.SR = input.SR |
| 6 (5.1) | 1 (Mono) | output.M = 0.7071 \* (input.L + input.R) + input.C  + 0.5 \* (input.SL + input.SR) |
| 6 (5.1) | 2 (Stereo) | output.L = input.L + 0.7071 \* (input.C + input.SL)  output.R = input.R + 0.7071 \* (input.C + input.SR) |
| 6 (5.1) | 4 (Quad) | output.L = input.L + 0.7071 \* input.C  output.R = input.R + 0.7071 \* input.C  output.SL = input.SL  output.SR = input.SR |

### `discrete`

| Number of input channels | Number of output channels | Mixing rules |
| :------------------------: | :------------------------- | :------------ |
| x | y where y > x | Fill each output channel with its counterpart(channel with same number), rest of output channels are silent channels |
| x | y where y \< x | Fill each output channel with its counterpart(channel with same number), rest of input channels are skipped |
