// Next.js API route for fetching the status of a specific render job from the scheduler
import { NextRequest, NextResponse } from 'next/server'
// Set scheduler URL
const SCHEDULER_URL = 'http://localhost:8080'
// Define the type for route context which includes params with an id
type RouteContext = {
  params: { id: string } | Promise<{ id: string }>
}
//GET request to fetch the status of a specific render job from the scheduler using the render job id from the route parameters
export async function GET(_: NextRequest, { params }: RouteContext) {
  // Resolve the params to get the render job id  
  const resolvedParams = await params
  //Get the id from the params, defaults to empty string if no id
  const id = resolvedParams?.id ?? ''
  // Validate that the id is not empty or just whitespace
  if (!id.trim()) {
    return NextResponse.json({ error: 'Missing render id.' }, { status: 400 })
  }
  // Make a GET request to the scheduler's /renders/{id}/status endpoint to fetch the status of the render job
  try {
    const res = await fetch(`${SCHEDULER_URL}/renders/${encodeURIComponent(id)}/status`, {
      method: 'GET',
      cache: 'no-store',
    })
    // Check the content type of the response to determine how to parse it
    const contentType = res.headers.get('content-type') ?? ''
    // If the response is JSON, parse it and return it as JSON response
    if (contentType.includes('application/json')) {
      const data = await res.json()
      return NextResponse.json(data, { status: res.status })
    }
    // If the response is not JSON, read it as text and return an error response 
    const text = await res.text()
    return NextResponse.json(
      {
        error: 'Scheduler returned a non-JSON response.',
        status: res.status,
        body: text,
      },
      { status: res.status }
    )
  } catch (error) {
    return NextResponse.json(
      {
        error: 'Could not reach scheduler status endpoint.',
        detail: error instanceof Error ? error.message : 'Unknown error',
      },
      { status: 502 }
    )
  }
}
