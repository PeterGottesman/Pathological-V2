export type SubmitRenderPayload = {
  width: number
  height: number
  frames_per_second: number
  animation_runtime: number
  samples_per_pixel: number
  scene_file_url: string
  output_filename: string
}

export type SubmitRenderRequest = SubmitRenderPayload

export type RenderStatus = 'Completed' | 'In Progress' | 'In Queue' | 'Error'
// What GET /renders sends back
export type RenderJob = {
  id: string | number
  status: RenderStatus
  width: number
  height: number
  frames_per_second: number
  animation_runtime: number
  frames_completed: number
  execution_time: number
  samples_per_pixel: number
  created_at: string
  scene_file_url: string
  output_filename: string
  download_link: string | null
  download_links: string[]
}

export type UploadScenePayload = {
  scene_name?: string
  scene_file_name: string
  scene_data: string
}

export type UploadSceneResponse = {
  scenes: string
}

export type RenderJobList = RenderJob[]
