import { NextResponse } from 'next/server'
import { writeFile, readdir, mkdir } from 'fs/promises'
import { join } from 'path'
import { existsSync } from 'fs'

const SCENES_DIR = join(process.cwd(), 'public', 'scenes')

export async function GET() {
  try {
    if (!existsSync(SCENES_DIR)) {
      await mkdir(SCENES_DIR, { recursive: true })
    }
    const files = await readdir(SCENES_DIR)
    const scenes = files.filter(f => f.endsWith('.gltf') || f.endsWith('.glb'))
    return NextResponse.json({ scenes })
  } catch (err) {
    return NextResponse.json({ scenes: [] })
  }
}

export async function POST(req: Request) {
  try {
    const formData = await req.formData()
    const file = formData.get('file') as File | null
    
    if (!file) {
      return NextResponse.json({ message: 'No file uploaded' }, { status: 400 })
    }

    const bytes = await file.arrayBuffer()
    const buffer = Buffer.from(bytes)

    if (!existsSync(SCENES_DIR)) {
      await mkdir(SCENES_DIR, { recursive: true })
    }

    const filePath = join(SCENES_DIR, file.name)
    await writeFile(filePath, buffer)

    return NextResponse.json({ success: true, filename: file.name })
  } catch (error) {
    console.error(error)
    return NextResponse.json({ message: 'Failed to upload scene to server' }, { status: 500 })
  }
}