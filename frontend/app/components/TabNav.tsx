"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

type Tab = { href: string; label: string };

// Add a { href, label } entry here to add a tab. Requires a matching
// app/<href>/page.tsx or the tab will 404 — see the tabbed-navigation PR
// description for the full walkthrough.
const TABS: Tab[] = [
  { href: "/", label: "Submit Render" },
  { href: "/upload-scene", label: "Upload Scene" },
  { href: "/view-renders", label: "View Renders" },
  { href: "/about", label: "About" },
];

const BASE_TAB_CLASSES =
  "rounded-lg border border-red-600 px-4 py-2 font-semibold transition-all";
const ACTIVE_TAB_CLASSES =
  "bg-red-700/60 text-red-100 drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]";
const INACTIVE_TAB_CLASSES =
  "bg-black text-red-400 hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]";

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
              aria-current={isActive ? "page" : undefined}
              className={`${BASE_TAB_CLASSES} ${
                isActive ? ACTIVE_TAB_CLASSES : INACTIVE_TAB_CLASSES
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