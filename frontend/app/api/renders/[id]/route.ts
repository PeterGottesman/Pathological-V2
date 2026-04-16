import { NextRequest, NextResponse } from 'next/server'
//Set scheduler URL
const SCHEDULER_URL = 'http://localhost:8080'
//Define the expected structure of route parameters, basically
type RouteContext = {
  params: { id: string } | Promise<{ id: string }>
}
//GET request to fetch the status of a render job from the scheduler
export async function GET(_: NextRequest, { params }: RouteContext) {
  const resolvedParams = await params
  const id = resolvedParams?.id ?? ''

  if (!id.trim()) {
    return NextResponse.json({ error: 'Missing render id.' }, { status: 400 })
  }

  try {
    const res = await fetch(`${SCHEDULER_URL}/renders/${encodeURIComponent(id)}/status`, {
      method: 'GET',
      cache: 'no-store',
    })

    const data = await res.json()
    return NextResponse.json(data, { status: res.status })
  } catch (error) {
    return NextResponse.json(
      {
        error: 'Could not reach scheduler status endpoint.',
        detail: error instanceof Error ? error.message : 'Unknown error',
      },
      { status: 502 }
    )
  }
}