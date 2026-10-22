'use client';

import { useEffect, useState } from 'react';
import type { RenderJobList } from '@/types/scheduler';
// How often to refresh the list (ms)
const POLL_INTERVAL_MS = 3000;

export default function ViewRendersPage() {
  const [jobs, setJobs] = useState<RenderJobList>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

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
          <div
            key={job.id}
            className="flex items-center justify-between rounded border border-red-900/50 bg-black p-6 transition-all hover:border-red-500 hover:shadow-[0_0_15px_rgba(220,38,38,0.4)]"
          >
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

            {/* These buttons don't do anything yet */}
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
