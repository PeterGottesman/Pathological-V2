'use client'
//Main and only page for the frontend, contains the form to submit render jobs to the scheduler and a section to display rendered images with options to download or delete them. It also implements polling to update the status of submitted jobs and their rendered images.
import { useEffect, useRef, useState } from 'react'
import Image from 'next/image'
import type { SubmitRenderPayload, RenderJob } from '@/types/scheduler'
// Creates type for form state based on the SubmitRenderRequest type, and a JobView type that extends RenderJob with additional fields for the requested file name and the local image URL. Also defines a SelectedImage type for managing the currently selected image in the UI.
type FormState = SubmitRenderPayload
type JobView = RenderJob & { imageUrl: string | null }
type SelectedImage = {
  src: string
  name: string
}
//Constants for polling interval
const POLL_INTERVAL_SECONDS = 10
const POLL_INTERVAL_MS = POLL_INTERVAL_SECONDS * 1000
const DEFAULT_FPS = 30
const DEFAULT_RUNTIME = 10
// Helper function to build an API image URL from an object key.
function buildImageApiUrl(outputFilename: string) {
  return `/api/render-image?image=${encodeURIComponent(outputFilename)}&ts=${Date.now()}`
}
//Helper function to resolve an image URL by checking object-key candidates through the /api/render-image endpoint.
function resolveApiImageUrl(
  outputFilename: string,
  downloadLink: string | null,
  animationRuntime: number
) {
  if (downloadLink) {
    const key = normalizeDownloadKey(downloadLink)
    if (key) return key
  }

  const base = toFileName(outputFilename).trim()
  if (!base) return null
  return `${base}_${Math.trunc(animationRuntime)}`
}
//Function to extract the file name from a path, used to help generate output file names and resolve image URLs.
function toFileName(value: string) {
  return value.split('/').pop() ?? value
}
//Ensure file ends in .png, return a default name if result is empty after trimming
function ensurePngName(name: string) {
  const trimmed = toFileName(name).trim()
  if (!trimmed) return 'render.png'
  return /\.png$/i.test(trimmed) ? trimmed : `${trimmed}.png`
}
//Normarilize download link by handling both s3:// and object key
function normalizeDownloadKey(downloadLink: string) {
  const raw = downloadLink.trim()

  if (raw.startsWith('s3://')) {
    const withoutScheme = raw.slice('s3://'.length)
    const slashIndex = withoutScheme.indexOf('/')
    if (slashIndex < 0) return null
    return withoutScheme.slice(slashIndex + 1)
  }

  if (raw.startsWith('http://') || raw.startsWith('https://')) {
    return raw
  }

  return raw
}
//Resolve the image URL for a render job by checking the download link and output filename, returning a local API URL if possible
function resolveImageUrl(
  outputFilename: string,
  downloadLink: string | null,
  animationRuntime: number
) {
  const key = resolveApiImageUrl(outputFilename, downloadLink, animationRuntime)
  return key ? buildImageApiUrl(key) : null
}
// The main React component for the home page, holds most UI state and logic 
export default function Home() {
    //State for managing form inputs
  const [forms, setForms] = useState<FormState[]>([
    {
      width: 1920,
      height: 1080,
      frames_per_second: DEFAULT_FPS,
      animation_runtime: DEFAULT_RUNTIME,
      samples_per_pixel: 16,
      scene_file_url: '/home/dtre/Pathological-V2/render_worker/test_scenes/cornell_box.gltf',
      output_filename: 'cornell_box.png',
    },
  ])
  //States for managing submission status, list of render jobs, countdown for next poll, and currently selected image for viewing
  const [submitting, setSubmitting] = useState(false)
  const [jobs, setJobs] = useState<JobView[]>([])
  const [secondsUntilNextPoll, setSecondsUntilNextPoll] = useState(POLL_INTERVAL_SECONDS)
  const [selectedImage, setSelectedImage] = useState<SelectedImage | null>(null)

  const jobsRef = useRef<JobView[]>([])

  useEffect(() => {
    jobsRef.current = jobs
  }, [jobs])
  
  function onSceneChange(index: number, value: string) {
    const v = value ?? ''
    const last = v.split('/').pop() || ''
    const base = last.replace(/\.[^/.]+$/, '')
    const filename = base ? `${base}.png` : 'frontend_render.png'

    setForms((prev) => {
      const next = [...prev]
      next[index] = {
        ...next[index],
        scene_file_url: v,
        output_filename: filename,
      }
      return next
    })
  }

  function onTextChange(index: number, key: keyof FormState, value: string) {
    setForms((prev) => {
      const next = [...prev]
      next[index] = { ...next[index], [key]: value as never }
      return next
    })
  }

  function onNumberChange(index: number, key: keyof FormState, value: string) {
    setForms((prev) => {
      const next = [...prev]
      next[index] = { ...next[index], [key]: Number(value) as never }
      return next
    })
  }

  function onAdd() {
    setForms((prev) => [
      ...prev,
      {
        width: 1920,
        height: 1080,
        frames_per_second: DEFAULT_FPS,
        animation_runtime: DEFAULT_RUNTIME,
        samples_per_pixel: 16,
        scene_file_url: '',
        output_filename: '',
      },
    ])
  }

  function onRemove(index: number) {
    setForms((prev) => prev.filter((_, i) => i !== index))
  }

  async function onDownload(job: JobView) {
    const key = resolveApiImageUrl(job.output_filename, job.download_link, job.animation_runtime)
    if (!key) return

    const downloadName = ensurePngName(job.output_filename)

    const localDownloadUrl = `/api/render-image?image=${encodeURIComponent(key)}&download=${encodeURIComponent(downloadName)}`
    const anchor = document.createElement('a')
    anchor.href = localDownloadUrl
    anchor.download = downloadName
    document.body.appendChild(anchor)
    anchor.click()
    anchor.remove()
  }
//Handler for when a user clicks on a rendered image to view it, sets the selected image state to open the image viewer modal
  async function onOpenImage(job: JobView) {
    const src = resolveImageUrl(job.output_filename, job.download_link, job.animation_runtime)

    if (!src) return
    setSelectedImage({ src, name: job.output_filename })
  }

  function onDelete(index: number, outputFilename: string) {
    setJobs((prev) => prev.filter((_, i) => i !== index))
    setSelectedImage((prev) => (prev?.name === outputFilename ? null : prev))
  }
//Handler for submitting the render job form; sends a POST request to the /api/renders endpoint for each form, then updates the jobs state with the responses and starts the polling countdown
  async function onSubmit() {
    try {
      setSubmitting(true)

      const results = await Promise.all(
        forms.map(async (form) => {
          const payload: SubmitRenderPayload = {
            ...form,
            frames_per_second: DEFAULT_FPS,
            animation_runtime: DEFAULT_RUNTIME,
          }

          const res = await fetch('/api/renders', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
          })

          if (!res.ok) {
            throw new Error(`Submit failed with status ${res.status}`)
          }

          const data = (await res.json()) as RenderJob
          const imageUrl = resolveImageUrl(data.output_filename, data.download_link, data.animation_runtime)

          return {
            ...data,
            imageUrl,
          } as JobView
        })
      )

      setJobs(results)
      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)
    } catch (error) {
      console.error(error)
    } finally {
      setSubmitting(false)
    }
  }
//useEffect hook to implement polling for job status updates and image URL resolution, with a countdown timer for the next poll
  useEffect(() => {
    if (jobs.length === 0) {
      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)
      return
    }

    let isCancelled = false
    let remaining = POLL_INTERVAL_SECONDS

    setSecondsUntilNextPoll(remaining)

    const countdownInterval = setInterval(() => {
      remaining = remaining > 1 ? remaining - 1 : POLL_INTERVAL_SECONDS
      if (!isCancelled) {
        setSecondsUntilNextPoll(remaining)
      }
    }, 1000)

    const pollInterval = setInterval(async () => {
      try {
        const currentJobs = jobsRef.current
        const hasPollableJobs = currentJobs.some((job) => {
          const id = String(job.id ?? '').trim()
          const shouldRefreshStatus = job.status !== 'Completed' && job.status !== 'Error'
          const shouldResolveImage = job.status !== 'Error' && job.imageUrl === null
          return id !== '' && (shouldRefreshStatus || shouldResolveImage)
        })

        if (!hasPollableJobs) return

        const updated = await Promise.all(
          currentJobs.map(async (job) => {
            if (job.status === 'Error') return job

            const id = String(job.id ?? '').trim()
            if (!id) return job

            const currentImageUrl = resolveImageUrl(job.output_filename, job.download_link, job.animation_runtime)

            if (job.status === 'Completed') {
              return {
                ...job,
                imageUrl: currentImageUrl,
              } as JobView
            }

            try {
              const res = await fetch(`/api/renders/${encodeURIComponent(String(job.id))}`, {
                method: 'GET',
                cache: 'no-store',
              })

              if (!res.ok) {
                return {
                  ...job,
                  imageUrl: currentImageUrl,
                } as JobView
              }

              const data = (await res.json()) as RenderJob
              const imageUrl = resolveImageUrl(data.output_filename, data.download_link, data.animation_runtime)

              return {
                ...data,
                imageUrl,
              } as JobView
            } catch {
              return {
                ...job,
                imageUrl: currentImageUrl,
              } as JobView
            }
          })
        )

        if (!isCancelled) {
          setJobs(updated)
        }
      } catch (error) {
        console.error(error)
      }
    }, POLL_INTERVAL_MS)

    return () => {
      isCancelled = true
      clearInterval(countdownInterval)
      clearInterval(pollInterval)
    }
  }, [jobs.length])

  const activeJobCount = jobs.filter((job) => {
    if (job.status === 'Error') return false
    if (job.status === 'Completed' && job.imageUrl !== null) return false
    return true
  }).length

  
//UI rendering logic
  return (
    <div className="flex min-h-screen flex-col bg-black text-red-500 font-mono">
      <header className="w-full border-b border-red-800 bg-black p-6 shadow-[0_0_15px_rgba(220,38,38,0.5)]">
        <div className="mx-auto flex max-w-7xl flex-col items-center justify-between gap-4 md:flex-row">
          <div className="flex flex-col">
            <h1 className="text-3xl font-bold tracking-tighter text-red-500 drop-shadow-[0_0_5px_rgba(220,38,38,0.8)]">
              PATHOLOGICAL V2
            </h1>
            <p className="text-xs text-red-800">BACS CAPSTONE: TEAM 19</p>
          </div>

          <div className="flex flex-wrap justify-center gap-6 text-sm text-red-400">
            <span className="cursor-default transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)]">
              Dontre Quarles
            </span>
            <span className="cursor-default transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)]">
              Kobie Morales
            </span>
            <span className="cursor-default transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)]">
              Hunter Ellenberger
            </span>
            <span className="cursor-default transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)]">
              Austin Johnson
            </span>
          </div>
        </div>
      </header>

      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
        <div className="mb-6 w-full max-w-6xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
          <h2 className="text-xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-2xl">
            About
          </h2>
          <p className="mt-3 text-sm leading-relaxed text-red-200 md:text-base">
            Pathological V2 is a distributed render pipeline. You submit a scene (GLTF) to scheduler with render parameters
            (resolution, frames, samples-per-pixel). The scheduler gets the job, dispatches work to render workers, and uploads
            results back to S3.
          </p>
        </div>

        <div className="grid w-full max-w-6xl grid-cols-1 gap-6 lg:grid-cols-2">
          <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
            <h2 className="mb-2 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-3xl">
              Submit Render Job
            </h2>

            <div className="space-y-5">
              {forms.map((form, i) => (
                <div key={i} className="relative space-y-5 rounded-lg border border-red-900/60 bg-black/20 p-4">
                  {i > 0 ? (
                    <button
                      type="button"
                      onClick={() => onRemove(i)}
                      aria-label={`Remove render form ${i + 1}`}
                      className="absolute right-2 top-2 rounded border border-red-700 px-2 py-0.5 text-xs font-bold text-red-300 transition-all hover:text-red-100 hover:drop-shadow-[0_0_8px_rgba(220,38,38,0.9)]"
                    >
                      X
                    </button>
                  ) : null}

                  <div className="space-y-2">
                    <label className="block text-sm font-semibold text-red-300">Scene file (.gltf)</label>
                    <input
                      placeholder="Absolute path to gltf file"
                      value={form.scene_file_url}
                      onChange={(e) => onSceneChange(i, e.target.value)}
                      className="block w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                    />
                  </div>

                  <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Output filename</label>
                      <input
                        value={form.output_filename}
                        onChange={(e) => onTextChange(i, 'output_filename', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Samples / Pixel</label>
                      <input
                        type="number"
                        min={1}
                        value={form.samples_per_pixel}
                        onChange={(e) => onNumberChange(i, 'samples_per_pixel', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Width</label>
                      <input
                        type="number"
                        min={1}
                        value={form.width}
                        onChange={(e) => onNumberChange(i, 'width', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Height</label>
                      <input
                        type="number"
                        min={1}
                        value={form.height}
                        onChange={(e) => onNumberChange(i, 'height', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>
                  </div>
                </div>
              ))}

              <button
                onClick={onSubmit}
                disabled={submitting}
                className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all ${
                  submitting
                    ? 'cursor-not-allowed opacity-60'
                    : 'hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]'
                }`}
              >
                {submitting ? 'Submitting…' : 'Submit'}
              </button>

              <button
                onClick={onAdd}
                className="w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]"
              >
                Add
              </button>
            </div>
          </div>

          <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
            <h2 className="mb-4 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-3xl">
              Renders
            </h2>

            <div className="rounded-lg border border-red-800 bg-black/50 p-4">
              <div className="flex items-center gap-3 rounded-lg border border-red-900/60 bg-black/40 px-3 py-2 text-sm text-red-200">
                <div className="relative flex h-10 w-10 items-center justify-center">
                  {activeJobCount > 0 ? (
                    <div className="absolute inset-0 animate-spin rounded-full border-2 border-red-900 border-t-red-400" />
                  ) : (
                    <div className="absolute inset-0 rounded-full border-2 border-red-900" />
                  )}
                  <span className="relative z-10 text-xs font-semibold">{secondsUntilNextPoll}</span>
                </div>
                <span>
                  {activeJobCount > 0
                    ? `Next poll in ${secondsUntilNextPoll}s`
                    : 'No active jobs to poll'}
                </span>
              </div>
            </div>

            <div className="mt-5 min-h-[500px] rounded-lg border border-red-800 bg-black/50 p-4">
              <div className="font-semibold text-red-300">Rendered Images</div>

              <div className="mt-4 grid grid-cols-1 gap-4">
                {jobs.length === 0 ? (
                  <div className="flex min-h-[180px] items-center justify-center rounded-lg border border-red-900/60 bg-black/40 text-sm text-red-300/70">
                    No rendered images yet.
                  </div>
                ) : (
                  jobs.map((job, idx) => (
                    <div
                      key={`${String(job.id)}-${idx}`}
                      className="rounded-lg border border-red-900/60 bg-black/40 p-4 text-sm text-red-200"
                    >
                      <div className="grid grid-cols-3 items-center gap-2">
                        <button
                          type="button"
                          onClick={() => onDownload(job)}
                          disabled={!job.imageUrl}
                          className={`justify-self-start rounded border border-red-800 px-2 py-1 text-xs font-semibold transition-all ${
                            job.imageUrl
                              ? 'hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,0.9)]'
                              : 'cursor-not-allowed opacity-50'
                          }`}
                        >
                          Download
                        </button>

                        <div className="text-center text-sm font-semibold text-red-300">{job.output_filename}</div>

                        <button
                          type="button"
                          onClick={() => onDelete(idx, job.output_filename)}
                          className="justify-self-end rounded border border-red-800 px-2 py-1 text-xs font-semibold transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,0.9)]"
                        >
                          Delete
                        </button>
                      </div>
                      {job.imageUrl ? (
                        <button
                          type="button"
                          onClick={() => onOpenImage(job)}
                          className="mt-3 block w-full"
                        >
                          <div className="flex h-56 w-full items-center justify-center overflow-hidden rounded-md border border-red-900/60 bg-black/40">
                            <Image
                              src={job.imageUrl}
                              alt={job.output_filename}
                              width={1024}
                              height={1024}
                              unoptimized
                              className="h-full w-full object-contain"
                            />
                          </div>
                        </button>
                      ) : (
                        <div className="mt-3 flex h-56 w-full items-center justify-center rounded-md border border-red-900/60 bg-black/40 text-xs text-red-300/70">
                          Rendering preview unavailable
                        </div>
                      )}
                    </div>
                  ))
                )}
              </div>
            </div>
          </div>
        </div>
      </main>

      {selectedImage ? (
        <button
          type="button"
          onClick={() => setSelectedImage(null)}
          className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 p-4"
        >
          <div className="w-full max-w-5xl rounded-lg border border-red-800 bg-black p-4">
            <div className="mb-3 text-center text-sm font-semibold text-red-300">{selectedImage.name}</div>
            <Image
              src={selectedImage.src}
              alt={selectedImage.name}
              width={1920}
              height={1080}
              unoptimized
              className="max-h-[80vh] w-full rounded-md border border-red-900/60 object-contain"
            />
          </div>
        </button>
      ) : null}
    </div>
  )
}
