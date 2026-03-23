'use client'

import { useState } from 'react'
import type { SubmitRenderRequest, RenderJob } from '@/types/scheduler'

type FormState = SubmitRenderRequest

export default function Home() {
  const [forms, setForms] = useState<FormState[]>([
    {
      id: 1,
      width: 1920,
      height: 1080,
      frames_per_second: 30,
      animation_runtime: 10,
      samples_per_pixel: 16,
      scene_file_url: 'https://example.com/scene.gltf',
      output_file_name: 'frontend_render.png',
    },
  ])

  const [submitting, setSubmitting] = useState(false)
  const [resp, setResp] = useState<RenderJob | RenderJob[] | null>(null)
  const [statusMsg, setStatusMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null)
  const [getId, setGetId] = useState<string>('')

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
      next[index] = { ...next[index], [key]: value as any }
      return next
    })
  }

  function onNumberChange(index: number, key: keyof FormState, value: string) {
    setForms((prev) => {
      const next = [...prev]
      next[index] = { ...next[index], [key]: Number(value) as any }
      return next
    })
  }

  function onAdd() {
    setForms((prev) => [
      ...prev,
      {
        id: prev.length + 1,
        width: 1920,
        height: 1080,
        frames_per_second: 30,
        animation_runtime: 10,
        samples_per_pixel: 16,
        scene_file_url: '',
        output_file_name: 'frontend_render.png',
      },
    ])
  }

  async function onSubmit() {
    setSubmitting(true)
    setStatusMsg(null)
    setResp(null)

    try {
      const results: RenderJob[] = []

      for (const form of forms) {
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

        results.push(data)
      }

      setResp(results)
      setStatusMsg({ type: 'success', text: 'Render jobs submitted.' })
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
        <div className="mb-6 w-full max-w-6xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
          <h2 className="text-xl md:text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
            About
          </h2>
          <p className="mt-3 text-sm md:text-base text-red-200 leading-relaxed">
            Pathological V2 is a distributed render pipeline. You submit a scene (GLTF) to S3 plus render parameters
            (resolution, frames, samples-per-pixel). The scheduler gets the job, dispatches work to render workers, and uploads
            results back to S3.
          </p>
        </div>

        <div className="grid w-full max-w-6xl grid-cols-1 gap-6 lg:grid-cols-2">
          <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
            <h2 className="text-2xl md:text-3xl font-bold mb-2 text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
              Submit Render Job
            </h2>

            <div className="space-y-5">
              {forms.map((form, i) => (
                <div key={i} className="space-y-5 rounded-lg border border-red-900/60 bg-black/20 p-4">
                  <div className="space-y-2">
                    <label className="block text-sm font-semibold text-red-300">Scene file (.gltf)</label>
                    <input
                      placeholder="s3 link to gltf file"
                      value={form.scene_file_url}
                      onChange={(e) => onSceneChange(i, e.target.value)}
                      className="block w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                    />
                  </div>

                  <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Output filename</label>
                      <input
                        value={form.output_file_name}
                        onChange={(e) => onTextChange(i, 'output_file_name', e.target.value)}
                        className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                      />
                    </div>

                    <div className="space-y-2">
                      <label className="block text-sm font-semibold text-red-300">Request ID</label>
                      <input
                        type="number"
                        min={0}
                        value={form.id}
                        onChange={(e) => onNumberChange(i, 'id', e.target.value)}
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
            </div>
          </div>

          <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
            <h2 className="text-2xl md:text-3xl font-bold mb-4 text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
              Renders
            </h2>

            <div className="rounded-lg border border-red-800 bg-black/50 p-4">
              <div className="flex items-center justify-between gap-4">
                <div className="font-semibold text-red-300">Get Status</div>
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

            <div className="mt-5 min-h-[500px] rounded-lg border border-red-800 bg-black/50 p-4">
              <div className="font-semibold text-red-300">Rendered Images</div>

              <div className="mt-4 grid grid-cols-1 gap-4">
                <div className="flex min-h-[180px] items-center justify-center rounded-lg border border-red-900/60 bg-black/40 text-sm text-red-300/70">
                  No rendered images yet.
                </div>
              </div>
            </div>
          </div>
        </div>
      </main>
    </div>
  )
}