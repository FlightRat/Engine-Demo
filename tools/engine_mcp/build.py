"""Build native targets with an explicit solution root and normalized Windows env."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("target", choices=["tests", "editor"])
    parser.add_argument("--configuration", choices=["Debug", "Release"], default="Debug")
    parser.add_argument("--msbuild")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    msbuild = args.msbuild or shutil.which("MSBuild.exe")
    if not msbuild:
        cl = shutil.which("cl.exe")
        candidate = Path(cl).parents[7] / "MSBuild/Current/Bin/MSBuild.exe" if cl else None
        if candidate and candidate.is_file():
            msbuild = str(candidate)
    if not msbuild:
        parser.error("MSBuild not found; pass --msbuild or use a VS developer shell")
    project = "EDITOR/tests/AutomationTests.vcxproj" if args.target == "tests" else "EDITOR/EDITOR.vcxproj"
    log = Path(__file__).parent / f"build-{args.target}-{args.configuration}.log"
    command = [msbuild, project, "/nologo", "/m:2", "/p:Platform=x64", f"/p:Configuration={args.configuration}",
               f"/p:SolutionDir={root}/", "/clp:ErrorsOnly;Summary", f"/flp:logfile={log};verbosity=normal"]
    return subprocess.call(command, cwd=root, env={k.upper(): v for k, v in os.environ.items()})


if __name__ == "__main__":
    raise SystemExit(main())
