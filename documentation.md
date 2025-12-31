# Repository Overview

This repository contains the PineBuds SDK along with build scripts, firmware applications, and platform support code. The layout below highlights the primary components and provides a brief description of the major files and directories so contributors can quickly find entry points.

## Build and Tooling at the Root
- `build.sh`: Wrapper that runs the firmware make target with parallel jobs, logging output to `log.txt` and surfacing errors when the build fails.
- `start_dev.sh`, `backup.sh`, `download.sh`, `convert.sh`, `clear.sh`, `build.sh`, `Makefile`: Shell helpers for launching the Docker-based dev environment, backing up firmware, flashing images, cleaning artifacts, and invoking the core make targets.
- `Dockerfile`, `docker-compose.yml`: Container definitions for reproducible builds and access to serial flashing tools.
- `notes.txt`: Project notes describing behaviors and change history.

### Build verification and expected outputs (M0 task)
- **Target**: The default firmware target invoked by `build.sh` is `T=open_source` (chip: `best2300p`, see `config/open_source/target.mk`).
- **Toolchain**: Builds assume the ARM GNU toolchain from `gcc-arm-none-eabi-9-2019-q4-major` (installed in the dev container at `/src/gcc-arm-none-eabi-9-2019-q4-major/bin`, see `Dockerfile`).
- **Host prerequisites**: The audio prompt converter invoked during the build requires `ffmpeg` and `xxd` on `PATH` (the latter typically provided by `vim-common`). The cross build also needs `arm-none-eabi-gcc/g++/ar` available.
- **Command to run**: From the repository root, execute `./build.sh` (or `make -j"$(nproc)" T=open_source DEBUG=1` if you prefer direct invocation). The wrapper writes compiler output to `log.txt` and reports failures to the console.
- **Expected artifacts**: Successful builds populate `out/open_source/` with `open_source.elf`, `open_source.bin` (flashable firmware image), `open_source.map`, `open_source.lst`, and related metadata (`open_source.str`, `build_info.o`, linker script copies). These names come from the root `Makefile` defaults where `IMAGE_FILE` is derived from the target name.
- **If the toolchain is missing**: Use the provided Docker image (`docker compose run dev_env`) so `arm-none-eabi-gcc` 9-2019q4 is available on `PATH`. Outside the container, ensure the same toolchain version is installed before running the build.

## Application Layer (`apps/`)
- `apps/main/`: Main application entry points and behavior orchestrators.
  - `apps.cpp`: Coordinates application initialization, power-on cases, and integration points for BLE, ANC, OTA, and TWS features.
  - `led_control.cpp` / `led_control.h`: Manages LED status timers to reflect device connection, scanning, and charging states.
  - `key_handler.cpp` / `key_handler.h`: Defines handlers for touch/button events and routes them to feature actions.
  - `rb_codec.cpp` / `rb_codec.h`: Implements ring-buffer–based audio codec helpers used by the player pipeline.
  - `gfps.cpp`: Google Fast Pair support for provisioning and discovery.
  - `apps_tester.cpp`: Harness for exercising application flows during testing builds.
  - `Makefile`: Builds the app objects into the firmware image.
- `apps/common/`: Reusable helpers shared across apps.
  - `app_thread.c` / `app_thread.h`: Threading utilities that wrap RTOS tasks used throughout the application layer.
  - `app_utils.c` / `app_utils.h`: Small helpers for timing, logging, and power utilities.
  - `app_spec_ostimer.*`: Specialized OS timer helpers for scheduling callbacks.
  - `randfrommic.*`: Randomness generation derived from microphone data.
- Additional app feature folders (`anc`, `audioplayers`, `battery`, `cmd`, `factory`, `key`, `mic`, `mic_alg`, `pwl`, `voice_detector`, etc.) each include focused logic for ANC control, audio playback, battery monitoring, command handling, factory tools, key mappings, microphone algorithms, pulse-width light control, and voice detection respectively.

## Services (`services/`)
Framework and middleware services used by applications:
- `app_ibrt/`, `app_tws/`: True Wireless Stereo coordination, connection roles, and peer link management.
- `ble_app/`, `app_ai/`: BLE profiles and AI/voice assistant integrations.
- `audio_process/`, `audioflinger/`, `audio_dump/`: Audio processing chains, routing, and diagnostics.
- `auto_test/`, `anc_spp_tool/`: Test harnesses and SPP-based ANC configuration tools.
- `Makefile`: Aggregates service components for the firmware build.

## Platform Support (`platform/`)
Hardware abstraction and startup code for the BES SoC:
- `platform/main/main.cpp`: Firmware entry point that initializes clocks, GPIO, flash, timers, and launches the application under RTOS.
- `platform/main/noapp_main.cpp`, `nostd_main.c`, `startup_main.S`: Alternate startup flows for minimal or no-application builds.
- `platform/hal/`: Hardware abstraction layer for timers, DMA, GPIO, power management, flash, and peripherals.
- `platform/drivers/`: Low-level drivers for chips, audio interfaces, and communications.
- `platform/cmsis/`: CMSIS definitions and RTOS integration.
- `Makefile`: Coordinates platform objects and cross-compilation settings.

## Configuration and Interfaces
- `config/`: Default configuration sources, including resource packs (audio prompts) and build-time options such as language-specific audio assets.
- `include/`: Project-wide headers exposing shared types, HAL interfaces, and configuration macros.
- `thirdparty/`: External libraries and codecs bundled with the SDK.
- `rtos/`: RTOS-specific integration code and configuration.

## Scripts and Utilities
- `scripts/`, `utils/`, `dev_tools/`, `services/audio_dump/`: Host-side utilities for flashing, log capture, and analysis.
- `uart_log.sh`: Convenience wrapper for attaching to UART logs during development.

## How to Navigate
- Start with `platform/main/main.cpp` to see hardware initialization and how control is transferred to the app layer.
- Follow into `apps/main/apps.cpp` and related helpers (`key_handler.cpp`, `led_control.cpp`) to understand runtime behavior, inputs, and status indications.
- Explore feature-specific folders under `apps/` and `services/` to find implementations for ANC, BLE, audio, and testing workflows.
- Use the root scripts when building or flashing to ensure the correct toolchain and container environment are used.
