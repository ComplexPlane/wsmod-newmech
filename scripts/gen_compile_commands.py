#!/usr/bin/env python3

from pathlib import Path
import json


def main():
    cmds = []
    repo_dir = Path(__file__).parent.parent.resolve()
    src_dir = repo_dir / "src"
    dep_include_dir = repo_dir / "3rdparty"

    # Find all .cpp files recursively in src directory
    for src_path in src_dir.rglob("*.cpp"):
        arguments = [
            "/usr/bin/c++",
            "-I", str(src_dir),
            "-isystem", str(dep_include_dir),
            "-m32",
            "-std=c++20",
            "-Wall",
            "-Werror",
            "-Wshadow",
            "-Wimplicit-fallthrough",
            "-Wno-write-strings",
            "-Wno-address-of-packed-member",
            "-c", str(src_path),
            "-o", str(src_path.with_suffix(".o")),
        ]
        cmds.append(
            {
                "directory": str(repo_dir),
                "arguments": arguments,
                "file": str(src_path),
            }
        )

    with open(repo_dir / "compile_commands.json", "w") as f:
        json.dump(cmds, f, indent=4)


if __name__ == "__main__":
    main()
