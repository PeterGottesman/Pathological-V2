#!/usr/bin/env bash
# Shared configuration for the deploy/demo scripts (submit-job.sh,
# make-video.sh) -- the kind-cluster-deployment equivalent of the repo
# root's demo/ scripts, which run everything as local processes instead.
# Sourced by the other scripts here -- not meant to be run directly.
#
# Run deploy/local-up.sh first.

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$DEMO_DIR/../.." && pwd)"
RUN_DIR="$DEMO_DIR/.run"
mkdir -p "$RUN_DIR"

NAMESPACE="pathological"
# Must match base/configmap.yaml's S3_BUCKET.
BUCKET="pathological-capstone-s3-bucket"
# Must match overlays/local/secret.yaml.
MINIO_ROOT_USER="minioadmin"
MINIO_ROOT_PASSWORD="minioadmin"
# Must match overlays/local/kustomization.yaml's MinIO NodePort patch and
# kind-config.yaml's port mapping.
MINIO_URL="http://localhost:9000"
# Must match overlays/local/kustomization.yaml's frontend NodePort patch and
# kind-config.yaml's port mapping.
FRONTEND_URL="http://localhost:3000"

# render_worker_pods: prints the name of every render_worker pod. Used to
# copy a scene file to all of them, since we don't control which one the
# scheduler assigns a given job to.
render_worker_pods() {
    kubectl -n "$NAMESPACE" get pods -l app=render-worker -o jsonpath='{.items[*].metadata.name}'
}
