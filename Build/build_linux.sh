#!/usr/bin/env bash
set -euo pipefail

# Build CYThread for Linux across multiple architectures.
# Cross-compiling requires toolchain files exposed via
#   export CYTHREAD_LINUX_TOOLCHAIN_AARCH64=/path/to/toolchain.cmake

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${ROOT_DIR}/Build/out/linux"
BIN_ROOT="${ROOT_DIR}/Bin/Linux"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"
ARCHS=("x86_64" "aarch64")
CONFIGS=("Debug" "Release")
LIB_TYPES=("shared" "static")
HOST_ARCH="$(uname -m)"

log() {
    printf "[Linux] %s\n" "$*"
}

to_upper() {
    echo "$1" | tr '[:lower:]-' '[:upper:]_'
}

build_variant() {
    local arch="$1"
    local lib_type="$2"
    local shared_flag="$3"
    local config="$4"

    local var_name="CYTHREAD_LINUX_TOOLCHAIN_$(to_upper "${arch}")"
    local toolchain="${!var_name-}"

    if [[ "${arch}" != "${HOST_ARCH}" && -z "${toolchain}" ]]; then
        log "Skipping ${arch} (${config}, ${lib_type}) - missing ${var_name}"
        return
    fi

    local build_dir="${BUILD_ROOT}/${arch}/${lib_type}/${config}"
    local artifact_dir="${BIN_ROOT}/${arch}/${lib_type}/${config}"

    log "Configuring ${arch} ${lib_type} ${config}"
    cmake -S "${ROOT_DIR}" -B "${build_dir}" \
        -G "${GENERATOR}" \
        -DCMAKE_BUILD_TYPE="${config}" \
        -DCYTHREAD_BUILD_SHARED="${shared_flag}" \
        -DCMAKE_SYSTEM_PROCESSOR="${arch}" \
        ${toolchain:+-DCMAKE_TOOLCHAIN_FILE="${toolchain}"}

    log "Building ${arch} ${lib_type} ${config}"
    cmake --build "${build_dir}"

    mkdir -p "${artifact_dir}"
    if [[ "${lib_type}" == "shared" ]]; then
        cp "${build_dir}/libCYThread.so" "${artifact_dir}/libCYThread.so"
    else
        cp "${build_dir}/libCYThread.a" "${artifact_dir}/libCYThread.a"
    fi
}

mkdir -p "${BIN_ROOT}"

for arch in "${ARCHS[@]}"; do
    for lib in "${LIB_TYPES[@]}"; do
        shared_flag="ON"
        [[ "${lib}" == "static" ]] && shared_flag="OFF"
        for cfg in "${CONFIGS[@]}"; do
            build_variant "${arch}" "${lib}" "${shared_flag}" "${cfg}"
        done
    done
done

log "Artifacts placed under ${BIN_ROOT}"

