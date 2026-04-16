import { NextRequest, NextResponse } from 'next/server'
import type { SubmitRenderPayload } from '@/types/scheduler'
//Set scheduler URL
const SCHEDULER_URL = 'http://localhost:8080'
//POST request to submit a render job to the scheduler
export async function POST(req: NextRequest) {
  const body = (await req.json()) as SubmitRenderPayload

  const schedulerRes = await fetch(`${SCHEDULER_URL}/renders`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
    cache: 'no-store',
  })
  //Return the scheduler's response directly to the client
  const data = await schedulerRes.json()
  return NextResponse.json(data, { status: schedulerRes.status })
}