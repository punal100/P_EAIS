#!/usr/bin/env bash
# ==============================================================================
# HEADLESS BUILD SCRIPT (Linux/macOS)
# P_EAIS Plugin Build
# ==============================================================================

set -euo pipefail

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Unreal Engine path (adjust to your environment)
UE_ROOT="${UE_ROOT:-/opt/unreal}"

# Project file path
UPROJECT_PATH="$(cd "$ROOT_DIR/../.." && pwd)/A_MiniFootball.uproject"

echo "[EAIS] Building Plugin..."
echo "[EAIS] UE Root: $UE_ROOT"
echo "[EAIS] Project: $UPROJECT_PATH"

# Check if UE exists
if [ ! -d "$UE_ROOT" ]; then
    echo "[EAIS] ERROR: Unreal Engine not found at $UE_ROOT"
    echo "[EAIS] Set UE_ROOT environment variable to your Unreal Engine installation"
    exit 1
fi

# Check if project exists
if [ ! -f "$UPROJECT_PATH" ]; then
    echo "[EAIS] ERROR: Project file not found at $UPROJECT_PATH"
    exit 1
fi

RUNUAT_CMD="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"

echo "[EAIS] Building Project (Includes Plugin)..."
"$RUNUAT_CMD" BuildCookRun \
    -project="$UPROJECT_PATH" \
    -noP4 \
    -clientconfig=Development \
    -serverconfig=Development \
    -platform=Linux \
    -build \
    -unattended \
    -utf8output

if [ $? -ne 0 ]; then
    echo "[EAIS] Build FAILED!"
    exit 1
fi

echo "[EAIS] Build SUCCESS!"
