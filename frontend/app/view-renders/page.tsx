export default function ViewRendersPage() {
  // Mock data only for UI purposes. In the future, this will be replaced with actual data fetched from the backend.
  const mockJobs = [
    { id: 1, name: 'render_001', scene: 'cornell_box.gltf', params: '256 spp, 1920x1080', status: 'Complete' },
  ];

  return (
    <div className="min-h-screen bg-black p-10 font-mono text-red-500">
      <h1 className="mb-8 text-3xl font-bold uppercase tracking-widest text-red-600 drop-shadow-[0_0_8px_rgba(220,38,38,0.8)]">
        Render Jobs
      </h1>

      <div className="flex flex-col gap-5">
        {mockJobs.map((job) => (
          <div 
            key={job.id} 
            className="flex items-center justify-between rounded border border-red-900/50 bg-black p-6 transition-all hover:border-red-500 hover:shadow-[0_0_15px_rgba(220,38,38,0.4)]"
          >
            {/* Job Information Columns */}
            <div className="flex gap-16">
              <div className="w-40">
                <span className="mb-1 block text-xs text-red-800">Name</span>
                <span className="text-red-300">{job.name}</span>
              </div>
              <div className="w-48">
                <span className="mb-1 block text-xs text-red-800">Scene File</span>
                <span className="text-red-300">{job.scene}</span>
              </div>
              <div className="w-48">
                <span className="mb-1 block text-xs text-red-800">Parameters</span>
                <span className="text-red-300">{job.params}</span>
              </div>
              <div className="w-32">
                <span className="mb-1 block text-xs text-red-800">Status</span>
                <span className="text-red-300">[{job.status}]</span>
              </div>
            </div>
            
            {/* 
              Note: These are just  placeholder buttons for now. I'll implement the actual functionality later when I connect the backend and have real data to work with.
          
            */}
            <div className="flex gap-4">
              <button className="rounded border border-red-800 px-6 py-2 text-sm font-bold transition hover:bg-red-900/40 hover:text-red-300">
                View Details
              </button>
              <button className="rounded border border-red-800 px-6 py-2 text-sm font-bold transition hover:bg-red-900/40 hover:text-red-300">
                View Output
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}