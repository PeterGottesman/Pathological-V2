#!/usr/bin/env bash
# Submits a render job to the scheduler running in the kind cluster (via the
# frontend's NodePort, same request its web form sends). Prints the render
# id and polls until it completes.
#
# Run deploy/local-up.sh first.
#
# Usage: [WIDTH=n] [HEIGHT=n] [FPS=n] [FRAMES=n] [SAMPLES=n] submit-job.sh [output_name] [scene_file]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

command -v jq >/dev/null || { echo "jq is required (apt install jq)." >&2; exit 1; }

OUTPUT_NAME="${1:-demo_render}"
SCENE_FILE="${2:-$PROJECT_ROOT/render_worker/test_scenes/cornell_box_animated.gltf}"
WIDTH="${WIDTH:-400}"
HEIGHT="${HEIGHT:-300}"
FPS="${FPS:-24}"
FRAMES="${FRAMES:-48}"
SAMPLES="${SAMPLES:-32}"

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
