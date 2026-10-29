'use client';

import { useEffect, useState } from 'react';
import type { RenderJobList } from '@/types/scheduler';
// How often to refresh the list (ms)
const POLL_INTERVAL_MS = 3000;

// One label and value in the details panel
function DetailRow({ label, value }: { label: string; value: string | number }) {
  return (
    <div>
      <span className="mb-1 block text-xs text-red-800">{label}</span>
      <span className="break-all text-red-300">{value}</span>
    </div>
  );
}

// Show the date in the viewer's local time, or the raw text if it isn't a date
function formatTime(value: string) {
  const date = new Date(value);
  return Number.isNaN(date.getTime()) ? value : date.toLocaleString();
}

export default function ViewRendersPage() {
  const [jobs, setJobs] = useState<RenderJobList>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  // Which job's details are open (by id, so the refresh doesn't close it)
    const [detailsJobId, setDetailsJobId] = useState<string | number | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function loadJobs() {
      try {
        const res = await fetch('/api/renders', { cache: 'no-store' });
        if (!res.ok) throw new Error(`Request failed (${res.status})`);
        const data = (await res.json()) as RenderJobList;
        if (!cancelled) {
          setJobs(data);
          setError(null);
        }
      } catch (err) {
        if (!cancelled) {
          setError(err instanceof Error ? err.message : 'Could not load render jobs.');
        }
      } finally {
        if (!cancelled) setLoading(false);
      }
    }
    // Load once right away, then keep checking
    loadJobs();
    const timer = setInterval(loadJobs, POLL_INTERVAL_MS);
    // Stop checking when we leave the page
    return () => {
      cancelled = true;
      clearInterval(timer);
    };
  }, []);

  return (
    <div className="min-h-screen bg-black p-10 font-mono text-red-500">
      <h1 className="mb-8 text-3xl font-bold uppercase tracking-widest text-red-600 drop-shadow-[0_0_8px_rgba(220,38,38,0.8)]">
        Render Jobs
      </h1>

      {error && <p className="mb-5 text-red-400">Could not load render jobs: {error}</p>}
      {loading && <p className="text-red-800">Loading...</p>}
      {!loading && !error && jobs.length === 0 && (
        <p className="text-red-800">No render jobs yet.</p>
      )}

      <div className="flex flex-col gap-5">
        {/* One card per job */}
        {jobs.map((job) => (
          <div key={job.id}>
            <div className="flex items-center justify-between rounded border border-red-900/50 bg-black p-6 transition-all hover:border-red-500 hover:shadow-[0_0_15px_rgba(220,38,38,0.4)]">
              {/* Job Information Columns */}
              <div className="flex gap-16">
                <div className="w-40">
                  <span className="mb-1 block text-xs text-red-800">Name</span>
                  <span className="break-all text-red-300">{job.output_filename}</span>
                </div>
                <div className="w-48">
                  <span className="mb-1 block text-xs text-red-800">Scene File</span>
                  <span className="break-all text-red-300">{job.scene_file_url}</span>
                </div>
                <div className="w-48">
                  <span className="mb-1 block text-xs text-red-800">Parameters</span>
                  <span className="text-red-300">
                    {job.samples_per_pixel} spp, {job.width}x{job.height}
                  </span>
                </div>
                <div className="w-32">
                  <span className="mb-1 block text-xs text-red-800">Status</span>
                  <span className="text-red-300">[{job.status}]</span>
                </div>
              </div>

              <div className="flex gap-4">
                <button
                  onClick={() => setDetailsJobId(job.id === detailsJobId ? null : job.id)}
                  className="rounded border border-red-800 px-6 py-2 text-sm font-bold transition hover:bg-red-900/40 hover:text-red-300"
                >
                  View Details
                </button>
                {/* View Output doesn't do anything yet */}
                <button className="rounded border border-red-800 px-6 py-2 text-sm font-bold transition hover:bg-red-900/40 hover:text-red-300">
                  View Output
                </button>
              </div>
            </div>

            {/* Full details for this job */}
            {job.id === detailsJobId && (
              <div className="mt-2 grid grid-cols-2 gap-6 rounded border border-red-900/50 bg-black p-6 md:grid-cols-4">
                <DetailRow label="Job ID" value={job.id} />
                <DetailRow label="Created" value={formatTime(job.created_at)} />
                <DetailRow label="Execution Time" value={job.execution_time} />
                <DetailRow label="Frames Completed" value={job.frames_completed} />
                <DetailRow label="Resolution" value={`${job.width}x${job.height}`} />
                <DetailRow label="Samples Per Pixel" value={job.samples_per_pixel} />
                <DetailRow label="Frames Per Second" value={job.frames_per_second} />
                <DetailRow label="Animation Runtime" value={job.animation_runtime} />
                <DetailRow label="Scene File" value={job.scene_file_url} />
              </div>
            )}
          </div>
        ))}
      </div>
    </div>
  );
}