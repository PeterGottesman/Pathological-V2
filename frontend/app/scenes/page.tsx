'use client'

import { useState } from 'react'
import Link from 'next/link'

export default function ScenesPage() {
  const [file, setFile] = useState<File | null>(null)
  const [uploading, setUploading] = useState(false)

  async function handleUpload(e: React.FormEvent) {
    e.preventDefault()
    if (!file) return

    setUploading(true)
    const formData = new FormData()
    formData.append('scene', file)

    try {
      const res = await fetch('/api/scenes', {
        method: 'POST',
        body: formData,
      })
      if (!res.ok) throw new Error('Upload failed')
      alert('Scene uploaded successfully!')
      setFile(null)
    } catch (err) {
      console.error(err)
      alert('Failed to upload scene.')
    } finally {
      setUploading(false)
    }
  }

  return (
    <div className="flex min-h-screen flex-col bg-black text-red-500 font-mono">
      <header className="w-full border-b border-red-800 bg-black p-6 shadow-[0_0_15px_rgba(220,38,38,0.5)]">
        <div className="mx-auto flex max-w-7xl items-center justify-between">
          <div className="flex flex-col">
            <h1 className="text-3xl font-bold tracking-tighter text-red-500 drop-shadow-[0_0_5px_rgba(220,38,38,0.8)]">
              PATHOLOGICAL V2
            </h1>
            <p className="text-xs text-red-800">SCENE UPLOADER</p>
          </div>
          <Link 
            href="/"
            className="rounded border border-red-800 px-4 py-2 text-sm font-semibold transition-all hover:text-red-200 hover:drop-shadow-[0_0_8px_rgba(220,38,38,0.9)]"
          >
            Back to Dashboard
          </Link>
        </div>
      </header>

      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
        <div className="w-full max-w-2xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
          <h2 className="mb-6 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)]">
            Upload Scene 
          </h2>
          
          <form onSubmit={handleUpload} className="space-y-6">
            <div className="space-y-2">
              <label className="block text-sm font-semibold text-red-300">Select .gltf or .glb file</label>
              <input
                type="file"
                accept=".gltf,.glb"
                onChange={(e) => setFile(e.target.files?.[0] || null)}
                className="block w-full text-sm text-red-200 file:mr-4 file:rounded file:border-0 file:bg-red-900/60 file:px-4 file:py-2 file:text-sm file:font-semibold file:text-red-200 hover:file:bg-red-800/80 cursor-pointer"
              />
            </div>

            <button
              type="submit"
              disabled={!file || uploading}
              className={`w-full rounded-lg border border-red-600 px-4 py-3 font-semibold transition-all ${
                !file || uploading
                  ? 'cursor-not-allowed opacity-60'
                  : 'hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]'
              }`}
            >
              {uploading ? 'Uploading...' : 'Upload Scene'}
            </button>
          </form>
        </div>
      </main>
    </div>
  )
}