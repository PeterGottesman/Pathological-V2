import { NextRequest, NextResponse } from 'next/server'
import type { SubmitRenderRequest, RenderJob } from '@/types/scheduler'

const SCHEDULER_URL = "http://localhost:8000"

export async function POST(req: NextRequest) {
  try {
    const body = (await req.json()) as SubmitRenderRequest

    const schedulerRes = await fetch(`${SCHEDULER_URL}/renders`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    })
    const data = (await schedulerRes.json()) as RenderJob
    return NextResponse.json(data, { status: schedulerRes.status })

  } catch (err) {
    console.error('POST /api/renders error:', err)
    return NextResponse.json({ error: 'Failed to submit render job' }, { status: 500 })
  }
}

export async function GET(req: NextRequest) {
  try {
    const { searchParams } = new URL(req.url)
    const id = searchParams.get('id')
    if (!id) {
      return NextResponse.json({ error: 'Missing id' }, { status: 400 })
    }
    const target = `${SCHEDULER_URL}/renders/${id}/status`
    const schedulerRes = await fetch(target, { method: 'GET'})
    const data = (await schedulerRes.json()) as RenderJob

    return NextResponse.json(data, { status: schedulerRes.status })
  } catch (err) {
    console.error('GET /api/renders error:', err)
    return NextResponse.json({ error: 'Failed to fetch render job' }, { status: 500 })
  }
}