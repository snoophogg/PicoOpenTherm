#!/usr/bin/env python3
"""
generate_discovery.py

Read `tools/discovery_list.json` and generate:
 - `src/generated/discovery_list.hpp` (C++ header)
 - `tools/discovery_list.py` (Python module)

Run this script when modifying `tools/discovery_list.json`.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_IN = ROOT / "discovery_list.json"
CPP_OUT = ROOT.parent / "src" / "generated" / "discovery_list.hpp"
PY_OUT = ROOT / "discovery_list.py"

def make_cpp(data):
    lines = []
    lines.append("#pragma once\n")
    lines.append("#include <array>\n#include <string>\n#include <vector>\n\nnamespace OpenTherm { namespace DiscoveryList {\n")

    for comp, ids in data.items():
        name = comp.upper()
        lines.append(f"static const std::vector<std::string> {name} = {{\n")
        for oid in ids:
            lines.append(f'    std::string("{oid}"),\n')
        lines.append("};\n\n")

    # Add helper accessor
    lines.append("} } // namespace OpenTherm::DiscoveryList\n")
    return "".join(lines)

def make_py(data):
    lines = []
    lines.append("# Auto-generated from discovery_list.json\n")
    lines.append("DISCOVERY = {")
    for comp, ids in data.items():
        lines.append(f"    '{comp}': [")
        for oid in ids:
            lines.append(f"        '{oid}',")
        lines.append("    ],")
    lines.append("}\n")
    return "\n".join(lines)

def main():
    data = json.loads(JSON_IN.read_text())
    CPP_OUT.parent.mkdir(parents=True, exist_ok=True)
    CPP_OUT.write_text(make_cpp(data))
    PY_OUT.write_text(make_py(data))
    print(f"Generated {CPP_OUT} and {PY_OUT}")

if __name__ == '__main__':
    main()
