FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake autoconf automake libtool pkg-config git \
    python3 ca-certificates file libncurses-dev && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /project
