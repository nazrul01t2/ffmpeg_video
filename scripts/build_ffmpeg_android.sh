#!/usr/bin/env bash
set -euo pipefail

# Build FFmpeg shared libraries for Android ABIs and copy them into:
#   app/src/main/cpp/ffmpeg/<abi>/lib*.so
#   app/src/main/cpp/ffmpeg/include
#
# Usage:
#   export ANDROID_NDK_HOME=$HOME/Android/Sdk/ndk/26.3.11579264
#   ./scripts/build_ffmpeg_android.sh

FFMPEG_VERSION="n7.1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="${ROOT_DIR}/.ffmpeg-build"
OUT_DIR="${ROOT_DIR}/app/src/main/cpp/ffmpeg"

: "${ANDROID_NDK_HOME:?ANDROID_NDK_HOME is required}"

HOST_TAG="linux-x86_64"
TOOLCHAIN="${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/${HOST_TAG}"
SYSROOT="${TOOLCHAIN}/sysroot"

ABIS=("arm64-v8a" "armeabi-v7a" "x86_64")

mkdir -p "${WORK_DIR}" "${OUT_DIR}"

if [ ! -d "${WORK_DIR}/ffmpeg" ]; then
  git clone --depth 1 --branch "${FFMPEG_VERSION}" https://github.com/FFmpeg/FFmpeg.git "${WORK_DIR}/ffmpeg"
fi

build_abi() {
  local abi="$1"
  local arch target cpu cc cxx cross_prefix

  case "${abi}" in
    arm64-v8a)
      arch="aarch64"
      target="aarch64-linux-android"
      cpu="armv8-a"
      ;;
    armeabi-v7a)
      arch="arm"
      target="armv7a-linux-androideabi"
      cpu="armv7-a"
      ;;
    x86_64)
      arch="x86_64"
      target="x86_64-linux-android"
      cpu="x86_64"
      ;;
    *)
      echo "Unsupported ABI: ${abi}" >&2
      exit 1
      ;;
  esac

  local api=24
  local prefix="${WORK_DIR}/install/${abi}"
  local build="${WORK_DIR}/build/${abi}"

  mkdir -p "${prefix}" "${build}"

  cc="${TOOLCHAIN}/bin/${target}${api}-clang"
  cxx="${TOOLCHAIN}/bin/${target}${api}-clang++"
  cross_prefix="${TOOLCHAIN}/bin/${target}-"

  pushd "${WORK_DIR}/ffmpeg" >/dev/null
  make distclean >/dev/null 2>&1 || true

  ./configure \
    --prefix="${prefix}" \
    --target-os=android \
    --arch="${arch}" \
    --cpu="${cpu}" \
    --sysroot="${SYSROOT}" \
    --cc="${cc}" \
    --cxx="${cxx}" \
    --cross-prefix="${cross_prefix}" \
    --nm="${TOOLCHAIN}/bin/llvm-nm" \
    --strip="${TOOLCHAIN}/bin/llvm-strip" \
    --ar="${TOOLCHAIN}/bin/llvm-ar" \
    --ranlib="${TOOLCHAIN}/bin/llvm-ranlib" \
    --enable-cross-compile \
    --enable-shared \
    --disable-static \
    --disable-programs \
    --disable-doc \
    --disable-avdevice \
    --disable-postproc \
    --disable-network

  make -j"$(nproc)"
  make install
  popd >/dev/null

  mkdir -p "${OUT_DIR}/${abi}"
  cp -f "${prefix}/lib/"*.so "${OUT_DIR}/${abi}/"
}

for abi in "${ABIS[@]}"; do
  echo "=== Building ${abi} ==="
  build_abi "${abi}"
done

mkdir -p "${OUT_DIR}/include"
cp -r "${WORK_DIR}/install/arm64-v8a/include/." "${OUT_DIR}/include/"

echo "Done. FFmpeg files are available in ${OUT_DIR}."
