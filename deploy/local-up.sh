#!/usr/bin/env bash
# Builds all three images, stands up a local kind cluster (if one isn't
# already running), loads the images into it, applies the local overlay,
# and installs Headlamp (a web UI for browsing/managing the cluster).
#
# Requires: docker, kind, kubectl.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
K8S_DIR="$SCRIPT_DIR/k8s"
CLUSTER_NAME="pathological"

echo "==> Building images"
docker build -f "$SCRIPT_DIR/docker/scheduler.Dockerfile" -t pathological-scheduler:local "$REPO_ROOT"
docker build -f "$SCRIPT_DIR/docker/render_worker.Dockerfile" -t pathological-render-worker:local "$REPO_ROOT"
docker build -t pathological-frontend:local "$REPO_ROOT/frontend"

if ! kind get clusters | grep -qx "$CLUSTER_NAME"; then
  echo "==> Creating kind cluster '$CLUSTER_NAME'"
  kind create cluster --config "$K8S_DIR/kind-config.yaml"
else
  echo "==> Reusing existing kind cluster '$CLUSTER_NAME'"
fi

echo "==> Loading images into kind"
kind load docker-image pathological-scheduler:local --name "$CLUSTER_NAME"
kind load docker-image pathological-render-worker:local --name "$CLUSTER_NAME"
kind load docker-image pathological-frontend:local --name "$CLUSTER_NAME"

echo "==> Applying manifests"
kubectl apply -k "$K8S_DIR/overlays/local"

echo "==> Waiting for rollout"
kubectl -n pathological rollout status deployment/scheduler --timeout=180s
kubectl -n pathological rollout status deployment/minio --timeout=180s
kubectl -n pathological rollout status statefulset/render-worker --timeout=180s
kubectl -n pathological rollout status deployment/frontend --timeout=180s

echo "==> Installing Headlamp (cluster web UI)"
kubectl apply -f "$K8S_DIR/addons/headlamp.yaml"
kubectl -n kube-system rollout status deployment/my-headlamp --timeout=180s

echo
echo "Ready: http://localhost:3000"
echo
echo "Cluster web UI (Headlamp):"
echo "  kubectl port-forward -n kube-system service/my-headlamp 8080:80"
echo "  then open http://localhost:8080 and log in with:"
echo "  kubectl create token my-headlamp --namespace kube-system --duration=24h"
