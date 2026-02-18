FROM node:20.11.1-alpine as builder
WORKDIR /app

# Install dependencies
COPY nss/package.json nss/package-lock.json ./
RUN npm ci

# Transpile
COPY nss/src ./src
COPY nss/tsconfig.json ./
RUN npm run build

FROM node:20.11.1-alpine as runner
ENV NODE_ENV production
WORKDIR /app

# Install production dependencies
COPY nss/package.json nss/package-lock.json ./
RUN npm ci

# Copy expected file structures
# JSON and sql should be in working directory of project
# Root sql should be sibling of project directory
COPY --from=builder /app/dist ./dist
COPY nss/nss_data ./nss_data
COPY nss/buildingOverrides.json ./
COPY nss/roomOverrides.json ./
COPY nss/sql ./sql
COPY sql ../sql

CMD npm start
