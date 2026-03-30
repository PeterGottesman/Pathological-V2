import { NextRequest, NextResponse } from 'next/server'
import type { SubmitRenderRequest, RenderJob } from '@/types/scheduler'

const SCHEDULER_URL = 'http://localhost:8080'

export async function POST(req: NextRequest) {
  const body = (await req.json()) as SubmitRenderRequest

  const schedulerRes = await fetch(`${SCHEDULER_URL}/renders`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
    cache: 'no-store',
  })

  const data = (await schedulerRes.json()) as RenderJob
  return NextResponse.json(data, { status: schedulerRes.status })
}

export async function GET(req: NextRequest) {
  const id = new URL(req.url).searchParams.get('id') ?? ''

  const schedulerRes = await fetch(`${SCHEDULER_URL}/renders/${encodeURIComponent(id)}/status`, {
    method: 'GET',
    cache: 'no-store',
  })

  const data = (await schedulerRes.json()) as RenderJob
  return NextResponse.json(data, { status: schedulerRes.status })
<<<<<<< Updated upstream
}
=======
}
>>>>>>> Stashed changes
