import { NextResponse } from 'next/server'
import { writeFile, readdir, mkdir } from 'fs/promises'
import path from 'path'

// Define local upload directory
const UPLOAD_DIR = path.join(process.cwd(), 'public', 'scenes')

export async function GET() {
  try {
    // Ensure directory exists to prevent errors on first load
    await mkdir(UPLOAD_DIR, { recursive: true })
    
    // Read the folder and return only gltf/glb files
    const files = await readdir(UPLOAD_DIR)
    const scenes = files.filter(f => f.endsWith('.gltf') || f.endsWith('.glb'))
    
    return NextResponse.json({ scenes })
  } catch (error) {
    console.error('Failed to read scenes directory:', error)
    return NextResponse.json({ scenes: [] })
  }
}

export async function POST(request: Request) {
  try {
    const formData = await request.formData()
    const file = formData.get('scene') as File | null

    if (!file) {
      return NextResponse.json({ error: 'No file uploaded' }, { status: 400 })
    }

    // Convert the file into a Node.js Buffer
    const bytes = await file.arrayBuffer()
    const buffer = Buffer.from(bytes)

    // Clean the filename to prevent path traversal issues
    const filename = file.name.replace(/[^a-zA-Z0-9.\-_]/g, '')
    const filepath = path.join(UPLOAD_DIR, filename)

    // Ensure the upload directory exists, then save the file
    await mkdir(UPLOAD_DIR, { recursive: true })
    await writeFile(filepath, buffer)

    return NextResponse.json({ success: true, filename })
  } catch (error) {
    console.error('Upload error:', error)
    return NextResponse.json({ error: 'Failed to upload scene' }, { status: 500 })
  }
}