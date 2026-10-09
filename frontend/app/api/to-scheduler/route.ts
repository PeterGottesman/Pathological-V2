import { NextRequest, NextResponse } from 'next/server'

const SCHEDULER_URL = 'http//localhost:8080'

export async function POST(req: NextRequest) {
    const body = await req.text()

    const schedulerRes = await fetch(`${SCHEDULER_URL}/to-scheduler`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: body,
        cache: 'no-store',
    })
}