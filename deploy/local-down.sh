#!/usr/bin/env bash
# Tears down the local kind cluster created by local-up.sh -- and
# everything running in it (the app, MinIO, Headlamp) along with it, since
# deleting the cluster deletes every namespace/resource inside it.
#
# Locally built Docker images (pathological-scheduler:local etc.) are left
# alone -- they're just a build cache, not cluster state, and removing them
# would make the next local-up.sh run rebuild from scratch.
#
# Requires: kind.
set -euo pipefail

CLUSTER_NAME="pathological"

if kind get clusters | grep -qx "$CLUSTER_NAME"; then
  echo "==> Deleting kind cluster '$CLUSTER_NAME'"
  kind delete cluster --name "$CLUSTER_NAME"
else
  echo "No kind cluster named '$CLUSTER_NAME' found -- nothing to do."
fi
