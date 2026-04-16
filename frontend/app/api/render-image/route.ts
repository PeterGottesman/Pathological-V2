import { NextRequest, NextResponse } from 'next/server'
import path from 'node:path'
const S3_PUBLIC_BASE_URL = 'https://pathological-capstone-s3-bucket.s3.us-east-2.amazonaws.com'
//Helper function to encode S3 object keys while preserving slashes
function encodeS3Key(key: string) {
  return key
    .split('/')
    .map((segment) => encodeURIComponent(segment))
    .join('/')
}
//Take an image name/path and convert it to a full S3 URL for fetching
function toS3ObjectUrl(image: string) {
  const raw = image.trim()
  if (raw.startsWith('http://') || raw.startsWith('https://')) {
    return raw
  }

  const baseUrl = S3_PUBLIC_BASE_URL.replace(/\/+$/, '')
  const key = raw.replace(/^\/+/, '')
  return `${baseUrl}/${encodeS3Key(key)}`
}
//Helper function to determine content type based on file extension, defaulting to binary if unknown
function getContentType(fileName: string) {
  const ext = path.extname(fileName).toLowerCase()
  if (ext === '.png') return 'image/png'
  return 'application/octet-stream'
}
//GET request to fetch a rendered image from S3, with optional download behavior based on query parameters
export async function GET(req: NextRequest) {
  const url = new URL(req.url)
  const download = url.searchParams.get('download')
  const image = url.searchParams.get('image')

  if (!image?.trim()) {
    return NextResponse.json({ error: 'Missing image name.' }, { status: 400 })
  }

  const fileName = path.basename(image)
  const remoteUrl = toS3ObjectUrl(image)

  try {
    const remoteRes = await fetch(remoteUrl, {
      method: 'GET',
      cache: 'no-store',
    })

    if (!remoteRes.ok || !remoteRes.body) {
      return NextResponse.json({ error: 'Rendered image not found.' }, { status: 404 })
    }

    const fileNameForDownload = path.basename(download?.trim() || fileName)
    const contentType = remoteRes.headers.get('content-type') || getContentType(fileNameForDownload)
    const dispositionType = download?.trim() ? 'attachment' : 'inline'

    return new NextResponse(remoteRes.body, {
      status: 200,
      headers: {
        'Content-Type': contentType,
        'Content-Disposition': `${dispositionType}; filename="${fileNameForDownload}"`,
        'Cache-Control': 'no-store',
      },
    })
  } catch {
    return NextResponse.json({ error: 'Rendered image not found.' }, { status: 404 })
  }
}
