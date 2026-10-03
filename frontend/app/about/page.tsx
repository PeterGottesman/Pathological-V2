// Static About page: project summary plus team credits, grouped by semester.
export default function About() {
  return (
    <div className="flex min-h-screen flex-col bg-black text-red-500 font-mono">
      <main className="flex flex-1 flex-col items-center p-6 md:p-24">
        <div className="w-full max-w-6xl space-y-6">
          <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
            <h2 className="mb-3 text-2xl font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-3xl">
              About
            </h2>
            <p className="text-sm leading-relaxed text-red-200 md:text-base">
              Pathological V2 is a distributed render pipeline. You submit a scene (GLTF) to the
              scheduler with render parameters (resolution, frames, samples-per-pixel). The
              scheduler takes the job, dispatches work to render workers, and uploads results back
              to S3.
            </p>
          </div>

          <div className="grid grid-cols-1 gap-6 md:grid-cols-2">
            <TeamCard title="Fall 2026 Team" members={FALL_2026_TEAM} />
            <TeamCard title="Spring 2026 Team" members={SPRING_2026_TEAM} />
          </div>
        </div>
      </main>
    </div>
  )
}

const FALL_2026_TEAM = [
    'David Ruiz', 
    'Jose Quintanilla',
    'Will Freeman'
]

const SPRING_2026_TEAM = [
  'Dontre Quarles',
  'Kobie Morales',
  'Hunter Ellenberger',
  'Austin Johnson',
]

function TeamCard({ title, members }: { title: string; members: string[] }) {
  return (
    <div className="rounded-xl border border-red-800 bg-black/40 p-6 shadow-[0_0_20px_rgba(220,38,38,0.25)]">
      <h3 className="mb-3 text-lg font-bold text-red-500 drop-shadow-[0_0_6px_rgba(220,38,38,0.7)] md:text-xl">
        {title}
      </h3>
      <ul className="space-y-2 text-sm text-red-200 md:text-base">
        {members.map((name) => (
          <li
            key={name}
            className="cursor-default transition-all hover:text-red-100 hover:drop-shadow-[0_0_8px_rgba(220,38,38,1)]"
          >
            {name}
          </li>
        ))}
      </ul>
    </div>
  )
}