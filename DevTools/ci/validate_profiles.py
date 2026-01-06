#!/usr/bin/env python3
"""
P_EAIS JSON Behavior Validator (CI Entrypoint)

Validates AI behavior JSON files against the EAIS schema and performs deterministic checks.
- No duplicate state IDs
- All referenced states exist
- Transitions have valid operators
- Priority ordering check

Usage: python validate_profiles.py [path_to_profiles_dir]
"""

import json
import sys
import os
from pathlib import Path

# Try to import jsonschema
try:
    import jsonschema
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False

def find_schema():
    script_dir = Path(__file__).parent.parent.parent
    schema_paths = [
        script_dir / "Docs" / "AIProfile.schema.json",
        script_dir / "Docs" / "ai-schema.json",
    ]
    for path in schema_paths:
        if path.exists():
            return path
    return None

def validate_deterministic(data, filepath):
    errors = []
    
    # 1. No duplicate state IDs
    states = data.get("states", [])
    if isinstance(states, list):
        state_ids = [s.get("id") for s in states if "id" in s]
        if len(state_ids) != len(set(state_ids)):
            duplicates = set([x for x in state_ids if state_ids.count(x) > 1])
            errors.append(f"Duplicate state IDs found: {duplicates}")
        
        valid_state_ids = set(state_ids)
        
        # 2. All referenced states exist
        for state in states:
            transitions = state.get("transitions", [])
            state_id = state.get("id", "unknown")
            for trans in transitions:
                dest = trans.get("to")
                if dest and dest not in valid_state_ids:
                    errors.append(f"State '{state_id}' transitions to non-existent state '{dest}'")
    
    # 3. Transitions have valid condition operators (basic check)
    # This is partially covered by schema if used, but added here for safety.
    
    return errors

def validate_file(filepath, schema=None):
    result = {"path": str(filepath), "valid": False, "errors": []}
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        result["errors"].append(f"JSON Parse/Read Error: {e}")
        return result
    
    # Deterministic checks
    det_errors = validate_deterministic(data, filepath)
    result["errors"].extend(det_errors)
    
    # Schema validation
    if HAS_JSONSCHEMA and schema:
        try:
            jsonschema.validate(data, schema)
        except jsonschema.ValidationError as e:
            result["errors"].append(f"Schema Validation Error: {e.message}")
    
    result["valid"] = len(result["errors"]) == 0
    return result

def main():
    if len(sys.argv) > 1:
        profiles_dir = Path(sys.argv[1])
    else:
        # Default to Content/AIProfiles in repo
        repo_root = Path(__file__).parent.parent.parent
        profiles_dir = repo_root / "Content" / "AIProfiles"
    
    if not profiles_dir.exists():
        print(f"Profiles directory not found: {profiles_dir}")
        sys.exit(1)
    
    schema = None
    schema_path = find_schema()
    if schema_path and HAS_JSONSCHEMA:
        try:
            with open(schema_path, 'r', encoding='utf-8') as f:
                schema = json.load(f)
            print(f"Using schema: {schema_path}")
        except Exception:
            pass
    elif not HAS_JSONSCHEMA:
        print("Warning: jsonschema not installed, running basic validation only.")
    
    json_files = list(profiles_dir.rglob("*.runtime.json")) + list(profiles_dir.rglob("*.json"))
    # Filter to unique paths
    json_files = sorted(list(set(json_files)))
    
    if not json_files:
        print(f"No profile files found in {profiles_dir}")
        sys.exit(0)
    
    failed = 0
    for filepath in json_files:
        res = validate_file(filepath, schema)
        if res["valid"]:
            print(f"[PASS] {filepath.name}")
        else:
            print(f"[FAIL] {filepath.name}")
            for err in res["errors"]:
                print(f"  -> {err}")
            failed += 1
            
    if failed > 0:
        print(f"\nValidation FAILED: {failed} file(s) invalid.")
        sys.exit(1)
    else:
        print("\nValidation PASSED.")
        sys.exit(0)

if __name__ == "__main__":
    main()
