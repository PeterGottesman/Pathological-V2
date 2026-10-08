'use client'
import { useState, useRef } from 'react'

export default function UploadScene() {
  const [file, setFile] = useState<File | null>(null)
  const [scenename, setScenename] = useState('') //
  const [uploading, setUploading] = useState(false)
  const [message, setMessage] = useState('')
  const fileInputRef = useRef<HTMLInputElement>(null)

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selected = e.target.files?.[0]
    
    if (selected) {
      if (selected.name.toLowerCase().endsWith('.gltf')) {
        setFile(selected)
        setMessage('')
      } else {
        setFile(null)
        setMessage('Invalid file type. Please select a .gltf file.')
        if (fileInputRef.current) fileInputRef.current.value = ''
      }
    }
  }

  const handleUpload = async (e: React.FormEvent) => {
    e.preventDefault()
    if (!file) return

    setUploading(true)
    setMessage('')

    // Placeholder: for now we simulate a 2-second server upload that just leads to nothing, will change it in the future
    await new Promise((resolve) => setTimeout(resolve, 2000))

    setMessage(`Success: ${file.name} uploaded! (Mock)`)
    setUploading(false)
    setFile(null)
    if (fileInputRef.current) fileInputRef.current.value = ''
  }

  return (
    <div className="flex min-h-screen flex-col bg-black text-red-500 font-mono">
      <main className="flex flex-1 flex-col items-center justify-center p-6 md:p-24">
        <div className="w-full max-w-2xl rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
          <h2 className="mb-6 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-3xl text-center">
            Upload Scene File
          </h2>

          <form onSubmit={handleUpload} className="space-y-6">
            <div className="space-y-2">
              <label className="block text-sm font-semibold text-red-300">
                Scene Name
              </label>
              <input
                type="text"
                value={scenename}
                onChange={(e) => setScenename(e.target.value)}
                placeholder="Enter scene name"
                className="w-full rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 placeholder:text-red-400 focus:outline-none focus:ring-2 focus:ring-red-700"
              />
            </div>
            
            <div className="space-y-2">
              <label className="block text-sm font-semibold text-red-300">
                Select .gltf File
              </label>
              <input
                type="file"
                accept=".gltf"
                ref={fileInputRef}
                onChange={handleFileChange}
                className="w-full cursor-pointer rounded-lg border border-red-800 bg-black px-3 py-2 text-red-200 file:mr-4 file:rounded file:border-0 file:bg-red-900/60 file:px-4 file:py-2 file:text-sm file:font-semibold file:text-red-200 hover:file:bg-red-800 focus:outline-none focus:ring-2 focus:ring-red-700"
              />
            </div>

            {message && (
              <div className={`text-sm font-semibold ${message.includes('Success') ? 'text-green-500' : 'text-red-500'}`}>
                {message}
              </div>
            )}

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