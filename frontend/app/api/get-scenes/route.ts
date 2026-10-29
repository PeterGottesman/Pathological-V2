import { NextRequest, NextResponse } from 'next/server'
import type { SceneListResponse } from '@/types/scheduler'
//Scheduler URL, overridable for containerized/Kubernetes deployments
const SCHEDULER_URL = process.env.SCHEDULER_URL || 'http://localhost:8080'
//GET request to fetch the list of uploaded scenes from the scheduler
export async function GET() {
    try {
    const res = await fetch(`${SCHEDULER_URL}/get-scenes`, {
      method: 'GET',
      cache: 'no-store',
    })
    if (!res.ok) {
      return NextResponse.json(
        { error: 'Scheduler could not list scenes.' },
        { status: res.status }
      )
    }
    const data = (await res.json()) as SceneListResponse
    
    if (data) {
        return NextResponse.json(data, { status: res.status })    
    }
    else {
        return NextResponse.json({}, {status: res.status})
    }
  } catch (error) {
    return NextResponse.json(
      {
        error: 'Could not reach scheduler scene list endpoint.',
        detail: error instanceof Error ? error.message : 'Unknown error',
      },
      { status: 502 }
    )
  }
}