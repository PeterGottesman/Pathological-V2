export interface SubmitRenderRequest { //Request sent from client
  fileName: string
  frames: number
  height: number
  width: number
  samplesPerPixel: number
}

export interface RenderJob { //Response sent from server/scheduler
  id: string
  renderName: string
  status: 'pending' | 'success' | 'error'
  createdAt: string
  executionTime: number
}
