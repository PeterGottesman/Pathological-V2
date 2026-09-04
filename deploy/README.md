# Deploying Pathological

Containers + Kubernetes manifests for the scheduler, render worker, and
frontend. One manifest family (Kustomize base + overlays), two ways to run
it:

- **`overlays/local`** — everything in a local [kind](https://kind.sigs.k8s.io/)
  cluster, using Mesa's lavapipe software Vulkan driver for the render worker
  and an in-cluster [MinIO](https://min.io/) instance standing in for S3. No
  GPU or AWS account needed.
- **`overlays/cluster`** — a skeleton for the real GPU cluster: same base
  manifests, patched for GPU node scheduling and pointed at real S3. This is
  intentionally left as an extension point (see below) since it depends on
  hardware this repo doesn't know about yet.

## Why lavapipe works here

The render worker hard-requires Vulkan's hardware ray-tracing extensions
(`vulkan_context.cpp`) — most software Vulkan implementations (SwiftShader,
older Mesa) don't implement those, so a naive "software GPU" setup would fail
at device creation, not just render slowly. Recent Mesa (24+) added
experimental software ray-tracing support to lavapipe, which is what makes
local testing possible at all. `render_worker.Dockerfile`'s runtime stage
installs `mesa-vulkan-drivers` for this. If a local build ever fails to find
a suitable device, check the Mesa version in the base image against what's
actually being used for local development.

## Local: kind + MinIO

Prerequisites: `docker`, [`kind`](https://kind.sigs.k8s.io/docs/user/quick-start/#installation),
`kubectl`.

```bash
./deploy/local-up.sh
```

This builds all three images, creates (or reuses) a kind cluster named
`pathological`, loads the images into it, applies `overlays/local`, installs
Headlamp (see below), and waits for everything to come up. Once it's done,
open **http://localhost:3000**.

What it sets up, on top of `base/`:

- `minio.yaml` — a single-replica MinIO deployment (ephemeral storage —
  emptyDir, so data doesn't survive a MinIO pod restart; fine for testing,
  not for anything you need to keep).
- `minio-bucket-job.yaml` — a one-shot Job that creates the render bucket and
  makes it anonymously downloadable (the frontend fetches images by plain
  HTTP GET, not a presigned URL).
- `secret.yaml` — static `minioadmin`/`minioadmin` credentials, local-testing
  only.
- A ConfigMap patch pointing `S3_ENDPOINT`/`S3_PUBLIC_BASE_URL` at MinIO
  instead of real AWS S3.
- NodePort patches on the frontend and MinIO Services; `kind-config.yaml`
  maps those to `localhost:3000` (frontend) and `localhost:9000`/`9001`
  (MinIO API/console — login `minioadmin`/`minioadmin`), so both are
  reachable from the host without a manual `kubectl port-forward`.

To tear down: `./deploy/local-down.sh` (deletes the kind cluster and
everything in it; locally built images are left alone).

Note: NodePort mappings only take effect for a kind cluster created *after*
`kind-config.yaml` was last changed — if you're reusing an older cluster and
a port isn't responding, recreate it (`local-down.sh` then `local-up.sh`).

### Cluster web UI (Headlamp)

`local-up.sh` also applies `k8s/addons/headlamp.yaml` — a vendored, static
render of the [Headlamp](https://headlamp.dev) Helm chart (the
community-recommended web UI now that the official Kubernetes Dashboard is
archived/unmaintained). It's a plain manifest, not managed by Helm at deploy
time, so bumping its version means re-rendering it (see the file's header
comment) rather than an in-place upgrade.

It installs into `kube-system`, not `pathological` — it's a general cluster
tool, not part of the app. To use it:

```bash
kubectl port-forward -n kube-system service/my-headlamp 8080:80
# then open http://localhost:8080 and log in with:
kubectl create token my-headlamp --namespace kube-system --duration=24h
```

Its ServiceAccount is bound to `cluster-admin` by the chart's defaults —
fine for local kind testing, but scope it down before ever applying this
manifest to a real cluster with real credentials.

### Rebuilding after a code change

`local-up.sh` is idempotent — rerun it. It reuses the existing kind cluster
and just rebuilds/reloads images and re-applies manifests.

### Scaling workers locally

```bash
kubectl -n pathological scale statefulset/render-worker --replicas=3
```

## Cluster: real GPU hardware

```bash
kubectl apply -k deploy/k8s/overlays/cluster
```

This is a **skeleton, not a finished config** — three things need filling in
before it'll work on real hardware, all marked in
`overlays/cluster/kustomization.yaml` and `render-worker-patch.yaml`:

1. **Images.** The `images:` block in `kustomization.yaml` is commented out;
   uncomment and point it at wherever your cluster's nodes can pull from.
   `render_worker.Dockerfile`'s runtime stage installs lavapipe — for real
   hardware you need a runtime image whose Vulkan ICD talks to the actual
   GPU/board instead (build a variant, or install the vendor's userspace
   driver into the runtime stage).
2. **GPU scheduling.** `render-worker-patch.yaml` has a placeholder
   `nodeSelector` (`pathological.io/gpu: "true"`) and a commented-out
   `resources.limits` example for the NVIDIA device plugin. Jetson boards
   (mentioned in the root README as a target platform) typically need device
   hostPath mounts instead of a device-plugin resource — see NVIDIA's Jetson
   container docs. Fill in whatever matches your actual node labels/hardware.
3. **S3 credentials.** Nothing creates the `pathological-s3-credentials`
   Secret on the cluster overlay (there's no real credential to commit to
   git). See `overlays/cluster/secret.example.yaml` for the shape and a
   `kubectl create secret` one-liner, or wire it through whatever
   secrets-management your cluster already uses.

`base/configmap.yaml`'s `S3_BUCKET`/`S3_REGION`/`S3_PUBLIC_BASE_URL` already
default to real AWS values (the same ones currently hardcoded in the
non-containerized build) — only the local overlay overrides them to point at
MinIO, so the cluster overlay doesn't need an S3 config patch unless you're
using a different bucket.

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
  vcpkg-overlay-triplets/    # skip vcpkg debug builds in Docker (build speed)
  demo/                      # submit a render job, download frames, make a video
  local-up.sh
  local-down.sh
frontend/Dockerfile
```

## Why render_worker is a StatefulSet

Render workers register their *own* address with the scheduler on startup,
and the scheduler dispatches jobs by calling back into that address — so
each worker needs a stable, individually reachable identity, not just a pod
IP that changes on every restart. A StatefulSet + headless Service gives
each pod a predictable DNS name
(`render-worker-0.render-worker.pathological.svc.cluster.local`, ...) that
the worker passes to the scheduler as its own `--render-address`. The
scheduler itself has no such requirement (workers only ever call it at a
single stable Service name), and keeps no state that would survive a
restart anyway, so it's a plain single-replica Deployment.

## Known gaps

- No Ingress/TLS — the frontend is reached via NodePort (local) or would need
  a Service/Ingress choice made for the real cluster.
- No CI wiring to build/push these images automatically.
- No HPA for render_worker; scale manually per hardware capacity.
- No persistence for MinIO in the local overlay (by design — it's for
  testing, not for keeping renders around).
