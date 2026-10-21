"""Re-verify every source of the project against the retail executable.

Each source in ``src/text/`` carries its address, its options and the
callees it calls.  This script compiles each of them with the SDK's
compiler, links it at its retail address with the callees at their retail
addresses, and compares the bytes with the retail ones.

That is the "reopening condition" of every source made executable: if the
toolchain or the options ever change, the comparison has to be run again
before a body is counted.

    python tools/verify_sources.py --elf work/eboot/EBOOT.elf \
        --toolchain /path/to/the/toolchain --image ps3gcc-toolchain

The toolchain is the one the contributor built; the script needs the SDK's
compiler, which the repository never ships (see LEGAL.md).
"""

from __future__ import annotations

import argparse
import glob
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

ADDRESS = re.compile(r"Address  : (0x[0-9A-Fa-f]+)")
FLAGS = re.compile(r"Flags    : (.+)")
CALLEE = re.compile(r"\b(sub_[0-9A-Fa-f]+)\s*\(")
PREFIX = "/usr/local/cell/host-linux/ppu"


def sources(directory: str) -> list[str]:
    return sorted(glob.glob(os.path.join(directory, "*.c")) +
                  glob.glob(os.path.join(directory, "*.cpp")))


def read_header(path: str) -> tuple[int, list[str], list[str]]:
    """The address, the options and the callees a source declares."""
    text = open(path, encoding="utf-8").read()
    address = ADDRESS.search(text)
    flags = FLAGS.search(text)
    callees = sorted(set(CALLEE.findall(text)))
    if not address or not flags:
        raise ValueError("the source does not say its address or its options")
    return int(address.group(1), 16), flags.group(1).split(), callees


def linker_script(address: int, callees: list[str]) -> str:
    lines = ["SECTIONS", "{", "  . = %s;" % hex(address)]
    for callee in callees:
        value = callee.split("_", 1)[1]
        lines.append("  %s = %s;" % (callee, hex(int(value, 16))))
        lines.append("  .%s = %s;" % (callee, hex(int(value, 16))))
    lines += ["  .text : { *(.text) }", "}"]
    return "\n".join(lines) + "\n"


def is_cpp(path: str) -> bool:
    """Whether a source is C++ — the extension, or the markers it carries."""
    if path.endswith(".cpp"):
        return True
    text = open(path, encoding="utf-8").read()
    return 'extern "C"' in text or "class " in text or "struct " in text and "::" in text


def build_and_compare(path: str, elf: str, toolchain: str, image: str) -> tuple[bool, str]:
    address, flags, callees = read_header(path)
    workdir = tempfile.mkdtemp(prefix="verify-")
    try:
        name = os.path.splitext(os.path.basename(path))[0]
        unit = "cpp" if is_cpp(path) else "c"
        shutil_copy = os.path.join(workdir, "c." + unit)
        with open(shutil_copy, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(open(path, encoding="utf-8").read())
        with open(os.path.join(workdir, "c.lds"), "w", encoding="utf-8") as handle:
            handle.write(linker_script(address, callees))
        driver = "ppu-lv2-g++" if unit == "cpp" else "ppu-lv2-gcc"
        # the C++ front end emits the exception tables, and the SDK's C++
        # runtime is not here: the tables are not needed to reproduce the
        # bytes, and the link only needs the personality routine
        extra = "-fno-exceptions -fno-rtti" if unit == "cpp" else ""
        command = (f"{PREFIX}/bin/{driver} -c -nostdinc -fno-builtin {extra} "
                   f"{' '.join(flags)} -o /w/c.o /w/c.{unit} && "
                   f"{PREFIX}/bin/ppu-lv2-ld -T /w/c.lds -o /w/c.elf /w/c.o")
        result = subprocess.run(
            ["docker", "run", "--rm", "-v", f"{workdir}:/w",
             "-v", f"{toolchain}:/usr/local/cell", image, "bash", "-c", command],
            capture_output=True, text=True)
        linked = os.path.join(workdir, "c.elf")
        if not os.path.exists(linked):
            return False, "compilation failed: " + (result.stdout + result.stderr)[-200:]
        compare = subprocess.run(
            [sys.executable, os.path.join(HERE, "compare_body.py"),
             "--elf", elf, "--object", linked, "--address", hex(address)],
            capture_output=True, text=True)
        text = (compare.stdout + compare.stderr).strip().splitlines()
        return compare.returncode == 0, text[0] if text else ""
    finally:
        import shutil
        shutil.rmtree(workdir, ignore_errors=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--elf", required=True, help="the retail executable")
    parser.add_argument("--toolchain", required=True,
                        help="where the SDK toolchain is mounted")
    parser.add_argument("--image", default="ps3gcc-toolchain",
                        help="the container the SDK compiler runs in")
    parser.add_argument("--sources", default=os.path.join(REPO, "src", "text"),
                        help="the directory of the sources")
    args = parser.parse_args(argv)

    paths = sources(args.sources)
    if not paths:
        print("no source found", file=sys.stderr)
        return 1
    matched = 0
    for path in paths:
        ok, message = build_and_compare(path, args.elf, args.toolchain, args.image)
        name = os.path.basename(path)
        print(f"{'OK  ' if ok else 'FAIL'} {name:38s} {message}")
        matched += ok
    print(f"\n{matched} of {len(paths)} sources reproduce the retail bytes")
    return 0 if matched == len(paths) else 1


if __name__ == "__main__":
    raise SystemExit(main())
