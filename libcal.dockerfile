FROM node:22.21.0-alpine as builder
WORKDIR /app

# Install dependencies
COPY libcal/package.json libcal/package-lock.json ./
RUN npm ci

# Transpile
COPY libcal/src ./src
COPY libcal/tsconfig.json ./
RUN npm run build

FROM node:22.21.0-alpine as runner
ENV NODE_ENV production
WORKDIR /app

# Install production dependencies
COPY libcal/package.json libcal/package-lock.json ./
RUN npm ci

# Copy expected file structures
# sql should be in working directory of project
# Root sql should be sibling of project directory
COPY --from=builder /app/dist ./dist
COPY libcal/sql ./sql
COPY sql ../sql

CMD npm start
