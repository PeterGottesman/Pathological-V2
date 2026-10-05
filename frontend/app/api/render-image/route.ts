import { NextRequest, NextResponse } from 'next/server'
import path from 'node:path'
import fs from 'node:fs'

const S3_PUBLIC_BASE_URL =
  process.env.S3_PUBLIC_BASE_URL ||
  'https://pathological-capstone-s3-bucket.s3.us-east-2.amazonaws.com'

function encodeS3Key(key: string) {
  return key
    .split('/')
    .map((segment) => encodeURIComponent(segment))
    .join('/')
}

function toS3ObjectUrl(image: string) {
  const raw = image.trim()
  const baseUrl = S3_PUBLIC_BASE_URL.replace(/\/+$/, '')
  const key = raw.replace(/^(https?:\/\/|\/+)/i, '')
  return `${baseUrl}/${encodeS3Key(key)}`
}

function getContentType(fileName: string) {
  const ext = path.extname(fileName).toLowerCase()
  if (ext === '.png') return 'image/png'
  if (ext === '.jpg' || ext === '.jpeg') return 'image/jpeg'
  if (ext === '.webp') return 'image/webp'
  return 'application/octet-stream'
}

export async function GET(req: NextRequest) {
  const url = new URL(req.url)
  const download = url.searchParams.get('download')
  const image = url.searchParams.get('image')

  if (!image?.trim()) {
    return NextResponse.json({ error: 'Missing image name.' }, { status: 400 })
  }

  const fileName = path.basename(image)
  const fileNameForDownload = path.basename(download?.trim() || fileName)
  const contentType = getContentType(fileNameForDownload)
  const dispositionType = download?.trim() ? 'attachment' : 'inline'

  // 1. Attempt to fetch from AWS S3
  try {
    const remoteUrl = toS3ObjectUrl(image)
    const remoteRes = await fetch(remoteUrl, { method: 'GET', cache: 'no-store' })
    if (remoteRes.ok && remoteRes.body) {
      const s3ContentType = remoteRes.headers.get('content-type') || contentType
      return new NextResponse(remoteRes.body, {
        status: 200,
        headers: {
          'Content-Type': s3ContentType,
          'Content-Disposition': `${dispositionType}; filename="${fileNameForDownload.replace(/"/g, '')}"`,
          'Cache-Control': 'no-store',
        },
      })
    }
  } catch {
    // If S3 fetch errors out, fall through to local fallback
  }

  // 2. Local Fallback: Check public/renders/ for local dummy/testing files
  const localFilePath = path.join(process.cwd(), 'public', 'renders', fileName)
  if (fs.existsSync(localFilePath)) {
    const fileBuffer = fs.readFileSync(localFilePath)
    return new NextResponse(fileBuffer, {
      status: 200,
      headers: {
        'Content-Type': contentType,
        'Content-Disposition': `${dispositionType}; filename="${fileNameForDownload.replace(/"/g, '')}"`,
        'Cache-Control': 'no-store',
      },
    })
  }

  return NextResponse.json({ error: 'Rendered image not found.' }, { status: 404 })
}