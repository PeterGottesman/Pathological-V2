'use client'

import { useMemo, useState } from 'react'
import type { SubmitRenderRequest, RenderJob } from '@/types/scheduler'

type FormState = SubmitRenderRequest

export default function Home() {
  const [form, setForm] = useState<FormState>({
    id: 1,
    width: 1920,
    height: 1080,
    frames_per_second: 30,
    animation_runtime: 10,
    samples_per_pixel: 16,
    scene_file_url: 'https://example.com/scene.gltf',
    output_filename: 'frontend_render.png',
  })

  const [submitting, setSubmitting] = useState(false)
  const [resp, setResp] = useState<RenderJob | null>(null)
  const [statusMsg, setStatusMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null)

  const [getId, setGetId] = useState<string>('')

  const canSubmit = useMemo(() => {
    return (
      Number.isFinite(form.id) &&
      form.id >= 0 &&
      !!form.scene_file_url &&
      form.width > 0 &&
      form.height > 0 &&
      form.frames_per_second > 0 &&
      form.animation_runtime > 0 &&
      form.samples_per_pixel > 0 &&
      !!form.output_filename
    )
  }, [form])

  function onSceneChange(value: string) {
    const v = value ?? ''
    const last = v.split('/').pop() || ''
    const base = last.replace(/\.[^/.]+$/, '')
    const filename = base ? `${base}.png` : 'frontend_render.png'

    setForm((prev) => ({
      ...prev,
      scene_file_url: v,
      output_filename: filename,
    }))
  }

  function onTextChange(key: keyof FormState, value: string) {
    setForm((prev) => ({ ...prev, [key]: value as any }))
  }

  function onNumberChange(key: keyof FormState, value: string) {
    setForm((prev) => ({ ...prev, [key]: Number(value) as any }))
  }

  async function onSubmit() {
    if (!canSubmit) return

    setSubmitting(true)
    setStatusMsg(null)
    setResp(null)

    try {
      const res = await fetch('/api/renders', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(form),
      })

      const data = await res.json()

      if (!res.ok) {
        setStatusMsg({ type: 'error', text: data?.error || 'Request failed' })
        return
      }

      setResp(data)
      setStatusMsg({ type: 'success', text: 'Render job submitted successfully.' })
    } catch (err: any) {
      setStatusMsg({ type: 'error', text: err?.message || 'Something went wrong' })
    } finally {
      setSubmitting(false)
    }
  }

  async function onGetById() {
    const id = getId.trim()
    if (!id) return

    setStatusMsg(null)
    setResp(null)

    try {
      const res = await fetch(`/api/renders?id=${encodeURIComponent(id)}`, { method: 'GET' })
      const data = await res.json()

      if (!res.ok) {
        setStatusMsg({ type: 'error', text: data?.error || 'Request failed' })
        return
      }

      setResp(data)
      setStatusMsg({ type: 'success', text: `Fetched render status for id=${id}.` })
    } catch (err: any) {
      setStatusMsg({ type: 'error', text: err?.message || 'Something went wrong' })
    }
  }

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
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">
              Dontre Quarles
            </span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">
              Kobie Morales
            </span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">
              Hunter Ellenberger
            </span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">
              Austin Johnson
            </span>
          </div>
        </div>
      </header>

      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
        <div className="mb-6 w-full max-w-3xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)] center">
          <h2 className="text-xl md:text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
            About
          </h2>
          <p className="mt-3 text-sm md:text-base text-red-200 leading-relaxed">
            Pathological V2 is a distributed render pipeline. You submit a scene (GLTF) to S3 plus render parameters
            (resolution, frames, samples-per-pixel). The scheduler gets the job, dispatches work to render workers, and uploads
            results back to S3.
          </p>
        </div>

        <div className="w-full max-w-3xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
          <h2 className="text-2xl md:text-3xl font-bold mb-2 text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
            Submit Render Job
          </h2>

          <div className="space-y-5">
            <div className="space-y-2">
              <label className="block text-sm font-semibold text-red-300">Scene file (.gltf)</label>

              <input
                placeholder="s3 link to gltf file"
                value={form.scene_file_url}
                onChange={(e) => onSceneChange(e.target.value)}
                className="block w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
              />
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Output filename</label>
                <input
                  value={form.output_filename}
                  onChange={(e) => onTextChange('output_filename', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Request ID</label>
                <input
                  type="number"
                  min={0}
                  value={form.id}
                  onChange={(e) => onNumberChange('id', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Frames / Second</label>
                <input
                  type="number"
                  min={1}
                  value={form.frames_per_second}
                  onChange={(e) => onNumberChange('frames_per_second', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Animation runtime</label>
                <input
                  type="number"
                  min={1}
                  value={form.animation_runtime}
                  onChange={(e) => onNumberChange('animation_runtime', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Samples / Pixel</label>
                <input
                  type="number"
                  min={1}
                  value={form.samples_per_pixel}
                  onChange={(e) => onNumberChange('samples_per_pixel', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Width</label>
                <input
                  type="number"
                  min={1}
                  value={form.width}
                  onChange={(e) => onNumberChange('width', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Height</label>
                <input
                  type="number"
                  min={1}
                  value={form.height}
                  onChange={(e) => onNumberChange('height', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>
            </div>

            <button
              onClick={onSubmit}
              disabled={!canSubmit || submitting}
              className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all
                ${!canSubmit || submitting
                  ? 'cursor-not-allowed opacity-60'
                  : 'hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]'
                }`}
            >
              {submitting ? 'Submitting…' : 'Submit'}
            </button>

            <div className="rounded-lg border border-red-800 bg-black/50 p-4">
              <div className="flex items-center justify-between gap-4">
                <div className="font-semibold text-red-300">Get by ID</div>
              </div>
              <div className="mt-3 flex gap-3">
                <input
                  placeholder="render id"
                  value={getId}
                  onChange={(e) => setGetId(e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
                <button
                  onClick={onGetById}
                  className="shrink-0 rounded-lg border border-red-600 px-4 py-2 font-semibold hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]"
                >
                  Get
                </button>
              </div>
            </div>

            <div className="rounded-lg border border-red-800 bg-black/50 p-4 text-sm text-red-200">
              <div className="flex items-center justify-between gap-4">
                <div className="font-semibold text-red-300">Latest Response</div>
                {statusMsg ? (
                  <span
                    className={`text-xs font-semibold ${
                      statusMsg.type === 'success' ? 'text-green-300' : 'text-red-300'
                    }`}
                  >
                    {statusMsg.type === 'success' ? 'SUCCESS' : 'ERROR'}
                  </span>
                ) : null}
              </div>

              {statusMsg ? <div className="mt-2 text-sm">{statusMsg.text}</div> : <div className="mt-2 text-red-300/70">No response yet.</div>}

              {resp ? (
                <pre className="mt-3 overflow-x-auto whitespace-pre-wrap break-words rounded border border-red-900/60 bg-black/40 p-3">
                  {JSON.stringify(resp, null, 2)}
                </pre>
              ) : null}
            </div>

            <div>
              <h1>Renders</h1>
            </div>
          </div>
        </div>
      </main>
    </div>
  )
}
