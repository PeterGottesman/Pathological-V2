import { NextRequest, NextResponse } from 'next/server'
import type { SceneListResponse } from '@/types/scheduler' 
//Scheduler URL, overridable for containerized/Kubernetes deployments
const SCHEDULER_URL = process.env.SCHEDULER_URL || 'http://localhost:8080'
//GET request to fetch the status of a render job from the scheduler

type Option = {
  label: string;
  id: string;
}

export async function GET(req: NextRequest) {
  
  const dummyOptions: Option[] = [{
        label: 'Caesar\'s assassination',
        id: '0'
      },
      {
        label: 'Marty McFly arrives in 1955',
        id: '1'
      },
      {
        label: 'Secret third thing',
        id: '2'
  }]

  try {
    const res = await fetch(`${SCHEDULER_URL}/get-scenes`, {
      method: 'GET',
      cache: 'no-store',
    })

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