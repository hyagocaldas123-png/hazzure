#!/usr/bin/env python3
# coding: utf-8
"""
Hazzure Master Release & Packaging Pipeline
Compiles submodules and verifies self-tests, then stages a distribution before
publication. Previous outputs and user profiles are retained.
"""

import os
import argparse
from release_package import assemble, suite_files
import sys
import subprocess

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RELEASE_DIR = os.path.join(ROOT, "release")

def run_cmd(cmd, cwd):
    print(f"\n[EXEC] {cmd} in {cwd}")
    res = subprocess.run(cmd, cwd=cwd, shell=True)
    if res.returncode != 0:
        print(f"[FATAL] Command failed with exit code {res.returncode}")
        sys.exit(res.returncode)

def main(argv=None):
    parser=argparse.ArgumentParser(description="Build and stage a verified Hazzure distribution.")
    parser.add_argument("--package-only",action="store_true",help="Package supplied artifacts without building or launching them.")
    parser.add_argument("--output",default=RELEASE_DIR)
    parser.add_argument("--internal")
    parser.add_argument("--runtime")
    parser.add_argument("--panel")
    parser.add_argument("--external")
    args=parser.parse_args(argv)
    if not args.package_only:
        run_cmd("cmd /c run_offset_dumper.cmd",os.path.join(ROOT,"tools"))
        run_cmd("cmd /c build.cmd",os.path.join(ROOT,"HazzureInternal"))
        run_cmd("cmd /c build.cmd && build-selftest.cmd && selftest.exe",os.path.join(ROOT,"HazzurePanel"))
        run_cmd("cmd /c build-external.cmd && test-external.cmd",ROOT)
        run_cmd('powershell -ExecutionPolicy Bypass -Command "& .\\build.ps1 -OutputName Hazzure.exe -SkipRenderer"',os.path.join(ROOT,"Hazzure"))
    files=suite_files(ROOT,args.internal,args.runtime,args.panel,args.external)
    result=assemble(files,args.output,ROOT,{
        "build_pipeline_executed":not args.package_only,
        "in_game_validation":False,
        "artifact_selection":"explicit overrides" if any((args.internal,args.runtime,args.panel)) else "canonical build outputs",
    })
    print("[OK] Verified files staged in: "+result["destination"])
    print("[OK] Manifest: "+result["manifest"])
    if result["previous"]:print("[OK] Previous release retained in: "+result["previous"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
