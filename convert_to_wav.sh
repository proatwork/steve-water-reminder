#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 2 ]; then
  echo "Usage: $0 <input_dir> <output_dir>"
  exit 1
fi

INPUT_DIR="$1"
OUTPUT_DIR="$2"

mkdir -p "$OUTPUT_DIR"

count=0
for f in "$INPUT_DIR"/*.mp3; do
  [ -f "$f" ] || continue
  name="$(basename "${f%.mp3}.wav")"
  ffmpeg -y -i "$f" -acodec pcm_s16le -ar 22050 -ac 1 "$OUTPUT_DIR/$name" </dev/null
  count=$((count + 1))
done

echo "Converted $count files -> $OUTPUT_DIR"
