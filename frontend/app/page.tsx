'use client'

import { useEffect, useRef, useState } from 'react'
import Image from 'next/image'
import type { SubmitRenderPayload, RenderJob } from '@/types/scheduler'

type FormState = SubmitRenderPayload
type JobView = RenderJob & { imageUrl: string | null }
type SelectedImage = { src: string; name: string }
type Tab = 'submit' | 'renders' | 'about'

const POLL_INTERVAL_SECONDS = 10
const POLL_INTERVAL_MS = POLL_INTERVAL_SECONDS * 1000
const DEFAULT_FPS = 30
const DEFAULT_RUNTIME = 10

// URL Resolvers
function buildImageApiUrl(outputFilename: string) {
  return `/api/render-image?image=${encodeURIComponent(outputFilename)}&ts=${Date.now()}`
}

function resolveApiImageUrl(outputFilename: string, downloadLink: string | null, animationRuntime: number) {
  if (downloadLink) {
    const key = normalizeDownloadKey(downloadLink)
    if (key) return key
  }
  const base = toFileName(outputFilename).trim()
  if (!base) return null
  return `${base}_${Math.trunc(animationRuntime)}`
}

function toFileName(value: string) {
  return value.split('/').pop() ?? value
}

function ensurePngName(name: string) {
  const trimmed = toFileName(name).trim()
  if (!trimmed) return 'render.png'
  return /\.png$/i.test(trimmed) ? trimmed : `${trimmed}.png`
}

function normalizeDownloadKey(downloadLink: string) {
  const raw = downloadLink.trim()
  if (raw.startsWith('s3://')) {
    const withoutScheme = raw.slice('s3://'.length)
    const slashIndex = withoutScheme.indexOf('/')
    if (slashIndex < 0) return null
    return withoutScheme.slice(slashIndex + 1)
  }
  if (raw.startsWith('http://') || raw.startsWith('https://')) return raw
  return raw
}

function resolveImageUrl(outputFilename: string, downloadLink: string | null, animationRuntime: number) {
  const key = resolveApiImageUrl(outputFilename, downloadLink, animationRuntime)
  return key ? buildImageApiUrl(key) : null
}

export default function Home() {
  const [activeTab, setActiveTab] = useState<Tab>('submit')

  const [forms, setForms] = useState<FormState[]>([{
    width: 1920, height: 1080, frames_per_second: DEFAULT_FPS,
    animation_runtime: DEFAULT_RUNTIME, samples_per_pixel: 16,
    scene_file_url: '', output_filename: 'cornell_box.png',
  }])
  
  const [submitting, setSubmitting] = useState(false)
  const [jobs, setJobs] = useState<JobView[]>([])
  const [secondsUntilNextPoll, setSecondsUntilNextPoll] = useState(POLL_INTERVAL_SECONDS)
  const [selectedImage, setSelectedImage] = useState<SelectedImage | null>(null)
  const [error, setError] = useState<string | null>(null)

  const [availableScenes, setAvailableScenes] = useState<string[]>([])
  const [uploadingScene, setUploadingScene] = useState(false)
  const [uploadStatus, setUploadStatus] = useState<string | null>(null)

  const jobsRef = useRef<JobView[]>([])

  useEffect(() => { loadScenes() }, [])
  useEffect(() => { jobsRef.current = jobs }, [jobs])

  async function loadScenes() {
    try {
      const res = await fetch('/api/scenes')
      if (res.ok) {
        const data = await res.json()
        setAvailableScenes(data.scenes || [])
      }
    } catch (err) {
      console.error('Failed to load scenes', err)
    }
  }

  // Handle direct file upload from the Submit box
  async function handleInlineUpload(e: React.ChangeEvent<HTMLInputElement>, formIndex: number) {
    const file = e.target.files?.[0]
    if (!file) return

    try {
      setUploadingScene(true)
      setUploadStatus(`Uploading ${file.name}...`)

      const formData = new FormData()
      formData.append('file', file)

      const res = await fetch('/api/scenes', { method: 'POST', body: formData })
      if (!res.ok) {
        const errData = await res.json().catch(() => ({}))
        throw new Error(errData.message || `Status ${res.status}`)
      }

      const data = await res.json()
      await loadScenes()
      
      // Auto-select the newly uploaded file in the form
      onSceneChange(formIndex, data.filename || file.name)
      setUploadStatus(`Success: ${file.name} uploaded`)
      setTimeout(() => setUploadStatus(null), 3000)
    } catch (err) {
      setUploadStatus(`Error: ${err instanceof Error ? err.message : 'Failed to upload'}`)
    } finally {
      setUploadingScene(false)
      e.target.value = '' // reset input
    }
  }

  function onSceneChange(index: number, value: string) {
    const v = value ?? ''
    const last = v.split('/').pop() || ''
    const base = last.replace(/\.[^/.]+$/, '')
    const filename = base ? `${base}.png` : 'frontend_render.png'

    setForms((prev) => {
      const next = [...prev]
      next[index] = { ...next[index], scene_file_url: v, output_filename: filename }
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
    setForms((prev) => [...prev, {
      width: 1920, height: 1080, frames_per_second: DEFAULT_FPS,
      animation_runtime: DEFAULT_RUNTIME, samples_per_pixel: 16,
      scene_file_url: '', output_filename: '',
    }])
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

  async function onOpenImage(job: JobView) {
    const src = resolveImageUrl(job.output_filename, job.download_link, job.animation_runtime)
    if (!src) return
    setSelectedImage({ src, name: job.output_filename })
  }

  function onDelete(index: number, outputFilename: string) {
    setJobs((prev) => prev.filter((_, i) => i !== index))
    setSelectedImage((prev) => (prev?.name === outputFilename ? null : prev))
  }

  async function onSubmit() {
    try {
      setSubmitting(true)
      setError(null)
      const results = await Promise.all(
        forms.map(async (form) => {
          const payload: SubmitRenderPayload = { ...form, frames_per_second: DEFAULT_FPS, animation_runtime: DEFAULT_RUNTIME }
          const res = await fetch('/api/renders', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
          })
          if (!res.ok) throw new Error(`Submit failed with status ${res.status}`)
          const data = (await res.json()) as RenderJob
          const imageUrl = resolveImageUrl(data.output_filename, data.download_link, data.animation_runtime)
          return { ...data, imageUrl } as JobView
        })
      )
      setJobs(results)
      setSecondsUntilNextPoll(POLL_INTERVAL_SECONDS)
      setActiveTab('renders') // Auto-switch to renders tab
    } catch (err) {
      setError(err instanceof Error ? err.message : 'An error occurred during submission.')
    } finally {
      setSubmitting(false)
    }
  }

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
      if (!isCancelled) setSecondsUntilNextPoll(remaining)
    }, 1000)

    const pollInterval = setInterval(async () => {
      try {
        const currentJobs = jobsRef.current
        const hasPollableJobs = currentJobs.some((job) => {
          const id = String(job.id ?? '').trim()
          return id !== '' && (job.status !== 'Completed' && job.status !== 'Error' || (job.status !== 'Error' && job.imageUrl === null))
        })

        if (!hasPollableJobs) return

        const updated = await Promise.all(
          currentJobs.map(async (job) => {
            if (job.status === 'Error') return job
            const id = String(job.id ?? '').trim()
            if (!id) return job

            const currentImageUrl = resolveImageUrl(job.output_filename, job.download_link, job.animation_runtime)
            if (job.status === 'Completed') return { ...job, imageUrl: currentImageUrl } as JobView

            try {
              const res = await fetch(`/api/renders/${encodeURIComponent(String(job.id))}`, { method: 'GET', cache: 'no-store' })
              if (!res.ok) return { ...job, imageUrl: currentImageUrl } as JobView
              const data = (await res.json()) as RenderJob
              const imageUrl = resolveImageUrl(data.output_filename, data.download_link, data.animation_runtime)
              return { ...data, imageUrl } as JobView
            } catch {
              return { ...job, imageUrl: currentImageUrl } as JobView
            }
          })
        )
        if (!isCancelled) setJobs(updated)
      } catch (err) {
        setError('Failed to poll for job updates.')
      }
    }, POLL_INTERVAL_MS)

    return () => {
      isCancelled = true
      clearInterval(countdownInterval)
      clearInterval(pollInterval)
    }
  }, [jobs.length])

  const activeJobCount = jobs.filter((j) => j.status !== 'Error' && !(j.status === 'Completed' && j.imageUrl !== null)).length

  return (
    <div className="flex min-h-screen flex-col">
      <header className="w-full border-b border-red-800 bg-black px-6 py-4 shadow-[0_0_15px_rgba(220,38,38,0.5)]">
        <div className="mx-auto flex max-w-7xl flex-col items-center justify-between gap-4 md:flex-row">
          <div className="flex flex-col">
            <h1 className="text-2xl font-bold tracking-tighter text-red-500 drop-shadow-[0_0_5px_rgba(220,38,38,0.8)]">
              PATHOLOGICAL V2
            </h1>
            <p className="text-xs text-red-800">BACS Fall 2026 Capstone</p>
          </div>
          <nav className="flex flex-wrap items-center gap-3">
            <button onClick={() => setActiveTab('submit')} className={`rounded border border-red-800 px-4 py-2 text-sm font-semibold transition-all ${activeTab === 'submit' ? 'bg-red-700 text-black shadow-[0_0_10px_rgba(220,38,38,0.8)]' : 'bg-black text-red-400 hover:bg-red-900/30'}`}>Submit Render</button>
            <button onClick={() => setActiveTab('renders')} className={`rounded border border-red-800 px-4 py-2 text-sm font-semibold transition-all ${activeTab === 'renders' ? 'bg-red-700 text-black shadow-[0_0_10px_rgba(220,38,38,0.8)]' : 'bg-black text-red-400 hover:bg-red-900/30'}`}>View Renders</button>
            <button onClick={() => setActiveTab('about')} className={`rounded border border-red-800 px-4 py-2 text-sm font-semibold transition-all ${activeTab === 'about' ? 'bg-red-700 text-black shadow-[0_0_10px_rgba(220,38,38,0.8)]' : 'bg-black text-red-400 hover:bg-red-900/30'}`}>About</button>
          </nav>
        </div>
      </header>

      <main className="flex flex-1 flex-col items-center p-6 md:p-12">
        <div className="w-full max-w-6xl">
          {activeTab === 'submit' && (
            <div className="grid grid-cols-1 gap-6 lg:grid-cols-2">
              <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
                <h2 className="mb-4 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">Submit Render Job</h2>
                <div className="space-y-5">
                  {forms.map((form, i) => (
                    <div key={i} className="relative space-y-5 rounded-lg border border-red-900/60 bg-black/20 p-4">
                      {i > 0 && <button onClick={() => onRemove(i)} className="absolute right-2 top-2 rounded border border-red-700 px-2 py-0.5 text-xs font-bold text-red-300 hover:text-red-100">X</button>}
                      
                      {/* INTEGRATED UPLOAD BOX */}
                      <div className="space-y-2">
                        <label className="block text-sm font-semibold text-red-300">Scene file (.gltf)</label>
                        <div className="flex gap-2">
                          <select value={form.scene_file_url} onChange={(e) => onSceneChange(i, e.target.value)} className="flex-1 rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 focus:ring-2 focus:ring-red-700 appearance-none">
                            <option value="" disabled className="text-red-700">Select an uploaded scene...</option>
                            {availableScenes.map((scene) => <option key={scene} value={scene}>{scene}</option>)}
                          </select>
                          <label className={`flex cursor-pointer items-center justify-center rounded-lg border border-red-600 px-4 py-2 text-sm font-semibold transition-all ${uploadingScene ? 'opacity-50 cursor-not-allowed' : 'hover:bg-red-900/30 hover:text-red-200'}`}>
                            {uploadingScene ? '...' : 'Upload'}
                            <input type="file" accept=".gltf,.glb,.bin,.json" className="hidden" onChange={(e) => handleInlineUpload(e, i)} disabled={uploadingScene} />
                          </label>
                        </div>
                        {uploadStatus && <p className="text-xs text-red-400 mt-1">{uploadStatus}</p>}
                      </div>

                      <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                        <div className="space-y-2">
                          <label className="block text-sm font-semibold text-red-300">Output filename</label>
                          <input value={form.output_filename} onChange={(e) => onTextChange(i, 'output_filename', e.target.value)} className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 focus:ring-2 focus:ring-red-700" />
                        </div>
                        <div className="space-y-2">
                          <label className="block text-sm font-semibold text-red-300">Samples / Pixel</label>
                          <input type="number" min={1} value={form.samples_per_pixel} onChange={(e) => onNumberChange(i, 'samples_per_pixel', e.target.value)} className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 focus:ring-2 focus:ring-red-700" />
                        </div>
                        <div className="space-y-2">
                          <label className="block text-sm font-semibold text-red-300">Width</label>
                          <input type="number" min={1} value={form.width} onChange={(e) => onNumberChange(i, 'width', e.target.value)} className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 focus:ring-2 focus:ring-red-700" />
                        </div>
                        <div className="space-y-2">
                          <label className="block text-sm font-semibold text-red-300">Height</label>
                          <input type="number" min={1} value={form.height} onChange={(e) => onNumberChange(i, 'height', e.target.value)} className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 focus:ring-2 focus:ring-red-700" />
                        </div>
                      </div>
                    </div>
                  ))}

                  <button onClick={onSubmit} disabled={submitting} className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all ${submitting ? 'cursor-not-allowed opacity-60' : 'hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]'}`}>
                    {submitting ? 'Submitting…' : 'Submit'}
                  </button>
                  <button onClick={onAdd} className="w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]">
                    Add Form
                  </button>
                </div>
              </div>

              <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
                <h2 className="mb-4 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">Renders</h2>
                <div className="space-y-4 rounded-lg border border-red-800 bg-black/50 p-4">
                  <div className="flex items-center gap-3 rounded-lg border border-red-900/60 bg-black/40 px-3 py-2 text-sm text-red-200">
                    <div className="relative flex h-10 w-10 items-center justify-center">
                      {activeJobCount > 0 ? <div className="absolute inset-0 animate-spin rounded-full border-2 border-red-900 border-t-red-400" /> : <div className="absolute inset-0 rounded-full border-2 border-red-900" />}
                      <span className="relative z-10 text-xs font-semibold">{secondsUntilNextPoll}</span>
                    </div>
                    <span>{activeJobCount > 0 ? `Next poll in ${secondsUntilNextPoll}s` : 'No active jobs to poll'}</span>
                  </div>
                  {error ? (
                    <div className="rounded-lg border border-red-600/80 bg-red-950/40 p-4 text-red-200">
                      <p className="font-bold text-red-400">⚠️ Failed to load jobs</p><p className="mt-1 text-xs text-red-300/80">{error}</p>
                    </div>
                  ) : jobs.length === 0 ? (
                    <div className="rounded-lg border border-dashed border-red-900/60 bg-black/30 p-6 text-center">
                      <p className="text-sm font-semibold text-red-400">No render jobs submitted yet</p>
                    </div>
                  ) : (
                    <div className="space-y-3">
                      {jobs.map((job) => <div key={job.id} className="rounded border border-red-900/50 bg-black/60 p-3 text-sm text-red-300">Render Job #{job.id} - {job.status}</div>)}
                    </div>
                  )}
                </div>
              </div>
            </div>
          )}

          {activeTab === 'renders' && (
            <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
              <h2 className="mb-4 text-2xl font-bold text-red-500">All Submitted Render Jobs</h2>
              <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                {jobs.length === 0 ? (
                  <div className="col-span-2 rounded-lg border border-red-900/60 bg-black/40 p-12 text-center text-red-400">No active or past render jobs found.</div>
                ) : (
                  jobs.map((job, idx) => (
                    <div key={`${job.id}-${idx}`} className="rounded-lg border border-red-900/60 bg-black/50 p-4 space-y-3">
                      <div className="flex items-center justify-between"><span className="font-bold text-red-300">Job #{job.id}</span><span className="text-xs px-2 py-0.5 rounded border border-red-800 bg-black text-red-400">{job.status}</span></div>
                      <p className="text-xs text-red-400">File: {job.output_filename}</p>
                      {job.imageUrl && (
                        <div className="h-40 w-full rounded border border-red-900/60 overflow-hidden bg-black">
                          <Image src={job.imageUrl} alt={job.output_filename} width={500} height={300} unoptimized className="h-full w-full object-contain" />
                        </div>
                      )}
                    </div>
                  ))
                )}
              </div>
            </div>
          )}

          {activeTab === 'about' && (
            <div className="mx-auto max-w-2xl rounded-xl border border-red-800 bg-black/40 p-8 shadow-[0_0_20px_rgba(220,38,38,0.25)] text-center space-y-6">
              <h2 className="text-3xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">PATHOLOGICAL V2</h2>
              <p className="text-sm text-red-400">BACS CAPSTONE: TEAM 19</p>
              <div className="grid grid-cols-2 gap-4 text-sm text-red-300 border-t border-b border-red-900/60 py-6">
                <div>Dontre Quarles</div><div>Kobie Morales</div><div>Hunter Ellenberger</div><div>Austin Johnson</div>
              </div>
            </div>
          )}
        </div>
      </main>
    </div>
  )
}