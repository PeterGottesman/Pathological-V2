# Build context is frontend/, e.g.:
#   docker build -t pathological-frontend frontend/

FROM node:22-slim AS builder
WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM node:22-slim AS runtime
WORKDIR /app
ENV NODE_ENV=production

# next.config.ts sets output: "standalone", so .next/standalone is a
# self-contained server bundle that only needs public/ and .next/static
# alongside it.
COPY --from=builder /app/.next/standalone ./
COPY --from=builder /app/.next/static ./.next/static
COPY --from=builder /app/public ./public

EXPOSE 3000
ENTRYPOINT ["node", "server.js"]
