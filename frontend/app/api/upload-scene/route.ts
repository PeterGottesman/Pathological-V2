import { NextRequest, NextResponse } from 'next/server'
import type { UploadScenePayload } from '@/types/scheduler'
//Scheduler URL, overridable for containerized/Kubernetes deployments
const SCHEDULER_URL = process.env.SCHEDULER_URL || 'http://localhost:8080'
//POST request to send GLTF scene file to scheduler, which will in turn send it to S3 
export async function POST(req: NextRequest) {
    const body = (await req.json()) as UploadScenePayload
  
    const schedulerRes = await fetch(`${SCHEDULER_URL}/upload-scene`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
        cache: 'no-store',
    })

    //response from scheduler
    const data = await schedulerRes.json()
    return NextResponse.json(data, { status: schedulerRes.status })    
}