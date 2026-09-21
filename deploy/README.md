# Deploying Pathological

Docker images and Kubernetes manifests for the scheduler, render worker, and
frontend. One manifest family (Kustomize base + overlays), two targets:

- **`overlays/local`** — full stack in a local [kind](https://kind.sigs.k8s.io/)
  cluster: software Vulkan for the render worker, [MinIO](https://min.io/)
  standing in for S3, [Headlamp](https://headlamp.dev) as a cluster web UI.
  No GPU or AWS account needed.
- **`overlays/cluster`** — skeleton for the real GPU hardware cluster. Needs
  setup before it'll work; see [Cluster overlay setup](#cluster-overlay-setup)
  below.

## Prerequisites

- `docker`
- [`kind`](https://kind.sigs.k8s.io/docs/user/quick-start/#installation)
- `kubectl`

Only needed for `deploy/demo/`'s scripts:

- `jq` — `submit-job.sh`
- `ffmpeg`, `python3` — `make-video.sh`

## Quick start

```bash
./deploy/local-up.sh
```

Builds all three images, creates (or reuses) a kind cluster named
`pathological`, loads the images into it, applies `overlays/local`, and
installs Headlamp. Prints access info when done:

- App: http://localhost:3000
- MinIO console: http://localhost:9001 (`minioadmin` / `minioadmin`)
- Headlamp: port-forward command + login token instructions

Tear down with `./deploy/local-down.sh` (deletes the kind cluster and
everything in it; locally built images are left alone).

## Commands

| Command | What it does |
| --- | --- |
| `./deploy/local-up.sh` | Build images, stand up/update the local kind cluster, deploy everything. Idempotent — rerun after any code change to rebuild/reload/reapply. |
| `./deploy/local-down.sh` | Delete the kind cluster and everything in it. |
| `deploy/demo/submit-job.sh [OPTIONS]` | Submit a render job and wait for it to complete. `--help` for options (`--output`, `--scene`, `--width`, `--height`, `--fps`, `--frames`, `--samples`); `WIDTH`/`HEIGHT`/`FPS`/`FRAMES`/`SAMPLES` env vars work too, as a lower-priority fallback under explicit flags. |
| `deploy/demo/make-video.sh <output_name> [fps] [out.mp4]` | Download a completed render's frames from MinIO and stitch them into an mp4. `<output_name>` must match what was passed to `submit-job.sh`. |
| `kubectl -n pathological scale statefulset/render-worker --replicas=N` | Scale render workers locally. |
| `kubectl port-forward -n kube-system service/my-headlamp 8080:80` | Reach Headlamp at http://localhost:8080; log in with `kubectl create token my-headlamp --namespace kube-system --duration=24h`. |

## Layout

```
deploy/
  docker/
    scheduler.Dockerfile
    render_worker.Dockerfile
  k8s/
    base/                   # scheduler, render_worker (StatefulSet), frontend, ConfigMap
    overlays/
      local/                # + MinIO, kind NodePort, local secret
      cluster/              # GPU scheduling / real S3 extension points
    addons/
      headlamp.yaml         # cluster web UI, vendored static manifest
    kind-config.yaml
  demo/                      # submit a render job, download frames, make a video
  local-up.sh
  local-down.sh
frontend/Dockerfile
```

`render_worker` is deployed as a StatefulSet (not a Deployment) because each
worker registers its own address with the scheduler and needs a stable,
individually reachable DNS name across restarts
(`render-worker-0.render-worker.pathological.svc.cluster.local`, ...) — don't
change this to a Deployment.

## Local overlay: what it adds on top of `base/`

- `minio.yaml` — single-replica MinIO deployment. Storage is `emptyDir`
  (ephemeral) — data does not survive a MinIO pod restart.
- `minio-bucket-job.yaml` — one-shot Job that creates the render bucket and
  sets it to anonymous-download (the frontend fetches images by plain HTTP
  GET, not a presigned URL).
- `secret.yaml` — static `minioadmin`/`minioadmin` credentials. Local testing
  only; do not reuse for the cluster overlay.
- A ConfigMap patch pointing `S3_ENDPOINT`/`S3_PUBLIC_BASE_URL` at MinIO
  instead of real AWS S3.
- NodePort patches on the frontend and MinIO Services, mapped by
  `kind-config.yaml` to `localhost:3000` / `localhost:9000` (MinIO API) /
  `localhost:9001` (MinIO console).

## Cluster overlay setup

`overlays/cluster` is a skeleton. Three things need filling in before it'll
work on real hardware, all marked in `overlays/cluster/kustomization.yaml`
and `render-worker-patch.yaml`:

1. **Images.** Uncomment the `images:` block in `kustomization.yaml` and
   point it at a registry your cluster's nodes can pull from.
   `render_worker.Dockerfile`'s runtime stage installs Mesa's lavapipe
   (software Vulkan) — build a variant whose runtime stage installs the
   actual GPU/board's Vulkan ICD instead.
2. **GPU scheduling.** `render-worker-patch.yaml` has a placeholder
   `nodeSelector` (`pathological.io/gpu: "true"`) and a commented-out
   `resources.limits` example for the NVIDIA device plugin. Jetson boards
   typically need device hostPath mounts instead of a device-plugin
   resource — see NVIDIA's Jetson container docs. Fill in whatever matches
   your actual node labels/hardware.
3. **S3 credentials.** Nothing creates the `pathological-s3-credentials`
   Secret here (no real credential to commit to git). See
   `overlays/cluster/secret.example.yaml` for the shape and a
   `kubectl create secret` one-liner, or wire it through whatever
   secrets-management your cluster already uses.

`base/configmap.yaml`'s `S3_BUCKET`/`S3_REGION`/`S3_PUBLIC_BASE_URL` already
default to real AWS values, so the cluster overlay doesn't need an S3 config
patch unless you're using a different bucket.

Deploy with:

```bash
kubectl apply -k deploy/k8s/overlays/cluster
```

## Maintenance

- **Updating Headlamp.** `k8s/addons/headlamp.yaml` is a vendored, static
  `helm template` render — not managed by Helm at deploy time. To bump its
  version, re-render it and re-add the file's header comment (`helm
  template` doesn't preserve it):
  ```bash
  helm repo add headlamp https://kubernetes-sigs.github.io/headlamp/
  helm repo update headlamp
  helm template my-headlamp headlamp/headlamp --namespace kube-system \
    --version <new-version> > deploy/k8s/addons/headlamp.yaml
  ```
  Its ServiceAccount is bound to `cluster-admin` by the chart's defaults —
  scope this down before ever applying the manifest to a cluster with real
  credentials.
- **NodePort changes.** `kind-config.yaml`'s port mappings only take effect
  for a kind cluster created *after* the change — editing it and rerunning
  `local-up.sh` on an existing cluster does nothing. Recreate the cluster
  (`local-down.sh` then `local-up.sh`) to pick up new mappings.
- **vcpkg build speed.** The repo-root `vcpkg-overlay/` (see the README's
  "Faster dependency builds" section) applies to Docker and native builds
  alike; the Dockerfiles just `COPY` it in.

## Troubleshooting

- **Render worker can't find a Vulkan device / RT device creation fails.**
  The app hard-requires Vulkan's hardware ray-tracing extensions; most
  software Vulkan implementations (SwiftShader, older Mesa) don't implement
  them. `render_worker.Dockerfile`'s runtime stage installs
  `mesa-vulkan-drivers` for this — check that the installed Mesa version is
  24+ (earlier versions lack lavapipe's ray-tracing support).
- **MinIO/Headlamp `ImagePullBackOff`.** Both rely on public images
  (`quay.io/minio/*`, `ghcr.io/headlamp-k8s/*`) pulled fresh into the kind
  cluster; a registry outage or a vendor moving/restricting an image tag
  will surface here. `kubectl -n pathological describe pod <pod>` (MinIO) or
  `kubectl -n kube-system describe pod <pod>` (Headlamp) shows the actual
  pull error.
- **A NodePort isn't responding.** See the NodePort-changes note above —
  most likely the cluster predates the current `kind-config.yaml`.

## Known gaps

- No Ingress/TLS — the frontend is reached via NodePort (local) or would need
  a Service/Ingress choice made for the real cluster.
- No CI wiring to build/push these images automatically.
- No HPA for render_worker; scale manually per hardware capacity.
- No persistence for MinIO in the local overlay (by design — it's for
  testing, not for keeping renders around).
