// Request sent from client -> scheduler
export interface SubmitRenderRequest {
  gltf_file_url: string
  frame_count: number
  width: number
  height: number
  samples_per_pixel: number
}

// Response sent from scheduler -> client
export interface RenderJob {
  id: number 
  status: 'Completed' | 'In Progress' | 'In Queue' | 'Error' 
  width: number
  height: number
  frame_count: number
  created_at: string
  execution_time: number
  output_filename: string
  samples_per_pixel: number
  download_link: string | null
}

