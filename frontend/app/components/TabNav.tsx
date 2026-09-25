"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const TABS = [
  { href: "/", label: "Submit Render" },
  { href: "/upload-scene", label: "Upload Scene" },
  { href: "/view-renders", label: "View Renders" },
  { href: "/about", label: "About" },
];

export default function TabNav() {
  const pathname = usePathname();

  return (
    <div className="w-full bg-black px-6 pb-4 pt-3">
      <div className="mx-auto flex max-w-7xl flex-wrap gap-3 font-mono text-sm">
        {TABS.map((tab) => {
          const isActive = pathname === tab.href;
          return (
            <Link
              key={tab.href}
              href={tab.href}
              className={`rounded-lg border border-red-600 px-4 py-2 font-semibold transition-all ${
                isActive
                  ? "bg-red-700/60 text-red-100 drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]"
                  : "bg-black text-red-400 hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]"
              }`}
            >
              {tab.label}
            </Link>
          );
        })}
      </div>
    </div>
  );
}
