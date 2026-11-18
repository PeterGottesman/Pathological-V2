#!/usr/bin/env bash
# GPU counterpart of local-up.sh. Run ON an EC2 GPU instance (e.g. g6.4xlarge
# with the AWS Deep Learning Base AMI, which ships the NVIDIA driver, Docker and
# the NVIDIA container toolkit). Builds the three images, installs a
# single-node k3s cluster whose default container runtime is nvidia, installs
# the NVIDIA device plugin, and applies overlays/aws-gpu.
#
# k3s rather than kind: kind nodes are containers and would need the NVIDIA
# toolkit installed and wired into containerd *inside* the node; k3s just
# uses the host's.
#
# The frontend is served on NodePort 30080 of the instance. Reach it from your
# machine with deploy/aws/remote-deploy.sh (SSH tunnel -> http://localhost:3000).
#
# Requires: docker, nvidia-smi + nvidia-container-toolkit, curl, sudo.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
K8S_DIR="$SCRIPT_DIR/k8s"

echo "==> Checking GPU"
nvidia-smi -L
command -v nvidia-ctk >/dev/null || { echo "nvidia-container-toolkit not found" >&2; exit 1; }

echo "==> Building images"
docker build -f "$SCRIPT_DIR/docker/scheduler.Dockerfile" -t pathological-scheduler:local "$REPO_ROOT"
docker build -f "$SCRIPT_DIR/docker/render_worker.Dockerfile" -t pathological-render-worker:local "$REPO_ROOT"
docker build -t pathological-frontend:local "$REPO_ROOT/frontend"

if ! command -v k3s >/dev/null; then
  echo "==> Installing k3s (default runtime: nvidia)"
  curl -sfL https://get.k3s.io | \
    INSTALL_K3S_EXEC="server --default-runtime nvidia --write-kubeconfig-mode 644 --disable traefik" sh -
fi
export KUBECONFIG=/etc/rancher/k3s/k3s.yaml
until kubectl get nodes 2>/dev/null | grep -q ' Ready'; do sleep 3; done

echo "==> Importing images into k3s"
for img in pathological-scheduler pathological-render-worker pathological-frontend; do
  docker save "$img:local" | sudo k3s ctr images import -
done

echo "==> Installing NVIDIA device plugin"
kubectl apply -f "$K8S_DIR/addons/nvidia-device-plugin.yaml"
echo "    waiting for the node to advertise nvidia.com/gpu"
until [ "$(kubectl get nodes -o jsonpath='{.items[0].status.allocatable.nvidia\.com/gpu}' 2>/dev/null)" ]; do sleep 3; done

echo "==> Applying manifests"
kubectl apply -k "$K8S_DIR/overlays/aws-gpu"
# Images keep the same :local tag across rebuilds, so `apply` alone won't
# roll pods onto freshly imported images on a re-run.
kubectl -n pathological rollout restart deployment/scheduler deployment/frontend statefulset/render-worker

echo "==> Waiting for rollout"
kubectl -n pathological rollout status deployment/scheduler --timeout=300s
kubectl -n pathological rollout status deployment/minio --timeout=300s
kubectl -n pathological wait --for=condition=complete job/minio-create-bucket --timeout=300s
kubectl -n pathological rollout status statefulset/render-worker --timeout=300s
kubectl -n pathological rollout status deployment/frontend --timeout=300s

echo
echo "Ready: frontend on this instance at http://localhost:30080"
echo "From your machine: deploy/aws/remote-deploy.sh tunnel  ->  http://localhost:3000"
echo "Render worker GPU check: kubectl -n pathological logs statefulset/render-worker | head"
