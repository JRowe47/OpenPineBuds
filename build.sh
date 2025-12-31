#!/usr/bin/env sh
set -eu

missing=""
for cmd in arm-none-eabi-gcc arm-none-eabi-g++ arm-none-eabi-ar ffmpeg xxd; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        missing="$missing $cmd"
    fi
done

if [ -n "$missing" ]; then
    cat <<MISSING
Missing build prerequisites:${missing}
- Install the ARM GNU toolchain (e.g., gcc-arm-none-eabi 9-2019q4) and ensure arm-none-eabi-gcc/g++/ar are on PATH.
- Install host utilities used by the audio prompt converter: ffmpeg and xxd (often provided by the vim-common package).
MISSING
    exit 127
fi

if make -j "$(nproc)" T=open_source DEBUG=1 >log.txt 2>&1; then
        echo "build success"
else
        echo "build failed and call log.txt"
        grep "error:" log.txt || true
fi
