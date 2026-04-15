'use client'
//Main and only page for the frontend, contains the form to submit render jobs to the scheduler and a section to display rendered images with options to download or delete them. It also implements polling to update the status of submitted jobs and their rendered images.
import { useEffect, useRef, useState } from 'react'
import type { SubmitRenderRequest, SubmitRenderPayload, RenderJob } from '@/types/scheduler'
// Creates type for form state based on the SubmitRenderRequest type, and a JobView type that extends RenderJob with additional fields for the requested file name and the local image URL. Also defines a SelectedImage type for managing the currently selected image in the UI.
type FormState = SubmitRenderRequest
type JobView = RenderJob & {
  requestedFileName: string
  imageUrl: string | null
}
type SelectedImage = {
  src: string
  name: string
}
//Constants for polling interval
const POLL_INTERVAL_SECONDS = 10
const POLL_INTERVAL_MS = POLL_INTERVAL_SECONDS * 1000
// Helper function to build a local image URL for fetching the rendered image from the render worker's build directory based on the output filename.
function buildLocalImageUrl(outputFilename: string) {
  return `/api/render-image?image=${encodeURIComponent(outputFilename)}&ts=${Date.now()}`
}
//Helper function to resolve the local image URL for a rendered image by checking if the requested file name or the output filename exists in the render worker's build directory. It makes GET requests to the /api/render-image endpoint with the 'exists' query parameter set to '1' to check for the existence.
async function resolveLocalImageUrl(requestedFileName: string, outputFilename: string) {
  const candidates = [requestedFileName, outputFilename].filter(
    (name, index, arr) => name && arr.indexOf(name) === index
  )

  for (const fileName of candidates) {
    const res = await fetch(`/api/render-image?image=${encodeURIComponent(fileName)}&exists=1`, {
      method: 'GET',
      cache: 'no-store',
    })

    if (res.ok) {
      return buildLocalImageUrl(fileName)
    }
  }

  return null
}
// The main React component for the home page, holds most UI state and logic 
export default function Home() {
    //State for managing form inputs
  const [forms, setForms] = useState<FormState[]>([
    {
      width: 1920,
      height: 1080,
      frames_per_second: 30,
      animation_runtime: 10,
      samples_per_pixel: 16,
      scene_file_url: '/home/dtre/Pathological-V2/render_worker/test_scenes/cornell_box.gltf',
      output_file_name: 'cornell_box.png',
    },
  ])
  //States for managing submission status, list of render jobs, countdown for next poll, and currently selected image for viewing
  const [submitting, setSubmitting] = useState(false)
  const [jobs, setJobs] = useState<JobView[]>([])
  const [secondsUntilNextPoll, setSecondsUntilNextPoll] = useState(POLL_INTERVAL_SECONDS)
  const [selectedImage, setSelectedImage] = useState<SelectedImage | null>(null)

  const jobsRef = useRef<JobView[]>([])
  const pollingStartedRef = useRef(false)

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
        output_file_name: filename,
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
        frames_per_second: 30,
        animation_runtime: 10,
        samples_per_pixel: 16,
        scene_file_url: '',
        output_file_name: '',
      },
    ])
  }

  function onDownload(outputFilename: string) {
    const href = `/api/render-image?image=${encodeURIComponent(outputFilename)}`
    const anchor = document.createElement('a')
    anchor.href = href
    anchor.download = outputFilename
    document.body.appendChild(anchor)
    anchor.click()
    anchor.remove()
  }

  async function onDelete(index: number, outputFilename: string) {
    const res = await fetch(`/api/render-image?image=${encodeURIComponent(outputFilename)}`, {
      method: 'DELETE',
    })

    if (!res.ok) {
      return
    }

    setJobs((prev) => prev.filter((_, i) => i !== index))
    setSelectedImage((prev) => (prev?.name === outputFilename ? null : prev))
  }

  async function onSubmit() {
    try {
      setSubmitting(true)

      const results = await Promise.all(
        forms.map(async (form) => {
          const { output_file_name, ...rest } = form

          const payload: SubmitRenderPayload = {
            ...rest,
            output_filename: output_file_name,
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
          const imageUrl = await resolveLocalImageUrl(form.output_file_name, data.output_filename)

          return {
            ...data,
            requestedFileName: form.output_file_name,
            imageUrl,
          } as JobView
        })
      )

      setJobs(results)
      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)
      pollingStartedRef.current = false
    } catch (error) {
      console.error(error)
    } finally {
      setSubmitting(false)
    }
  }

  useEffect(() => {
    if (jobs.length === 0) {
      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)
      pollingStartedRef.current = false
      return
    }

    if (pollingStartedRef.current) return
    pollingStartedRef.current = true

    let isCancelled = false
    let countdownInterval: ReturnType<typeof setInterval> | null = null
    let pollTimeout: ReturnType<typeof setTimeout> | null = null

    const clearTimers = () => {
      if (countdownInterval) clearInterval(countdownInterval)
      if (pollTimeout) clearTimeout(pollTimeout)
    }

    const runPollCycle = () => {
      if (isCancelled) return

      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)

      let remaining = POLL_INTERVAL_SECONDS

      countdownInterval = setInterval(() => {
        remaining -= 1

        if (remaining >= 0 && !isCancelled) {
          setSecondsUntilNextPoll(remaining)
        }

        if (remaining <= 0 && countdownInterval) {
          clearInterval(countdownInterval)
        }
      }, 1000)

      pollTimeout = setTimeout(async () => {
        try {
          const currentJobs = jobsRef.current
          const hasPollableJobs = currentJobs.some((job) => {
            const id = String(job.id ?? '').trim()
            const isTerminal = job.status === 'Completed' || job.status === 'Error'
            const isLocalComplete = job.status !== 'Error' && job.imageUrl !== null
            return id !== '' && !isTerminal && !isLocalComplete
          })

          if (hasPollableJobs) {
            const updated = await Promise.all(
              currentJobs.map(async (job) => {
                const isTerminal = job.status === 'Completed' || job.status === 'Error'
                const isLocalComplete = job.status !== 'Error' && job.imageUrl !== null
                if (isTerminal || isLocalComplete) return job

                const res = await fetch(`/api/renders/${encodeURIComponent(String(job.id))}`, {
                  method: 'GET',
                  cache: 'no-store',
                })

                if (!res.ok) {
                  throw new Error(`Polling failed for id=${String(job.id)} with status ${res.status}`)
                }

                const data = (await res.json()) as RenderJob
                const imageUrl = job.imageUrl ?? (await resolveLocalImageUrl(job.requestedFileName, data.output_filename))

                return {
                  ...data,
                  requestedFileName: job.requestedFileName,
                  imageUrl,
                } as JobView
              })
            )

            if (!isCancelled) {
              setJobs(updated)
            }
          }
        } catch (error) {
          console.error(error)
        } finally {
          if (!isCancelled) {
            runPollCycle()
          }
        }
      }, POLL_INTERVAL_MS)
    }

    runPollCycle()

    return () => {
      isCancelled = true
      pollingStartedRef.current = false
      clearTimers()
    }
  }, [jobs.length])

  const activeJobCount = jobs.filter((job) => {
    const isTerminal = job.status === 'Completed' || job.status === 'Error'
    const isLocalComplete = job.status !== 'Error' && job.imageUrl !== null
    return !isTerminal && !isLocalComplete
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
                <div key={i} className="space-y-5 rounded-lg border border-red-900/60 bg-black/20 p-4">
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
                        value={form.output_file_name}
                        onChange={(e) => onTextChange(i, 'output_file_name', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Frames / Second</label>
                      <input
                        type="number"
                        min={1}
                        value={form.frames_per_second}
                        onChange={(e) => onNumberChange(i, 'frames_per_second', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Animation runtime</label>
                      <input
                        type="number"
                        min={1}
                        value={form.animation_runtime}
                        onChange={(e) => onNumberChange(i, 'animation_runtime', e.target.value)}
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
                          onClick={() => onDownload(job.output_filename)}
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
                          onClick={() => setSelectedImage({ src: job.imageUrl as string, name: job.output_filename })}
                          className="mt-3 block w-full"
                        >
                          <div className="flex h-56 w-full items-center justify-center overflow-hidden rounded-md border border-red-900/60 bg-black/40">
                            <img
                              src={job.imageUrl}
                              alt={job.requestedFileName}
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
            <img
              src={selectedImage.src}
              alt={selectedImage.name}
              className="max-h-[80vh] w-full rounded-md border border-red-900/60 object-contain"
            />
          </div>
        </button>
      ) : null}
    </div>
  )
}
