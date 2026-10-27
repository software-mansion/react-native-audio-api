# Phone orchestra server

Companion server for the **Orchestra** demo (`src/demos/Orchestra/`). Phones
connect over WebSocket, estimate the server clock (NTP-style ping/pong) and
schedule their part of the score with `start(when)`, so a room full of phones
plays one piece together.

```sh
node server.ts            # Node 22.18+ (type stripping), no dependencies
PORT=9000 node server.ts  # custom port (default 8765)
```

The server prints its LAN addresses: open the `http://…` one in a browser for
the dashboard (start/stop, tempo, score, per-phone part assignment,
sync quality). Phones join by pointing their camera at the dashboard's QR
code: it encodes `audioapi-example://orchestra?url=ws://<host>:8765`, which
opens the app on the Orchestra screen and connects. The `ws://…` address can
also be typed in by hand. Once connected, any phone can start or stop the
piece for everyone: its Start button sends the same `start` message as the
dashboard (3 s count-in, from bar 1), and every phone shows the countdown.

The QR code is rendered by `qrcodegen.ts`, vendored from
[nayuki/QR-Code-generator](https://github.com/nayuki/QR-Code-generator) (MIT)
and adapted to the erasable-syntax subset Node's type stripping accepts; its
output was checked identical to the upstream build for inputs spanning
versions 1–8.

Phones and the laptop must share a network. Conference Wi-Fi is the usual
failure point — bring a router, and watch the RTT column on the dashboard.

Scores live in `score.ts`; the wire format in `../src/demos/Orchestra/protocol.ts`.

## Recorded instruments

Parts with an `instrument` play recorded notes instead of the synth: every
General MIDI instrument of the FluidR3_GM soundfont, one mp3 per semitone
(~22 KB), or `drums`, an acoustic kit (kick, snare, clap, closed and open
hi-hat, two toms, crash, ride; a few takes each, picked at random) for parts
whose notes are General MIDI percussion keys. The kit's key mapping lives in
`drumKit.ts`. At startup the server downloads every note the scores use into
`samples/` (kept out of git) and serves it at `/samples/<instrument>/<note>.mp3`,
so phones need only the local network; notes nobody asked for yet are
downloaded on first request. Run the server once with internet access before
a show. Without it, phones fall back to the synth voices.

Phones load the notes of their own part when it is dealt, and play the synth
for any note that has not loaded yet. The phone's Voice row can switch any
part to piano, strings, flute, trumpet, harp or marimba, or back to the synth.

Sample credits: FluidR3_GM by Frank Wen (MIT), rendered to per-note mp3 by
Benjamin Gleitzman in [midi-js-soundfonts](https://github.com/gleitz/midi-js-soundfonts)
(CC BY 3.0). Drums from the [Versilian Community Sample Library](https://github.com/sgossner/VCSL)
by Versilian Studios (CC0).

## Importing MIDI files

Drop Standard MIDI Files (`.mid`, format 0 or 1) into `midi/` and restart the
server (or run `node --watch server.ts`). Each file becomes a score named after
the file. Only import music you have the right to use.

At startup the server prints every imported part, for example:

```
MIDI My Song.mid → "My Song" (midi-my-song), 120 bpm, 128 beats
  part "Lead": 212 notes, midi 64-84, pluck
  part "Bass": 160 notes, midi 33-45, transposed +24, pluck
  part "Drums (ch 10)": 410 notes, midi 36-51, kit
```

- Each track (or each channel of a multi-channel track) becomes one part.
- Each part plays the recorded instrument its General MIDI program selects
  (piano when the file sets none).
- Channel 10 becomes a `kit` part on the recorded drum kit; keys the kit has
  no piece for (cowbell, congas, …) are dropped.
- Parts lower than midi 55 move up whole octaves so phones can play them.
- Only the first tempo is used; tempo changes are reported as a warning.

To choose voices and how many phones play each part, add `My Song.json` next to
`My Song.mid`, using the part names the server printed:

```json
{
  "title": "My Song",
  "bpm": 118,
  "parts": {
    "Lead": { "instrument": "violin", "maxPlayers": 1 },
    "Bass": { "timbre": "pluck", "transpose": 12 },
    "Strings": { "skip": true }
  }
}
```

`instrument` takes any name from
[the FluidR3_GM list](https://gleitz.github.io/midi-js-soundfonts/FluidR3_GM/names.json)
and replaces the one the file selects. Timbres (the synth fallback): `bell`,
`pluck`, `pad`, `lead`, `hat`, `clap`, `kit`, `click`.
`maxPlayers` caps how many phones are dealt the part automatically (the rest
fill the uncapped parts). `transpose` (semitones) replaces the automatic octave
shift. Files that fail to parse are reported and skipped.

`splitDrums` breaks a drum part into one part per group of kit pieces, so the
kit is shared across phones. Each group becomes a part named `<part>: <group>`
that takes its own entry in `parts`; pieces no group lists stay in the original
part. Pieces: `kick`, `snare`, `clap`, `closedHat`, `openHat`, `lowTom`,
`highTom`, `crash`, `ride`.

```json
"Drums (ch 10)": {
  "splitDrums": {
    "Kick": ["kick"],
    "Snare": ["snare", "clap"],
    "Hats": ["closedHat", "openHat"],
    "Toms": ["lowTom", "highTom", "crash", "ride"]
  }
},
"Drums (ch 10): Kick": { "maxPlayers": 1 }
```
