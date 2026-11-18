#!/usr/bin/env bash
# Run from your own machine. Drives deploy/aws-gpu-up.sh on an EC2 GPU instance.
#
#   HOST=<public-ip> KEY=<path/to/key.pem> deploy/aws/remote-deploy.sh [sync|up|tunnel|all]
#
#   sync    rsync the repo to ~/pathological on the instance
#   up      run deploy/aws-gpu-up.sh there (log: ~/up.log; streamed here)
#   tunnel  forward localhost:3000 -> instance frontend (and 9001 MinIO console)
#   all     sync, up, tunnel (default)
set -euo pipefail

: "${HOST:?set HOST to the instance public IP}"
: "${KEY:?set KEY to the ssh private key path}"
USER_NAME="${SSH_USER:-ubuntu}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SSH_OPTS=(-i "$KEY" -o StrictHostKeyChecking=accept-new -o ServerAliveInterval=30)

do_sync() {
  # --delete so the instance holds exactly this checkout (no stale files).
  rsync -az --delete -e "ssh ${SSH_OPTS[*]}" \
    --exclude .git --exclude 'build*' --exclude test_output --exclude uploads --exclude node_modules \
    "$REPO_ROOT/" "$USER_NAME@$HOST:pathological/"
}
do_up() {
  ssh "${SSH_OPTS[@]}" "$USER_NAME@$HOST" 'cd ~/pathological && ./deploy/aws-gpu-up.sh 2>&1 | tee ~/up.log'
}
do_tunnel() {
  echo "Tunnel up: http://localhost:3000 (Ctrl-C to close)"
  ssh "${SSH_OPTS[@]}" -N -L 3000:localhost:30080 -L 9001:localhost:30091 "$USER_NAME@$HOST"
}

case "${1:-all}" in
  sync) do_sync ;;
  up) do_up ;;
  tunnel) do_tunnel ;;
  all) do_sync; do_up; do_tunnel ;;
  *) echo "usage: $0 [sync|up|tunnel|all]" >&2; exit 2 ;;
esac
