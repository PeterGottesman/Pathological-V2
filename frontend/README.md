# Pathological V2 Frontend

This frontend is the user interface for submitting render jobs and viewing results.
It is built with Next.js (App Router) and talks to the scheduler through Next API routes.

## What This Frontend Does

- Lets users submit one or more render jobs.
- Polls job status on a timer.
- Shows render previews.
- Downloads completed renders.
- Uses a local API route to proxy image fetches from S3.

## How It Connects To The Project

Flow:

1. Browser submits a job to `POST /api/renders`.
2. Next API route forwards to scheduler `POST http://localhost:8080/renders`.
3. Scheduler dispatches work to render worker(s) over gRPC.
4. Worker renders, uploads output to S3, and reports completion.
5. Browser polls `GET /api/renders/:id`.
6. Next API route forwards to scheduler `GET /renders/:id/status`.
7. Completed images are fetched through `GET /api/render-image?image=...`.

## Quick Start (Frontend Only)

From the `frontend/` directory:

```bash
npm install
npm run dev
```

Open `http://localhost:3000`.

Note: job submit/status will fail unless the scheduler is running on `localhost:8080`.

## Full Local Run (Scheduler + Worker + Frontend)

Run in this order.

Prerequisites:

- Build tools and dependencies for `scheduler/` and `render_worker/` (CMake + vcpkg setup).
- AWS credentials available to those binaries (`default` profile is used in code).

### 1) Start Scheduler

From `scheduler/`:

```bash
cmake --preset default
cmake --build build
./build/pathological-sched --http-port 8080 --grpc-port 50052
```

### 2) Start Render Worker

From `render_worker/`:

```bash
cmake --preset default
cmake --build build
./build/pathological 127.0.0.1:50052 --port 50051 --render-address 127.0.0.1 --name worker-1
```

### 3) Start Frontend

From `frontend/`:

```bash
npm install
npm run dev
```

## Important Frontend Files

- `app/page.tsx`: Main UI and client logic (form state, submit, polling, image actions).
- `app/api/renders/route.ts`: Forwards submit requests to scheduler `POST /renders`.
- `app/api/renders/[id]/route.ts`: Forwards status checks to scheduler `GET /renders/{id}/status`.
- `app/api/render-image/route.ts`: Streams image data from S3 to browser (inline/download).
- `types/scheduler.ts`: Shared TypeScript request/response types.
- `app/layout.tsx`: Root layout and metadata.
- `app/global.css`: Global styles currently used by layout.

## Scheduler Contract Used By Frontend

Submit payload keys:

- `width`
- `height`
- `frames_per_second`
- `animation_runtime`
- `samples_per_pixel`
- `scene_file_url`
- `output_filename`

Expected status response includes:

- `id`
- `status` (`Completed`, `In Progress`, `In Queue`, `Error`)
- `download_link` (nullable)
- same render settings fields as submit payload

## Notes For Next Students

- Scheduler URL is hardcoded to `http://localhost:8080` in the Next API routes.
- S3 bucket URL is hardcoded in `app/api/render-image/route.ts`.
- Default scene path in the form is machine-specific (`/home/...`) and should be replaced for your environment.
- `frames_per_second` and `animation_runtime` are currently forced to constants during submit in `app/page.tsx`.
- `app/global.css` and `app/globals.css` both exist with similar content; only `app/global.css` is imported by layout.


## Quick Troubleshooting

- Submit fails immediately: check scheduler is running at `localhost:8080`.
- Jobs stay in queue: verify worker is connected to scheduler gRPC (`127.0.0.1:50052` by default).
- No preview image: check S3 upload completed and object key matches `output_filename + "_" + animation_runtime`.
- Scene load issues: make sure scene path exists and worker machine can access it.

