#!/usr/bin/env python3
import sys
import yaml

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <metadata.yaml> <model.yaml>")
        sys.exit(1)

    with open(sys.argv[1]) as f:
        meta = yaml.safe_load(f)["meta_parameters"]

    with open(sys.argv[2]) as f:
        model = yaml.safe_load(f)["functions"]

    for fn in model:
        name = fn["name"]

        # Only pointer-typed params matter
        ptr_params = [
            p["name"] for p in fn.get("params", [])
            if p["type"]["kind"] == "pointer"
        ]

        if not ptr_params:
            continue

        # Param names covered by metadata (index 1 of each [Tag, paramName, ...] entry)
        covered = {entry[1] for entry in meta.get(name, [])}

        missing = [p for p in ptr_params if p not in covered]

        if missing:
            print(f"{name}:")
            for p in missing:
                print(f"  - {p}")

if __name__ == "__main__":
    main()
