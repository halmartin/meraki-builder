# syntax=docker/dockerfile:1
FROM ubuntu:18.04

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
    bc \
    binutils \
    bison ncurses-dev \
    build-essential \
    ca-certificates \
    cpio \
    file \
    flex \
    git \
    make \
    perl \
    python3 \
    rsync \
    u-boot-tools \
    unzip \
    wget \
    xxd

ENV BUILDROOT_VERSION 2021.02.8

RUN mkdir -p /src/buildroot && \
  wget -qO - https://www.buildroot.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.bz2 | \
  tar jxf - -C /src/buildroot --strip-components=1

WORKDIR /src/buildroot
