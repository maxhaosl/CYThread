#!/usr/bin/env bash
set -euo pipefail

# Run every supported platform build script in sequence to produce all
# available CYThread binaries for the current host environment.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOST_OS="$(uname -s)"

log() {
    printf "[all] %s\n" "$*"
}

run_script() {
    local script_name="$1"
    local path="${SCRIPT_DIR}/${script_name}"

    if [[ ! -f "${path}" ]]; then
        log "Skipping ${script_name} (missing)"
        return
    fi

    log "Running ${script_name}"
    bash "${path}"
}

case "${HOST_OS}" in
    Darwin)
        run_script "build_macos.sh"
        run_script "build_ios.sh"
        ;;
    Linux)
        run_script "build_linux.sh"
        ;;
    *)
        log "Host ${HOST_OS} does not support native desktop builds; skipping macOS/iOS/Linux scripts"
        ;;
esac

run_script "build_android.sh"

log "All requested builds finished"

