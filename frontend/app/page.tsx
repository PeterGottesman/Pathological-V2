'use client'

import { SubmitRenderRequest, RenderJob } from "@/types/scheduler"
import { useMemo, useState } from 'react'

type FormState = SubmitRenderRequest

export default function Home() {
  const [form, setForm] = useState<FormState>({
    gltf_file_url: '',
    frame_count: 1,
    width: 1920,
    height: 1080,
    samples_per_pixel: 16,
  })

  const [sceneURL, setSceneURL] = useState<string >('')
  const [submitting, setSubmitting] = useState(false)
  const [resp, setResp] = useState<RenderJob | null>(null)
  const [error, setError] = useState<string | null>(null)

  const canSubmit = useMemo(
    () => !!form.gltf_file_url && form.frame_count > 0 && form.width > 0 && form.height > 0 && form.samples_per_pixel > 0,
    [form]
  )

  function onSceneChange(value: string ) {
    setSceneURL(value)
    setForm((prev) => ({ ...prev, sceneURL: value ?? '', fileName: value.split('/').pop() ?? '' }))
  }

  function onNumberChange(key: keyof FormState, value: string) {
    setForm((prev) => ({ ...prev, [key]: Number(value) as any }))
  }

  async function onSubmit() {
    if (!canSubmit) return
  
    setSubmitting(true)
    setError(null)
    setResp(null)
  
    try {
      const res = await fetch('/api/renders', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(form),
      })
  
      const data = await res.json()
  
      if (!res.ok) {
        throw new Error(data?.error || 'Request failed')
      }
  
      setResp(data)
    } catch (err: any) {
      setError(err?.message || 'Something went wrong')
    } finally {
      setSubmitting(false)
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
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">Dontre Quarles</span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">Kobie Morales</span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">Hunter Ellenberger</span>
            <span className="hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)] transition-all cursor-default">Austin Johnson</span>
          </div>
        </div>
      </header>

      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
      {/* About Section */}
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
              <label className="block text-sm font-semibold text-red-300">
                Scene file (.gltf)
              </label>

              <input
                placeholder="s3 link to gltf file"
                value={sceneURL}
                onChange={(e) => onSceneChange(e.target.value)}
                className="block w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-sm text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
              />
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Frames</label>
                <input
                  type="number"
                  min={1}
                  value={form.frame_count}
                  onChange={(e) => onNumberChange('frame_count', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200
                             placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Samples / Pixel</label>
                <input
                  type="number"
                  min={1}
                  value={form.samples_per_pixel}
                  onChange={(e) => onNumberChange('samples_per_pixel', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200
                             placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Width</label>
                <input
                  type="number"
                  min={1}
                  value={form.width}
                  onChange={(e) => onNumberChange('width', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200
                             placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Height</label>
                <input
                  type="number"
                  min={1}
                  value={form.height}
                  onChange={(e) => onNumberChange('height', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200
                             placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>
            </div>

            <button
              onClick={onSubmit}
              disabled={!canSubmit || submitting}
              className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all
                ${(!canSubmit || submitting)
                  ? 'cursor-not-allowed opacity-60'
                  : 'hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]'
                }`}
            >
              {submitting ? 'Submitting…' : 'Submit'}
            </button>
            {/* Renders Section */}
            <div>
              <h1>Renders</h1>
            </div>



          </div>
        </div>
      </main>
    </div>
  )
}
