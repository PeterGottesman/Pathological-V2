#!/usr/bin/env bash
# Downloads a render's frames from the kind cluster's MinIO and stitches
# them into an mp4 with ffmpeg.
#
# Usage: make-video.sh <output_filename> [fps] [out.mp4]
#   output_filename must match what was passed to submit-job.sh / the web
#   form - frames are stored in the bucket as "<output_filename>_<frame>".
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

command -v ffmpeg >/dev/null || { echo "ffmpeg is required." >&2; exit 1; }

PREFIX="${1:?usage: make-video.sh <output_filename> [fps] [out.mp4]}"
FPS="${2:-24}"
OUT_FILE="${3:-$RUN_DIR/${PREFIX}.mp4}"

DOWNLOAD_DIR="$RUN_DIR/frames-raw-$PREFIX"
FRAMES_DIR="$RUN_DIR/frames-seq-$PREFIX"
rm -rf "$DOWNLOAD_DIR" "$FRAMES_DIR"
mkdir -p "$DOWNLOAD_DIR" "$FRAMES_DIR"

echo "Downloading frames for '$PREFIX' from bucket '$BUCKET'..."
# --network host is Linux-only (unsupported by default on Docker Desktop for
# macOS/Windows); host.docker.internal + --add-host's host-gateway value
# reaches the host's published ports the same way on Linux, macOS, and
# Windows instead.
docker run --rm --add-host=host.docker.internal:host-gateway \
    -e MC_HOST_local="http://${MINIO_ROOT_USER}:${MINIO_ROOT_PASSWORD}@host.docker.internal:${MINIO_URL##*:}" \
    -v "$DOWNLOAD_DIR:/out" \
    quay.io/minio/mc mirror --quiet "local/$BUCKET" /out

python3 - "$PREFIX" "$DOWNLOAD_DIR" "$FRAMES_DIR" <<'PY'
import os, re, shutil, sys

prefix, indir, outdir = sys.argv[1:4]
pattern = re.compile(re.escape(prefix) + r"_(\d+)$")

frames = []
for name in os.listdir(indir):
    m = pattern.fullmatch(name)
    if m:
        frames.append((int(m.group(1)), name))
frames.sort()

if not frames:
    sys.exit(f"No frames found for '{prefix}' in {indir}. Has the render finished?")

for i, (_, name) in enumerate(frames):
    shutil.copyfile(os.path.join(indir, name), os.path.join(outdir, f"frame_{i:05d}.png"))

print(f"{len(frames)} frame(s) found.")
PY

ffmpeg -y -framerate "$FPS" -i "$FRAMES_DIR/frame_%05d.png" -pix_fmt yuv420p "$OUT_FILE"

echo
echo "Video written to $OUT_FILE"
