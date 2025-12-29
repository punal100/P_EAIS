#!/usr/bin/env bash
# ==============================================================================
# HEADLESS TEST RUNNER (Linux/macOS)
# P_EAIS Automation Tests
# ==============================================================================

set -euo pipefail

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Unreal Engine path (adjust to your environment)
UE_ROOT="${UE_ROOT:-/opt/unreal}"

# Project file path
UPROJECT_PATH="$(cd "$ROOT_DIR/../.." && pwd)/A_MiniFootball.uproject"

echo "[EAIS] Running Automation Tests..."
echo "[EAIS] UE Root: $UE_ROOT"
echo "[EAIS] Project: $UPROJECT_PATH"

# Check if UE exists
if [ ! -d "$UE_ROOT" ]; then
    echo "[EAIS] ERROR: Unreal Engine not found at $UE_ROOT"
    echo "[EAIS] Set UE_ROOT environment variable to your Unreal Engine installation"
    exit 1
fi

UE_EDITOR_CMD="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"

# Check if editor exists
if [ ! -f "$UE_EDITOR_CMD" ]; then
    # Try alternative path
    UE_EDITOR_CMD="$UE_ROOT/Engine/Binaries/Linux/UE4Editor-Cmd"
fi

if [ ! -f "$UE_EDITOR_CMD" ]; then
    echo "[EAIS] ERROR: UnrealEditor-Cmd not found"
    exit 1
fi

"$UE_EDITOR_CMD" "$UPROJECT_PATH" \
    -ExecCmds="Automation RunTests EAIS; Quit" \
    -unattended \
    -nullrhi \
    -testexit="Automation Test Queue Empty" \
    -log \
    -stdout \
    -FullStdOutLogOutput

if [ $? -ne 0 ]; then
    echo "[EAIS] Tests FAILED!"
    exit 1
fi

echo "[EAIS] Tests PASSED!"
