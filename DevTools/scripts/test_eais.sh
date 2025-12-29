#!/usr/bin/env bash
# ==============================================================================
# EAIS TEST RUNNER (Linux/macOS) - Automated Test Pipeline
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
PROJECT_ROOT="$(cd "$ROOT_DIR/../.." && pwd)"

UE_ROOT="${UE_ROOT:-/opt/unreal}"
UPROJECT_PATH="$PROJECT_ROOT/A_MiniFootball.uproject"
AI_PROFILES_DIR="$PROJECT_ROOT/Content/AIProfiles"
OUTPUT_DIR="$ROOT_DIR/DevTools/output"

# Create output dir
mkdir -p "$OUTPUT_DIR"

echo "========================================"
echo " P_EAIS Build & Validation Pipeline"
echo "========================================"
echo ""
echo "Project: $UPROJECT_PATH"
echo "Plugin: $ROOT_DIR"
echo ""

# ============================================================================
# Step 1: Validate AI Profiles
# ============================================================================

echo "========================================"
echo " Step 1: Validating AI Profiles"
echo "========================================"

PROFILE_COUNT=0
PROFILE_ERRORS=0

for profile in "$AI_PROFILES_DIR"/*.json; do
    if [ -f "$profile" ]; then
        PROFILE_COUNT=$((PROFILE_COUNT + 1))
        
        # Basic JSON validation using python if available
        if command -v python3 &> /dev/null; then
            if python3 -c "import json; json.load(open('$profile'))" 2>/dev/null; then
                echo "  [PASS] $(basename "$profile")"
            else
                echo "  [FAIL] $(basename "$profile")"
                PROFILE_ERRORS=$((PROFILE_ERRORS + 1))
            fi
        else
            # Just check if it's valid JSON using jq if available
            if command -v jq &> /dev/null; then
                if jq . "$profile" > /dev/null 2>&1; then
                    echo "  [PASS] $(basename "$profile")"
                else
                    echo "  [FAIL] $(basename "$profile")"
                    PROFILE_ERRORS=$((PROFILE_ERRORS + 1))
                fi
            else
                echo "  [SKIP] $(basename "$profile") (no JSON validator available)"
            fi
        fi
    fi
done

echo ""
echo "Profile Validation: $((PROFILE_COUNT - PROFILE_ERRORS))/$PROFILE_COUNT passed"

# ============================================================================
# Step 2: Build Project (optional)
# ============================================================================

if [ "${SKIP_BUILD:-0}" != "1" ]; then
    echo ""
    echo "========================================"
    echo " Step 2: Building Project"
    echo "========================================"
    
    if [ -f "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ]; then
        "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
            -project="$UPROJECT_PATH" \
            -noP4 \
            -clientconfig=Development \
            -platform=Linux \
            -build \
            -unattended \
            -utf8output 2>&1 | tee "$OUTPUT_DIR/build_output.txt"
            
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            echo "[ERROR] Build failed!"
            exit 1
        fi
        echo "[OK] Build successful"
    else
        echo "[WARN] RunUAT.sh not found, skipping build"
    fi
fi

# ============================================================================
# Step 3: Run Automation Tests
# ============================================================================

if [ "${VALIDATE_ONLY:-0}" != "1" ]; then
    echo ""
    echo "========================================"
    echo " Step 3: Running Automation Tests"
    echo "========================================"
    
    UE_EDITOR_CMD="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"
    
    if [ -f "$UE_EDITOR_CMD" ]; then
        "$UE_EDITOR_CMD" "$UPROJECT_PATH" \
            -ExecCmds="Automation RunTests EAIS; Quit" \
            -unattended \
            -nullrhi \
            -testexit="Automation Test Queue Empty" \
            -nop4 \
            -stdout \
            -FullStdOutLogOutput 2>&1 | tee "$OUTPUT_DIR/test_output.txt"
            
        # Count results
        PASSED=$(grep -c "Result=Passed" "$OUTPUT_DIR/test_output.txt" 2>/dev/null || echo "0")
        FAILED=$(grep -c "Result=Failed" "$OUTPUT_DIR/test_output.txt" 2>/dev/null || echo "0")
        
        echo ""
        echo "Test Results: $PASSED passed, $FAILED failed"
        
        if [ "$FAILED" -gt 0 ]; then
            echo "[ERROR] Some tests failed!"
            exit 1
        fi
    else
        echo "[WARN] UnrealEditor-Cmd not found, skipping tests"
    fi
fi

# ============================================================================
# Summary
# ============================================================================

echo ""
echo "========================================"
echo " Pipeline Summary"
echo "========================================"
echo ""

if [ "$PROFILE_ERRORS" -eq 0 ]; then
    echo "  ✓ All checks PASSED!"
    exit 0
else
    echo "  ✗ $PROFILE_ERRORS error(s) found"
    exit 1
fi
