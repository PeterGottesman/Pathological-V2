import { NextRequest, NextResponse } from 'next/server'
import type { SubmitRenderRequest, RenderJob } from '@/types/scheduler'

const SCHEDULER_URL = 'http://localhost:8080'

export async function POST(req: NextRequest) {
  try {
    const body = (await req.json()) as SubmitRenderRequest

    const schedulerRes = await fetch(`${SCHEDULER_URL}/renders`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
      cache: 'no-store',
    })

    const text = await schedulerRes.text()
    let data: any = null
    try {
      data = text ? JSON.parse(text) : null
    } catch {
      data = { error: text || 'Non-JSON response from scheduler' }
    }

    return NextResponse.json(data, { status: schedulerRes.status })
  } catch {
    return NextResponse.json({ error: 'Failed to submit render job' }, { status: 500 })
  }
}

export async function GET(req: NextRequest) {
  try {
    const url = new URL(req.url)
    const id = url.searchParams.get('id')

    if (!id) {
      return NextResponse.json({ error: 'Missing id query param' }, { status: 400 })
    }

    const schedulerRes = await fetch(`${SCHEDULER_URL}/renders/${encodeURIComponent(id)}/status`, {
      method: 'GET',
      cache: 'no-store',
    })

    const text = await schedulerRes.text()
    let data: any = null
    try {
      data = text ? JSON.parse(text) : null
    } catch {
      data = { error: text || 'Non-JSON response from scheduler' }
    }

    return NextResponse.json(data, { status: schedulerRes.status })
  } catch {
    return NextResponse.json({ error: 'Failed to fetch render job' }, { status: 500 })
  }
}