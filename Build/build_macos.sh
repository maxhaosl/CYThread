#!/usr/bin/env bash
set -euo pipefail

# Build CYThread for macOS per-architecture (x86_64, arm64) in both
# Debug/Release and static/shared variants, then merge into universal binaries.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${ROOT_DIR}/Build/out/macos"
ARCH_BIN_ROOT="${ROOT_DIR}/Bin/macOS"
UNIVERSAL_BIN_ROOT="${ARCH_BIN_ROOT}/universal"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"

ARCHES=("x86_64" "arm64")
CONFIGS=("Debug" "Release")
LIB_TYPES=("shared" "static")

log() {
    printf "[macOS] %s\n" "$*"
}

artifact_name() {
    local lib_type="$1"
    if [[ "${lib_type}" == "shared" ]]; then
        echo "libCYThread.dylib"
    else
        echo "libCYThread.a"
    fi
}

build_arch_variant() {
    local arch="$1"
    local lib_type="$2"
    local shared_flag="$3"
    local config="$4"

    local build_dir="${BUILD_ROOT}/${arch}/${lib_type}/${config}"
    local bin_dir="${ARCH_BIN_ROOT}/${arch}/${config}"
    local artifact
    artifact="$(artifact_name "${lib_type}")"

    log "Configuring ${arch} ${lib_type} ${config}"
    cmake -S "${ROOT_DIR}" -B "${build_dir}" \
        -G "${GENERATOR}" \
        -DCMAKE_BUILD_TYPE="${config}" \
        -DCYTHREAD_BUILD_SHARED="${shared_flag}" \
        -DCMAKE_OSX_ARCHITECTURES="${arch}"

    log "Building ${arch} ${lib_type} ${config}"
    cmake --build "${build_dir}"

    mkdir -p "${bin_dir}"
    cp "${build_dir}/${artifact}" "${bin_dir}/${artifact}"
}

combine_universal() {
    local lib_type="$1"
    local config="$2"
    local artifact
    artifact="$(artifact_name "${lib_type}")"
    local universal_dir="${UNIVERSAL_BIN_ROOT}/${config}"
    local universal_path="${universal_dir}/${artifact}"

    mkdir -p "${universal_dir}"

    local inputs=()
    for arch in "${ARCHES[@]}"; do
        local arch_path="${ARCH_BIN_ROOT}/${arch}/${config}/${artifact}"
        if [[ ! -f "${arch_path}" ]]; then
            log "Missing ${artifact} for ${arch} ${config}, cannot create universal binary."
            exit 1
        fi
        inputs+=("${arch_path}")
    done

    log "Creating universal ${lib_type} ${config}"
    lipo -create "${inputs[@]}" -output "${universal_path}"
}

mkdir -p "${ARCH_BIN_ROOT}"

for arch in "${ARCHES[@]}"; do
    for lib in "${LIB_TYPES[@]}"; do
        shared_flag="ON"
        [[ "${lib}" == "static" ]] && shared_flag="OFF"
        for cfg in "${CONFIGS[@]}"; do
            build_arch_variant "${arch}" "${lib}" "${shared_flag}" "${cfg}"
        done
    done
done

for lib in "${LIB_TYPES[@]}"; do
    for cfg in "${CONFIGS[@]}"; do
        combine_universal "${lib}" "${cfg}"
    done
done

log "Per-arch libraries placed under ${ARCH_BIN_ROOT}/{x86_64,arm64}, universal outputs under ${UNIVERSAL_BIN_ROOT}"

