#!/usr/bin/env bash
# Submits a render job to the scheduler running in the kind cluster (via the
# frontend's NodePort, same request its web form sends). Prints the render
# id and polls until it completes. Run --help for options.
#
# Run deploy/local-up.sh first.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

DEFAULT_SCENE="$PROJECT_ROOT/render_worker/test_scenes/cornell_box_animated.gltf"

usage() {
    cat <<EOF
Usage: submit-job.sh [OPTIONS]

Submits a render job to the scheduler running in the kind cluster, waits
for it to complete, and prints the next step (make-video.sh). Run
deploy/local-up.sh first.

Options:
  -o, --output NAME   Output name; frames are stored in the MinIO bucket as
                       NAME_<frame> (default: demo_render)
  -s, --scene PATH    Path to a .gltf scene file on the host (default: the
                       animated Cornell box test scene)
  -w, --width N       Render width in pixels (default: 400)
      --height N      Render height in pixels (default: 300)
  -f, --fps N         Frames per second (default: 24)
  -n, --frames N      Number of frames to render (default: 48)
      --samples N     Samples per pixel (default: 32)
  -h, --help          Show this help and exit

Width/height/fps/frames/samples can also be set via WIDTH/HEIGHT/FPS/FRAMES/
SAMPLES environment variables; explicit flags take priority over those.

Examples:
  submit-job.sh
  submit-job.sh --output my_render --frames 10 --fps 12
  WIDTH=160 HEIGHT=120 submit-job.sh -o quick_test -n 6
EOF
}

OUTPUT_NAME="demo_render"
SCENE_FILE="$DEFAULT_SCENE"
WIDTH="${WIDTH:-400}"
HEIGHT="${HEIGHT:-300}"
FPS="${FPS:-24}"
FRAMES="${FRAMES:-48}"
SAMPLES="${SAMPLES:-32}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -o|--output)  OUTPUT_NAME="${2:?missing value for $1}"; shift 2 ;;
        -s|--scene)   SCENE_FILE="${2:?missing value for $1}"; shift 2 ;;
        -w|--width)   WIDTH="${2:?missing value for $1}"; shift 2 ;;
        --height)     HEIGHT="${2:?missing value for $1}"; shift 2 ;;
        -f|--fps)     FPS="${2:?missing value for $1}"; shift 2 ;;
        -n|--frames)  FRAMES="${2:?missing value for $1}"; shift 2 ;;
        --samples)    SAMPLES="${2:?missing value for $1}"; shift 2 ;;
        -h|--help)    usage; exit 0 ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

command -v jq >/dev/null || { echo "jq is required (apt install jq)." >&2; exit 1; }

if [[ ! -f "$SCENE_FILE" ]]; then
    echo "Scene file not found: $SCENE_FILE" >&2
    exit 1
fi

# scene_file_url is read as a literal path on the render worker's own
# filesystem (not an S3 key -- see render_worker/src/render_worker.cpp), so
# the scene has to actually exist there first. Copy it to every worker pod
# since we don't control which one the scheduler assigns the job to.
IN_POD_PATH="/tmp/$(basename "$SCENE_FILE")"
pods="$(render_worker_pods)"
if [[ -z "$pods" ]]; then
    echo "No render-worker pods found in namespace '$NAMESPACE'. Is the cluster up (deploy/local-up.sh)?" >&2
    exit 1
fi
for pod in $pods; do
    echo "Copying scene to $pod:$IN_POD_PATH"
    kubectl -n "$NAMESPACE" cp "$SCENE_FILE" "$pod:$IN_POD_PATH"
done

payload=$(jq -n \
    --argjson width "$WIDTH" --argjson height "$HEIGHT" \
    --argjson fps "$FPS" --argjson frames "$FRAMES" --argjson samples "$SAMPLES" \
    --arg scene "$IN_POD_PATH" --arg output "$OUTPUT_NAME" \
    '{width: $width, height: $height, frames_per_second: $fps, animation_runtime: $frames,
      samples_per_pixel: $samples, scene_file_url: $scene, output_filename: $output}')

response=$(curl -sS -X POST "${FRONTEND_URL}/api/renders" \
    -H 'Content-Type: application/json' -d "$payload")
id=$(echo "$response" | jq -r '.id // empty')
if [[ -z "$id" ]]; then
    echo "Submit failed:" >&2
    echo "$response" | jq . >&2
    exit 1
fi

echo "Submitted render $id ($FRAMES frames as '${OUTPUT_NAME}_<frame>' in bucket '$BUCKET')."
echo -n "Waiting for it to complete"
status="In Queue"
while [[ "$status" == "In Queue" || "$status" == "In Progress" ]]; do
    echo -n "."
    sleep 2
    status=$(curl -sS "${FRONTEND_URL}/api/renders/$id" | jq -r '.status')
done
echo " $status"

if [[ "$status" != "Completed" ]]; then
    echo "Render did not complete successfully (status: $status)." >&2
    exit 1
fi

cat <<EOF

Make a video:  deploy/demo/make-video.sh $OUTPUT_NAME $FPS
EOF
