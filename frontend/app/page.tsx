'use client'
//Submit page for the frontend, contains the form to submit render jobs from the list of currently uploaded scenes
import { useEffect, useState } from 'react'
import type { SubmitRenderPayload, SceneListResponse } from '@/types/scheduler'
import Dropdown, { Option } from './components/Dropdown'
// Creates type for form state based on the SubmitRenderRequest type
type NumberField = 'width' | 'height' | 'frames_per_second' | 'animation_runtime' | 'samples_per_pixel'
type FormState = Omit<SubmitRenderPayload, NumberField> & Record<NumberField, string>

//Constants for animation FPS and runtime
const DEFAULT_FPS = 30
const DEFAULT_RUNTIME = 5



// The main React component for the home page, holds most UI state and logic 
export default function Home() {
  // State for updating dropdown menu
  const [selected, setSelected] = useState<(Option | null)[]>([null])
  const [dropdownOptions, setDropdownOptions] = useState<Option[]>([])
  
  // Function for Updating Dropdown with list of Scenes
  async function getSceneList() {
  try {
    const res = await fetch('/api/get-scenes', {
      method: 'GET',
      cache: 'no-store',
    })
    const data: SceneListResponse = await res.json()
    if (!data) {
      return
    }
    const sceneList: Option[] = Object.entries(data).map(([key, value]: [string, string]) => ({
      label: key,
      id: value
    }))
    setDropdownOptions(sceneList)
  } catch (error) {
    console.error(error)
  }
  }

  // useEffect hook for updating the dropdown with the list scenes whenever the submit renders page is loaded
  useEffect(() => {
    getSceneList();
  }, [])

    //State for managing form inputs
  const [forms, setForms] = useState<FormState[]>([
    {
      width: '1920',
      height: '1080',
      frames_per_second: String(DEFAULT_FPS),
      animation_runtime: String(DEFAULT_RUNTIME),
      samples_per_pixel: '16',
      scene_file_url: '/home/dtre/Pathological-V2/render_worker/test_scenes/cornell_box.gltf',
      output_filename: 'cornell_box.png',
    },
  ])
  //State for managing submission status
  const [submitting, setSubmitting] = useState(false)


  function canSubmit() {
    return submitting || selected.includes(null) || forms.some((form) =>
    [
      form.width,
      form.height,
      form.samples_per_pixel,
      form.animation_runtime,
      form.frames_per_second
    ].some(
      (value) =>
        !/^\d+$/.test(value) ||
      !Number.isSafeInteger(Number(value)) ||
      Number(value) < 1
      )
    )
  }

  function handleSelectedChange(index: number, option: Option) {
    setSelected((prev) => {
      const next = [...prev]
      next[index] = option
      return next
    })
    const scene_filename = option.id ?? ''
    const base = scene_filename.replace(/\.[^/.]+$/, '')
    const output_filename = base ? `${base}.png` : 'frontend_render.png'

    setForms((prev) => {
      const next = [...prev]
      next[index] = {
        ...next[index],
        scene_file_url: scene_filename,
        output_filename: output_filename,
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

  function onNumberChange(index: number, key: NumberField, value: string) {
    setForms((prev) => {
      const next = [...prev]
      next[index] = { ...next[index], [key]: value }
      return next
    })
  }

  function onAdd() {
    setSelected((prev) => [
      ...prev, null
    ])
    setForms((prev) => [
      ...prev,
      {
        width: '1920',
        height: '1080',
        frames_per_second: String(DEFAULT_FPS),
        animation_runtime: String(DEFAULT_RUNTIME),
        samples_per_pixel: '16',
        scene_file_url: '',
        output_filename: '',
      },
    ])
  }

  function onRemove(index: number) {
    setSelected((prev) => prev.filter((_, i) => i !== index))
    setForms((prev) => prev.filter((_, i) => i !== index))
  }


//Handler for submitting the render job form; sends a POST request to the /api/renders endpoint for each form
  async function onSubmit() {
    try {
      setSubmitting(true)

      await Promise.all(
        forms.map(async (form) => {
          const payload: SubmitRenderPayload = {
            ...form,
            width: Number(form.width),
            height: Number(form.height),
            samples_per_pixel: Number(form.samples_per_pixel),
            animation_runtime: Number(form.animation_runtime),
            frames_per_second: Number(form.frames_per_second),
          }

          const res = await fetch('/api/renders', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
          })

          if (!res.ok) {
            throw new Error(`Submit failed with status ${res.status}`)
          }
        })
      )
    } catch (error) {
      console.error(error)
    } finally {
      setSubmitting(false)
    }
  }

  
//UI rendering logic
  return (
    <div className="flex min-h-screen flex-col bg-black text-red-500 font-mono">
      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
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
                  <label className="block text-sm font-semibold text-red-300">Scene file</label>
                  <Dropdown 
                    options={dropdownOptions}
                    selected={selected[i]}
                    onSelectedChanges={(e) => handleSelectedChange(i, e)}
                  />
                </div>

                <div className="grid grid-cols-1 gap-4 md:grid-cols-2">
                  <div className="space-y-2">
                    <label className="block text-sm font-semibold text-red-300">Name</label>
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

                  <div className="space-y-2">
                    <label className="block text-sm font-semibold text-red-300">Animation Runtime (Frames)</label>
                    <input
                      type="number"
                      min={1}
                      step={1}
                      value={form.animation_runtime}
                      onChange={(e) => onNumberChange(i, 'animation_runtime', e.target.value)}
                      className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                    />
                  </div>

                  <div className="space-y-2">
                    <label className="block text-sm font-semibold text-red-300">Frames Per Second</label>
                    <input
                      type="number"
                      min={1}
                      step={1}
                      value={form.frames_per_second}
                      onChange={(e) => onNumberChange(i, 'frames_per_second', e.target.value)}
                      className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-700 focus:outline-none focus:ring-2 focus:ring-red-700"
                    />
                  </div>
                </div>
              </div>
            ))}

            <button
              onClick={onSubmit}
              disabled={canSubmit()}
              className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all ${
                canSubmit()
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
      </main>
    </div>
  )
}