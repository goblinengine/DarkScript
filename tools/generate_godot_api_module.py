#!/usr/bin/env python
"""Generate Daslang bindings from Godot extension_api.json.

This is a placeholder scaffold.

Intended flow:
- Read Godot's extension API description JSON.
- Emit Daslang module(s) that wrap Godot classes, methods, enums, and constants.

Future implementation should likely:
- Use daScript's binding facilities (dasbind / annotations / macros).
- Generate thin wrappers that convert Variant <-> daScript values.

Usage idea:
  python tools/generate_godot_api_module.py --api ../../godot-cpp/gdextension/extension_api.json --out ./generated
"""

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--api", required=True, help="Path to extension_api.json")
    parser.add_argument("--out", required=True, help="Output folder")
    args = parser.parse_args()

    api_path = Path(args.api)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    data = json.loads(api_path.read_text(encoding="utf-8"))

    # Minimal smoke output so CI/users can see the tool runs.
    summary = {
        "header": data.get("header", {}),
        "builtin_class_count": len(data.get("builtin_classes", [])),
        "class_count": len(data.get("classes", [])),
        "global_enum_count": len(data.get("global_enums", [])),
    }

    (out_dir / "api_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
