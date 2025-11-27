#!/usr/bin/env bash
set -euo pipefail

# Build CYThread for Android ABIs with both static and shared variants.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${ROOT_DIR}/Build/out/android"
BIN_ROOT="${ROOT_DIR}/Bin/Android"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-21}"
ABIS=("arm64-v8a" "armeabi-v7a" "x86_64")
CONFIGS=("Debug" "Release")
LIB_TYPES=("shared" "static")

shopt -s nullglob

log() {
    printf "[Android] %s\n" "$*"
}

detect_android_sdk() {
    local candidates=()
    local home="${HOME:-}"

    [[ -n "${ANDROID_SDK_ROOT:-}" ]] && candidates+=("${ANDROID_SDK_ROOT}")
    [[ -n "${ANDROID_HOME:-}"    ]] && candidates+=("${ANDROID_HOME}")
    [[ -n "${ANDROID_SDK:-}"     ]] && candidates+=("${ANDROID_SDK}")

    if [[ -n "${home}" ]]; then
        candidates+=(
            "${home}/Library/Android/sdk"
            "${home}/Android/Sdk"
            "${home}/android-sdk"
        )
    fi

    candidates+=(
        "/usr/local/share/android-sdk"
        "/usr/local/Caskroom/android-sdk/latest"
        "/usr/lib/android-sdk"
        "/opt/android-sdk"
    )

    for path in "${candidates[@]}"; do
        [[ -n "${path}" && -d "${path}" ]] && { echo "${path}"; return 0; }
    done

    return 1
}

detect_android_ndk() {
    local sdk_root="$1"
    local -a candidates=()

    [[ -n "${ANDROID_NDK_HOME:-}" ]] && candidates+=("${ANDROID_NDK_HOME}")
    [[ -n "${ANDROID_NDK_ROOT:-}" ]] && candidates+=("${ANDROID_NDK_ROOT}")

    if [[ -n "${sdk_root}" ]]; then
        candidates+=("${sdk_root}/ndk-bundle")
        local ndk_dir="${sdk_root}/ndk"
        if [[ -d "${ndk_dir}" ]]; then
            local -a versioned=()
            while IFS= read -r line; do
                versioned+=("${line}")
            done < <(find "${ndk_dir}" -mindepth 1 -maxdepth 1 -type d -print 2>/dev/null | sort -rV || true)
            candidates+=("${versioned[@]}")
        fi
    fi

    for candidate in "${candidates[@]}"; do
        [[ -n "${candidate}" && -d "${candidate}" ]] || continue
        if [[ -f "${candidate}/build/cmake/android.toolchain.cmake" ]]; then
            echo "${candidate}"
            return 0
        fi
    done

    return 1
}

ensure_android_env() {
    if [[ -z "${ANDROID_SDK_ROOT:-}" ]]; then
        local sdk_path
        sdk_path="$(detect_android_sdk || true)"
        if [[ -z "${sdk_path}" ]]; then
            log "Unable to locate Android SDK automatically; set ANDROID_SDK_ROOT"
            return 1
        fi
        export ANDROID_SDK_ROOT="${sdk_path}"
        [[ -z "${ANDROID_HOME:-}" ]] && export ANDROID_HOME="${ANDROID_SDK_ROOT}"
    fi

    local ndk_candidate="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
    if [[ -n "${ndk_candidate}" && ! -f "${ndk_candidate}/build/cmake/android.toolchain.cmake" ]]; then
        log "ANDROID_NDK_HOME points to ${ndk_candidate} but no toolchain file exists. Auto-detecting a valid NDK."
        ndk_candidate=""
    fi

    if [[ -z "${ndk_candidate}" ]]; then
        ndk_candidate="$(detect_android_ndk "${ANDROID_SDK_ROOT}")"
        if [[ -z "${ndk_candidate}" ]]; then
            log "Unable to locate Android NDK under ${ANDROID_SDK_ROOT}; install via SDK Manager or set ANDROID_NDK_HOME"
            return 1
        fi
    fi

    export ANDROID_NDK_HOME="${ndk_candidate}"
    export ANDROID_NDK_ROOT="${ANDROID_NDK_HOME}"

    if [[ ! -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then
        log "Android NDK at ${ANDROID_NDK_HOME} is missing the CMake toolchain file."
        return 1
    fi

    return 0
}

if ! ensure_android_env; then
    log "Skipping Android build because SDK/NDK is not available on this machine."
    exit 0
fi


build_variant() {
    local abi="$1"
    local lib_type="$2"
    local shared_flag="$3"
    local config="$4"

    local build_dir="${BUILD_ROOT}/${abi}/${lib_type}/${config}"
    local bin_dir="${BIN_ROOT}/${abi}/${config}"

    log "Configuring ${abi} ${lib_type} ${config}"
    cmake -S "${ROOT_DIR}" -B "${build_dir}" \
        -G "${GENERATOR}" \
        -DANDROID=ON \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_ANDROID_NDK="${ANDROID_NDK_HOME}" \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="${abi}" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
        -DCMAKE_BUILD_TYPE="${config}" \
        -DCYTHREAD_BUILD_SHARED="${shared_flag}"

    log "Building ${abi} ${lib_type} ${config}"
    cmake --build "${build_dir}"

    mkdir -p "${bin_dir}"
    if [[ "${lib_type}" == "shared" ]]; then
        cp "${build_dir}/libCYThread.so" "${bin_dir}/libCYThread.so"
    else
        cp "${build_dir}/libCYThread.a" "${bin_dir}/libCYThread.a"
    fi
}

mkdir -p "${BIN_ROOT}"

for abi in "${ABIS[@]}"; do
    for lib in "${LIB_TYPES[@]}"; do
        shared_flag="ON"
        [[ "${lib}" == "static" ]] && shared_flag="OFF"
        for cfg in "${CONFIGS[@]}"; do
            build_variant "${abi}" "${lib}" "${shared_flag}" "${cfg}"
        done
    done
done

log "Libraries placed under ${BIN_ROOT}"

