FROM ubuntu:19.04

# Install build dependencies
RUN apt-get update -qq && \
  apt-get install -qq \
  autoconf \
  curl \
  fuse3 \
  g++ \
  git \
  libcurl4-openssl-dev \
  libfuse3-dev \
  libjsoncpp-dev \
  libmicrohttpd-dev \
  libtinyxml2-dev \
  libtool \
  make \
  pkg-config \
  python \
  wget

# Build application
WORKDIR libcloudstorage
ADD . .
RUN ./bootstrap && \
  ./configure --with-curl --with-microhttpd --with-fuse && \
  make -j$(nproc) && \
  make install

ENV LD_LIBRARY_PATH=/usr/local/lib
