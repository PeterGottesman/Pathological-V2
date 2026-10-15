import { NextRequest, NextResponse } from 'next/server'

const SCHEDULER_URL = 'http://localhost:8080'

export async function POST(req: NextRequest) {

    const body = await req.json()
  
    const schedulerRes = await fetch(`${SCHEDULER_URL}/to-scheduler`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
        cache: 'no-store',
    })
    //const data = await schedulerRes.json()
    //return NextResponse.json(data, { status: schedulerRes.status })

    return NextResponse.json({
        message: 'Upload Successful'
    }, {status: 200})
}