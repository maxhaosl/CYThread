#!/usr/bin/env bash
set -euo pipefail

# Build CYThread for iOS (device + simulator) as universal binaries.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${ROOT_DIR}/Build/out/ios"
BIN_ROOT="${ROOT_DIR}/Bin/iOS"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-14.0}"

SDK_MATRIX=(
    "iphoneos:arm64"
    "iphonesimulator:arm64;x86_64"
)

CONFIGS=("Debug" "Release")
LIB_TYPES=("shared" "static")

log() {
    printf "[iOS] %s\n" "$*"
}

build_variant() {
    local sdk="$1"
    local archs="$2"
    local lib_type="$3"
    local shared_flag="$4"
    local config="$5"

    local build_dir="${BUILD_ROOT}/${sdk}/${lib_type}/${config}"
    local bin_dir="${BIN_ROOT}/${sdk}/${config}"

    log "Configuring ${sdk} ${lib_type} ${config}"
    cmake -S "${ROOT_DIR}" -B "${build_dir}" \
        -G "${GENERATOR}" \
        -DCMAKE_BUILD_TYPE="${config}" \
        -DCYTHREAD_BUILD_SHARED="${shared_flag}" \
        -DCMAKE_OSX_SYSROOT="${sdk}" \
        -DCMAKE_OSX_ARCHITECTURES="${archs}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}"

    log "Building ${sdk} ${lib_type} ${config}"
    cmake --build "${build_dir}"

    mkdir -p "${bin_dir}"
    if [[ "${lib_type}" == "shared" ]]; then
        cp "${build_dir}/libCYThread.dylib" "${bin_dir}/libCYThread.dylib"
    else
        cp "${build_dir}/libCYThread.a" "${bin_dir}/libCYThread.a"
    fi
}

mkdir -p "${BIN_ROOT}"

for entry in "${SDK_MATRIX[@]}"; do
    IFS=":" read -r sdk archs <<< "${entry}"
    for lib_type in "${LIB_TYPES[@]}"; do
        shared_flag="ON"
        [[ "${lib_type}" == "static" ]] && shared_flag="OFF"
        for cfg in "${CONFIGS[@]}"; do
            build_variant "${sdk}" "${archs}" "${lib_type}" "${shared_flag}" "${cfg}"
        done
    done
done

create_universal() {
    local lib_type="$1"
    local config="$2"

    local ext="dylib"
    [[ "${lib_type}" == "static" ]] && ext="a"

    local device_lib="${BIN_ROOT}/iphoneos/${config}/libCYThread.${ext}"
    local sim_lib="${BIN_ROOT}/iphonesimulator/${config}/libCYThread.${ext}"
    local output_dir="${BIN_ROOT}/universal/${lib_type}/${config}"
    local output_lib="${output_dir}/libCYThread.${ext}"

    if [[ ! -f "${device_lib}" || ! -f "${sim_lib}" ]]; then
        log "Skipping universal ${lib_type} ${config} (missing device or simulator build)"
        return
    fi

    mkdir -p "${output_dir}"
    log "Creating universal ${lib_type} ${config}"
    lipo -create "${device_lib}" "${sim_lib}" -output "${output_lib}"
}

for lib_type in "${LIB_TYPES[@]}"; do
    for cfg in "${CONFIGS[@]}"; do
        create_universal "${lib_type}" "${cfg}"
    done
done

log "Libraries placed under ${BIN_ROOT} (including universal outputs in ${BIN_ROOT}/universal)"

