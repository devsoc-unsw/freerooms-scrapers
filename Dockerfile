FROM node:18.16.0-alpine as builder
WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY . .
RUN npm run build

FROM node:18.16.0-alpine as runner
ENV NODE_ENV production
WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY --from=builder /app/dist ./dist
COPY --from=builder /app/buildingLocations.json ./

CMD npm start
