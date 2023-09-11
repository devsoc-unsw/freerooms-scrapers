FROM node:20.6.1-alpine as builder
WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY . .
RUN npm run build

FROM node:20.6.1-alpine as runner
ENV NODE_ENV production
WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY --from=builder /app/dist ./dist
COPY --from=builder /app/buildingOverrides.json ./
COPY --from=builder /app/sql ./sql

CMD npm start
