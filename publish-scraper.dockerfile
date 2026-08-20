FROM debian:bookworm-slim AS builder

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        libcurl4-openssl-dev \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

COPY publish-scraper/CMakeLists.txt ./
COPY publish-scraper/include ./include
COPY publish-scraper/src ./src

RUN cmake \
        -S . \
        -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF \
    && cmake --build build

FROM debian:bookworm-slim AS runner

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libcurl4 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/publish-scraper ./publish-scraper

COPY publish-scraper/data ./data
COPY publish-scraper/config ./config
COPY publish-scraper/sql ./sql

CMD ["./publish-scraper"]