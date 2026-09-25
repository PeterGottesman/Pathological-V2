import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import "./global.css";
import TabNav from "./components/TabNav";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin"],
});

const geistMono = Geist_Mono({
  variable: "--font-geist-mono",
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: "Pathological V2",
  description: "Vulkan Path Tracer",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body
        className={`${geistSans.variable} ${geistMono.variable} antialiased`}
      >
        <header className="w-full border-b border-red-800 bg-black p-6 shadow-[0_0_15px_rgba(220,38,38,0.5)]">
          <div className="mx-auto flex max-w-7xl flex-col items-center justify-between gap-4 md:flex-row">
            <div className="flex flex-col">
              <h1 className="text-3xl font-bold tracking-tighter text-red-500 drop-shadow-[0_0_5px_rgba(220,38,38,0.8)]">
                PATHOLOGICAL V2
              </h1>
              <p className="text-xs text-red-800">BACS Fall 2026 Capstone</p>
            </div>
          </div>
        </header>
        <TabNav />
        {children}
      </body>
    </html>
  );
}
