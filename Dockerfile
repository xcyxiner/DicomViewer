# Dockerfile
FROM ubuntu:24.04
# 避免交互式询问
ENV DEBIAN_FRONTEND=noninteractive
# 安装所有系统依赖（从你 workflow 中汇总）
RUN --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    cmake \
    libx11-dev \
    libxcb1-dev \
    libxkbcommon-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxrender-dev \
    libxi-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxinerama-dev \
    libfontconfig1-dev \
    libfreetype-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*
RUN --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update && apt-get install -y \
    autoconf \
    autoconf-archive \
    automake \
    libtool \
    '^libxcb.*-dev' \
    libx11-xcb-dev \
    libxkbcommon-x11-dev \
    libegl1-mesa-dev \
    clang-format-18 \
    clang-tidy-18 \
    cppcheck \
    lcov \
    clang-18 \
    doxygen \
    git \
    curl \
    wget \
    curl zip unzip tar \
    nodejs \
    npm \
    && rm -rf /var/lib/apt/lists/*
RUN --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update && apt-get install -y \
    sudo \
    libwayland-dev \
    libxt-dev \
    && rm -rf /var/lib/apt/lists/*
# 3. 克隆并引导 vcpkg（固定在 /opt/vcpkg）
WORKDIR /opt
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH=/opt/vcpkg:$PATH
# 4. 复制项目的 vcpkg.json（并锁定文件，如果有）
#   注意：Dockerfile 必须放在项目根目录，且 vcpkg.json 在同一目录
WORKDIR /workspace
COPY vcpkg.json vcpkg.lock* ./
COPY triplets ./triplets
# 4. 复制预下载的源码包（如果存在）
#    在项目根目录创建 vcpkg-cache 文件夹，放入 *.tar.gz, *.zip 等
COPY vcpkg-cache/downloads/* /opt/vcpkg/downloads/
ENV VCPKG_MAX_CONCURRENCY=2
# 5. 预安装项目依赖（利用 Docker 层缓存）
#    如果 vcpkg.json 未变，此层会复用缓存，极大加速
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/opt/vcpkg/packages \
    vcpkg install --triplet x64-linux --overlay-triplets=./triplets --recurse \
    && mkdir -p /opt/vcpkg-cache \
    && cp -r /root/.cache/vcpkg/archives/* /opt/vcpkg-cache/
ENV VCPKG_BINARY_SOURCES=clear;files,/opt/vcpkg-cache,readwrite
ENV CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
ENV VCPKG_ROOT=/opt/vcpkg