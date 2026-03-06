#!/usr/bin/env python3
"""
Verify that all functions in ze_api.yaml with pointer parameters
have corresponding entries in ze_meta_parameters.yaml.
"""

import sys
import yaml


def load_yaml(path):
    with open(path) as f:
        return yaml.safe_load(f)


IGNORED_PARAMS = {"ptr", "pDdiTable"}


def get_pointer_params(func):
    return [
        param["name"]
        for param in func.get("params", [])
        if param.get("type", {}).get("kind") == "pointer"
        and param["name"] not in IGNORED_PARAMS
    ]


def main():
    api_path, meta_path = sys.argv[1], sys.argv[2]

    api = load_yaml(api_path)
    meta = load_yaml(meta_path)

    meta_keys = set(meta.get("meta_parameters", {}).keys())

    # Build {func_name: [pointer_param_names]} for functions that have pointer params
    needs_meta = {
        func["name"]: get_pointer_params(func)
        for func in api.get("functions", [])
        if get_pointer_params(func)
    }

    missing = {name: params for name, params in needs_meta.items() if name not in meta_keys}

    if missing:
        print(f"❌ {len(missing)} function(s) with pointer params are missing metadata:\n")
        for func_name, params in missing.items():
            print(f"  {func_name}:")
            for p in params:
                print(f"    - {p}")
        sys.exit(1)
    else:
        print(f"✅ All {len(needs_meta)} function(s) with pointer params have metadata.")
        sys.exit(0)


if __name__ == "__main__":
    main()
