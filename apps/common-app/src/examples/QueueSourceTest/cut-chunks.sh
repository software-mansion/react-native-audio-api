#!/usr/bin/env bash
# TEMPORARY — regenerate 1s WAV chunks for QueueSourceTest.
# Usage: ./cut-chunks.sh [segment_seconds]
set -euo pipefail

SEGMENT="${1:-1}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/../WavScheduleSplit"
OUT="$SCRIPT_DIR/chunks"

mkdir -p "$OUT/music" "$OUT/pad" "$OUT/tone"
rm -f "$OUT"/music/*.wav "$OUT"/pad/*.wav "$OUT"/tone/*.wav

echo "Segmenting music ($SEGMENT s)…"
ffmpeg -y -hide_banner -loglevel error \
  -i "$SRC/music-track4-15s.wav" \
  -f segment -segment_time "$SEGMENT" -reset_timestamps 1 -c:a pcm_s16le \
  "$OUT/music/chunk-%03d.wav"

echo "Concat + segmenting pad…"
ffmpeg -y -hide_banner -loglevel error \
  -i "$SRC/ffmpeg-pad-part1.wav" -i "$SRC/ffmpeg-pad-part2.wav" \
  -filter_complex "[0:a][1:a]concat=n=2:v=0:a=1[a]" -map "[a]" \
  -f segment -segment_time "$SEGMENT" -reset_timestamps 1 -c:a pcm_s16le \
  "$OUT/pad/chunk-%03d.wav"

echo "Concat + segmenting tone…"
ffmpeg -y -hide_banner -loglevel error \
  -i "$SRC/ffmpeg-tone-part1.wav" -i "$SRC/ffmpeg-tone-part2.wav" \
  -filter_complex "[0:a][1:a]concat=n=2:v=0:a=1[a]" -map "[a]" \
  -f segment -segment_time "$SEGMENT" -reset_timestamps 1 -c:a pcm_s16le \
  "$OUT/tone/chunk-%03d.wav"

echo "Done:"
echo "  music: $(ls "$OUT/music" | wc -l | tr -d ' ') files"
echo "  pad:   $(ls "$OUT/pad" | wc -l | tr -d ' ') files"
echo "  tone:  $(ls "$OUT/tone" | wc -l | tr -d ' ') files"
echo "Update ffmpegChunks.ts requires if the file count changed."
