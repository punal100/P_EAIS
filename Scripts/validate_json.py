#!/usr/bin/env python3
"""
P_EAIS JSON Behavior Validator

Validates AI behavior JSON files against the EAIS schema.
Usage: python validate_json.py [path_to_profiles_dir]

@Author: Punal Manalan
@Date: 29/12/2025
"""

import json
import sys
import os
from pathlib import Path

# Try to import jsonschema, provide helpful error if missing
try:
    import jsonschema
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False
    print("[WARN] jsonschema module not installed. Install with: pip install jsonschema")
    print("[WARN] Falling back to basic JSON syntax validation only.")

def find_schema():
    """Find the EAIS schema file relative to this script."""
    script_dir = Path(__file__).parent.parent
    schema_paths = [
        script_dir / "Docs" / "ai-schema.json",
        script_dir / "Content" / "EAISSchemas" / "eaibehavior.schema.json",
    ]
    
    for path in schema_paths:
        if path.exists():
            return path
    
    return None

def validate_basic(json_data, filepath):
    """Basic validation without jsonschema."""
    errors = []
    
    # Check required fields
    if "name" not in json_data and "Name" not in json_data:
        errors.append("Missing 'name' field")
    
    if "states" not in json_data and "States" not in json_data:
        errors.append("Missing 'states' field")
    else:
        states = json_data.get("states", json_data.get("States", {}))
        if isinstance(states, dict) and len(states) == 0:
            errors.append("No states defined")
        elif isinstance(states, list) and len(states) == 0:
            errors.append("No states defined")
    
    return errors

def validate_file(filepath, schema=None):
    """Validate a single JSON file."""
    result = {
        "path": str(filepath),
        "valid": False,
        "errors": [],
        "warnings": []
    }
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        result["errors"].append(f"JSON Parse Error: {e}")
        return result
    except Exception as e:
        result["errors"].append(f"File Read Error: {e}")
        return result
    
    # Basic validation
    basic_errors = validate_basic(data, filepath)
    result["errors"].extend(basic_errors)
    
    # Schema validation (if available)
    if HAS_JSONSCHEMA and schema:
        try:
            jsonschema.validate(data, schema)
        except jsonschema.ValidationError as e:
            result["errors"].append(f"Schema Validation: {e.message}")
        except jsonschema.SchemaError as e:
            result["warnings"].append(f"Schema Error: {e.message}")
    
    result["valid"] = len(result["errors"]) == 0
    return result

def main():
    # Determine profiles directory
    if len(sys.argv) > 1:
        profiles_dir = Path(sys.argv[1])
    else:
        # Default: Content/AIProfiles relative to project
        script_dir = Path(__file__).parent.parent.parent.parent
        profiles_dir = script_dir / "Content" / "AIProfiles"
    
    if not profiles_dir.exists():
        print(f"[ERROR] Profiles directory not found: {profiles_dir}")
        sys.exit(1)
    
    # Load schema
    schema = None
    schema_path = find_schema()
    if schema_path and HAS_JSONSCHEMA:
        try:
            with open(schema_path, 'r', encoding='utf-8') as f:
                schema = json.load(f)
            print(f"[INFO] Using schema: {schema_path}")
        except Exception as e:
            print(f"[WARN] Could not load schema: {e}")
    
    # Find all JSON files
    json_files = list(profiles_dir.rglob("*.json"))
    
    if not json_files:
        print(f"[WARN] No JSON files found in {profiles_dir}")
        sys.exit(0)
    
    print(f"[INFO] Validating {len(json_files)} profile(s)...\n")
    
    # Validate each file
    passed = 0
    failed = 0
    
    for filepath in json_files:
        result = validate_file(filepath, schema)
        rel_path = filepath.relative_to(profiles_dir)
        
        if result["valid"]:
            print(f"[OK] {rel_path}")
            passed += 1
        else:
            print(f"[ERR] {rel_path}")
            for error in result["errors"]:
                print(f"      -> {error}")
            failed += 1
        
        for warning in result.get("warnings", []):
            print(f"[WARN] {rel_path}: {warning}")
    
    # Summary
    print(f"\n{'='*40}")
    print(f"Results: {passed} passed, {failed} failed")
    print(f"{'='*40}")
    
    sys.exit(2 if failed else 0)

if __name__ == "__main__":
    main()
