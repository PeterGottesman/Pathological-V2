// This API route fetches rendered images from the render worker's build directory, for now until s3 is implemented
import { NextRequest, NextResponse } from 'next/server'
import { access, readFile, unlink } from 'node:fs/promises'
import path from 'node:path'
//Get the content type based on the file extension, currently only supports PNG images; could be expanded in the future if we want to support video creation
function getContentType(fileName: string) {
  const ext = path.extname(fileName).toLowerCase()
  if (ext === '.png') return 'image/png'
  return 'application/octet-stream'
}
//GET request to fetch a rendered image file from the render worker's build directory based on the image name provided in the query parameters. If the 'exists' query parameter is set to '1', it will only check if the file exists and return a JSON response indicating whether it exists or not, without returning the file content. If the file does not exist, it returns a 404 error response.
export async function GET(req: NextRequest) {
  const url = new URL(req.url)
  const image = url.searchParams.get('image')
  const existsOnly = url.searchParams.get('exists') === '1'
    // Validate that the image name is not empty or just whitespace
  if (!image?.trim()) {
    return NextResponse.json({ error: 'Missing image name.' }, { status: 400 })
  }
  //Build the file path to the rendered image in the render worker's build directory
  const fileName = path.basename(image)
  const rootDir = path.resolve(process.cwd(), '..')
  const filePath = path.join(rootDir, 'render_worker', 'build', fileName)
  //Try to access the file if existsOnly is true, else just read the file and return its content with the matching application content type
  try {
    if (existsOnly) {
      await access(filePath)
      return NextResponse.json({ exists: true, file: fileName }, { status: 200 })
    }
    
    const file = await readFile(filePath)

    return new NextResponse(file, {
      status: 200,
      headers: {
        'Content-Type': getContentType(fileName),
        'Cache-Control': 'no-store',
      },
    })
  } catch {
    if (existsOnly) {
      return NextResponse.json({ exists: false, file: fileName }, { status: 404 })
    }

    return NextResponse.json({ error: 'Rendered image not found.' }, { status: 404 })
  }
}
//DELETE request to delete a rendered image file from the render worker's build directory based on the image name provided in the query parameters. It returns a JSON response indicating whether the deletion was successful or if the file was not found.
export async function DELETE(req: NextRequest) {
  const url = new URL(req.url)
  const image = url.searchParams.get('image')

  if (!image?.trim()) {
    return NextResponse.json({ error: 'Missing image name.' }, { status: 400 })
  }

  const fileName = path.basename(image)
  const rootDir = path.resolve(process.cwd(), '..')
  const filePath = path.join(rootDir, 'render_worker', 'build', fileName)

  try {
    await unlink(filePath)
    return NextResponse.json({ deleted: true, file: fileName }, { status: 200 })
  } catch {
    return NextResponse.json({ deleted: false, error: 'Rendered image not found.' }, { status: 404 })
  }
}
