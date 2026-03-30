export type SubmitRenderRequest = {
  width: number
  height: number
  frames_per_second: number
  animation_runtime: number
  samples_per_pixel: number
  scene_file_url: string
  output_file_name: string
}

export type RenderStatus = 'Completed' | 'In Progress' | 'In Queue' | 'Error'
<<<<<<< Updated upstream

export type RenderJob = {
  id: number
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
}

=======

export type RenderJob = {
  id: number
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
}
>>>>>>> Stashed changes
