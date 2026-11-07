#!/usr/bin/env python3
"""Build DQK.EXE from src/ under DOSBox-X: Turbo C 2.01, TASM 3.1 and RTLink/Plus.

    build/build.py --tc DIR --tasm DIR --rtlink DIR [--out DIR] [--keep]

  --tc      Turbo C 2.01: the directory holding TCC.EXE, with INCLUDE\\ beside it
  --tasm    Turbo Assembler 3.1 (Borland C++ 3.1's BIN): the directory holding TASM.EXE
  --rtlink  RTLink/Plus: the installed directory holding RTLINK.EXE
  --out     where DQK.EXE, DQK.MAP and the logs land (default: obj/)
  --keep    keep the staged DOS drive (it is printed) for inspection

Each tool directory may also be given by environment variable: DQK_TC, DQK_TASM,
DQK_RTLINK.

What it does, with no Python in the byte path: copies src/c, src/asm and build/ into a
fresh staging directory, mounts it as C: with the compiler as D: (Turbo C's default
TURBOC.CFG names D:\\INCLUDE), the assembler as E: and the linker as F:, and runs
build\\BUILD.BAT in DOSBox-X with no window. BUILD.BAT compiles every C module with
`tcc -c -ml -O -Z -d`, assembles every assembly module, and links them with
`rtlink @DQK.LNK`. This script then checks that every object and DQK.EXE exist and
that the logs carry no errors, and copies the results to --out.

Exit status: 0 built, 1 a tool reported an error or an output is missing, 2 a usage or
setup problem.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def die(msg, rc=2):
    print(f"build.py: {msg}", file=sys.stderr)
    sys.exit(rc)


def find(dirpath, name):
    """The entry in dirpath named `name`, case-insensitively, or None."""
    for entry in os.listdir(dirpath):
        if entry.lower() == name.lower():
            return os.path.join(dirpath, entry)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tc", default=os.environ.get("DQK_TC"))
    ap.add_argument("--tasm", default=os.environ.get("DQK_TASM"))
    ap.add_argument("--rtlink", default=os.environ.get("DQK_RTLINK"))
    ap.add_argument("--out", default=os.path.join(ROOT, "obj"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--timeout", type=int, default=900, help="seconds before DOSBox-X is killed")
    args = ap.parse_args()

    for opt, exe in (("tc", "TCC.EXE"), ("tasm", "TASM.EXE"), ("rtlink", "RTLINK.EXE")):
        d = getattr(args, opt)
        if not d:
            die(f"--{opt} is required (or DQK_{opt.upper()})")
        if not os.path.isdir(d) or not find(d, exe):
            die(f"--{opt} {d}: no {exe} there")
    dosbox = shutil.which(os.environ.get("DOSBOX", "dosbox-x"))
    if not dosbox:
        die("dosbox-x is not on PATH (or set DOSBOX)")

    stage = tempfile.mkdtemp(prefix="dqk-build-")
    for sub, dst in (("src/c", "SRC"), ("src/asm", "SRC"), ("build", "BUILD")):
        os.makedirs(os.path.join(stage, dst), exist_ok=True)
        for name in os.listdir(os.path.join(ROOT, sub)):
            path = os.path.join(ROOT, sub, name)
            if not os.path.isfile(path) or name.endswith((".py", ".md")):
                continue
            target = os.path.join(stage, dst, name.upper())
            if dst == "BUILD":       # DOS text: CRLF line ends
                with open(path, "rb") as fh:
                    text = fh.read().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
                with open(target, "wb") as fh:
                    fh.write(text)
            else:
                shutil.copy(path, target)
    os.makedirs(os.path.join(stage, "OUT"))

    conf = os.path.join(stage, "dosbox.conf")
    with open(conf, "w") as fh:
        fh.write(f"""[sdl]
output=surface
[dosbox]
memsize=16
[cpu]
cycles=max
[dos]
xms=true
ems=false
[autoexec]
mount c "{stage}"
mount d "{os.path.abspath(args.tc)}"
mount e "{os.path.abspath(args.tasm)}"
mount f "{os.path.abspath(args.rtlink)}"
c:
cd \\BUILD
call BUILD.BAT
exit
""")
    env = {k: v for k, v in os.environ.items() if k not in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY")}
    env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    try:
        subprocess.run(["timeout", "-s", "KILL", str(args.timeout), dosbox, "-conf", conf,
                        "-nopromptfolder", "-nomenu", "-silent"],
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, env=env, check=False)
    finally:
        pass

    out = os.path.join(stage, "OUT")
    problems = []
    for src_dir, ext in (("src/c", ".c"), ("src/asm", ".asm")):
        for name in sorted(os.listdir(os.path.join(ROOT, src_dir))):
            if name.endswith(ext):
                obj = name[: -len(ext)].upper() + ".OBJ"
                if not os.path.exists(os.path.join(stage, "SRC", obj)):
                    problems.append(f"no {obj}: {src_dir}/{name} did not build")
    for log, pat in (("CC.LOG", r"^(Error|Fatal) "), ("AS.LOG", r"^\*\*(Error|Fatal)\*\*"),
                     ("LINK.LOG", r"\b(error|fatal) ert\d+")):
        path = os.path.join(out, log)
        if not os.path.exists(path):
            problems.append(f"no {log}: BUILD.BAT did not reach that step")
            continue
        with open(path, errors="replace") as fh:
            for line in fh:
                if re.search(pat, line, re.I):
                    problems.append(f"{log}: {line.rstrip()}")
    if not os.path.exists(os.path.join(out, "DQK.EXE")):
        problems.append("no DQK.EXE")

    os.makedirs(args.out, exist_ok=True)
    for name in os.listdir(out):
        shutil.copy(os.path.join(out, name), os.path.join(args.out, name))
    if args.keep:
        print(f"staged drive kept at {stage}")
    else:
        shutil.rmtree(stage, ignore_errors=True)

    if problems:
        print("\n".join(problems[:200]))
        if len(problems) > 200:
            print(f"... {len(problems) - 200} more")
        print(f"FAILED: {len(problems)} problem(s); logs in {args.out}")
        return 1
    exe = os.path.join(args.out, "DQK.EXE")
    print(f"built {exe} ({os.path.getsize(exe)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
