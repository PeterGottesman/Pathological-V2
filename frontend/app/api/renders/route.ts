//Next.js API route for submitting a render job to the scheduler
import { NextRequest, NextResponse } from 'next/server'
import type { SubmitRenderPayload, RenderJob } from '@/types/scheduler'
//Set scheduler URL
const SCHEDULER_URL = 'http://localhost:8080'
//POST request to submit a render job to the scheduler
export async function POST(req: NextRequest) {
  //Parse the request body as JSON and type it as SubmitRenderPayload
  const body = (await req.json()) as SubmitRenderPayload
  //Store HTTP response from the scheduler after making a POST request to the /renders endpoint with the render job details in the request body
  const schedulerRes = await fetch(`${SCHEDULER_URL}/renders`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
    cache: 'no-store',
  })
  //Parse the response from scheduler as JSON and cast type as RenderJob
  const data = (await schedulerRes.json()) as RenderJob
  return NextResponse.json(data, { status: schedulerRes.status })
}
