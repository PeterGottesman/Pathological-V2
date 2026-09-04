import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // Produces a self-contained server bundle (.next/standalone) for the
  // Docker/Kubernetes deployment in deploy/.
  output: "standalone",
};

export default nextConfig;