FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    g++ make libcurl4-openssl-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN make

ENV PORT=4000
EXPOSE 4000
VOLUME ["/app/data"]

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
  CMD curl -f http://localhost:4000/health || exit 1

CMD ["./server"]
