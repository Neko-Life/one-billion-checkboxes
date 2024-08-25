FROM archlinux:base as init

RUN pacman -Sy --needed --noconfirm openssl reflector && reflector --save /etc/pacman.d/mirrorlist && \
      pacman -Syu --needed --noconfirm libc++

FROM init as build

# Build dependencies
WORKDIR /app

# Copy source files
COPY include ./include
COPY src ./src
COPY libs ./libs
COPY CMakeLists.txt ./

# Install dependencies
RUN pacman -Syu --needed --noconfirm base-devel libc++ cmake clang

RUN mkdir -p build && cd build && \
      export CC=clang && \
      export CXX=clang++ && \
      export LDFLAGS='-flto -stdlib=libc++ -lc++' && \
      export CFLAGS='-flto' && \
      export CXXFLAGS='-flto -stdlib=libc++' && \
      cmake .. && make all -j12

FROM init as deploy

RUN groupadd atcboxes && useradd -m -g atcboxes atcboxes

USER atcboxes

WORKDIR /home/atcboxes

COPY --chown=atcboxes:atcboxes --from=build \
             /app/build/atcboxes \
             /home/atcboxes/

ENV LD_LIBRARY_PATH=/home/atcboxes

CMD ./atcboxes
