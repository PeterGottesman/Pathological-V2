'use client'

import { SubmitRenderRequest, RenderJob } from "@/types/scheduler"
import { useMemo, useState } from 'react'

type FormState = SubmitRenderRequest

export default function Home() {
  const [form, setForm] = useState<FormState>({
    fileName: '',
    frames: 1,
    width: 1920,
    height: 1080,
    samplesPerPixel: 16,
  })

  const [selectedFile, setSelectedFile] = useState<File | null>(null)
  const [submitting, setSubmitting] = useState(false)
  const [resp, setResp] = useState<RenderJob | null>(null)
  const [error, setError] = useState<string | null>(null)

  const canSubmit = useMemo(
    () => !!form.fileName && form.frames > 0 && form.width > 0 && form.height > 0 && form.samplesPerPixel > 0,
    [form]
  )

  function onFileChange(file: File | null) {
    setSelectedFile(file)
    setForm((prev) => ({ ...prev, fileName: file?.name ?? '' }))
  }

  function onNumberChange(key: keyof FormState, value: string) {
    setForm((prev) => ({ ...prev, [key]: Number(value) as any }))
  }

  async function onSubmit() {
    setSubmitting(true)
    setError(null)


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
                type="file"
                accept=".gltf,.glb,.obj"
                onChange={(e) => onFileChange(e.target.files?.[0] ?? null)}
                className="block w-full text-sm text-red-300
                           file:mr-4 file:rounded-lg file:border file:border-red-700
                           file:bg-black file:px-4 file:py-2 file:text-red-400
                           hover:file:text-red-200 hover:file:drop-shadow-[0_0_8px_rgba(220,38,38,0.9)]
                           file:transition-all"
              />

              <div className="text-xs text-red-400">
                Selected: <span className="text-red-300">{selectedFile?.name ?? 'None'}</span>
              </div>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Frames</label>
                <input
                  type="number"
                  min={1}
                  value={form.frames}
                  onChange={(e) => onNumberChange('frames', e.target.value)}
                  className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200
                             placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                />
              </div>

              <div className="space-y-2">
                <label className="block text-sm font-semibold text-red-300">Samples / Pixel</label>
                <input
                  type="number"
                  min={1}
                  value={form.samplesPerPixel}
                  onChange={(e) => onNumberChange('samplesPerPixel', e.target.value)}
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

            <div className="rounded-lg border border-red-900 bg-black/60 p-4">
              <div className="text-sm font-semibold text-red-300 mb-2">Scheduler Response</div>

              {!resp && !error && (
                <div className="text-sm text-red-400">No response yet.</div>
              )}

              {error && (
                <div className="text-sm text-red-200">
                  <span className="font-semibold text-red-300">Error:</span> {error}
                </div>
              )}

              {resp && (
                <div className="text-sm text-red-200 space-y-1">
                  <div>
                    <span className="font-semibold text-red-300">renderName:</span> {resp.renderName}
                  </div>
                  <div>
                    <span className="font-semibold text-red-300">status:</span> {resp.status}
                  </div>
                </div>
              )}
            </div>

          </div>
        </div>
      </main>
    </div>
  )
}
