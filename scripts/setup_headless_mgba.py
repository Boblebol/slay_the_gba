# /// script
# requires-python = ">=3.11"
# dependencies = ["cmake", "cffi", "setuptools"]
# ///
"""Build the headless emulator used by scripts/gba_headless.py.

mGBA is compiled as a library, without any window, and its Python bindings are built against it. Everything
lands in tools/mgba/, which is not committed. Needs git and a C compiler (Xcode command line tools on macOS,
build-essential on Linux).

Run once from the repository root:  uv run scripts/setup_headless_mgba.py
"""

from __future__ import annotations

import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / "tools" / "mgba"
PACKAGE = WORK / "mgba"  # the importable `mgba` package, with the library next to it
# Third-party sources, compiled and run on the developer's machine and on the CI: each one is pinned to a
# commit, checked after the download, so a moved tag or branch cannot change what gets built.
MGBA_REPO = "https://github.com/mgba-emu/mgba.git"
MGBA_TAG, MGBA_COMMIT = "0.10.3", "1c61b54208ca6266129d0f2394c04bd8c44f98c5"
BINDINGS_REPO = "https://github.com/hanzi/libmgba-py.git"
BINDINGS_TAG, BINDINGS_COMMIT = "0.2.0-2", "524f63b1f2cd77d640c1c38d19617be8909829c6"

# The library only: no window, no video encoder, no scripting. The bindings are generated from the headers of
# this very build, so both always agree on what exists.
CMAKE_OPTIONS = [
    "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5", "-DBUILD_SHARED=ON", "-DBUILD_STATIC=OFF",
    "-DBUILD_QT=OFF", "-DBUILD_SDL=OFF", "-DBUILD_GL=OFF", "-DBUILD_GLES2=OFF", "-DBUILD_GLES3=OFF",
    "-DBUILD_UPDATER=OFF", "-DUSE_FFMPEG=OFF", "-DUSE_LUA=OFF", "-DUSE_DISCORD_RPC=OFF", "-DUSE_EDITLINE=OFF",
    "-DUSE_SQLITE3=OFF", "-DUSE_ELF=OFF", "-DUSE_LIBZIP=OFF", "-DUSE_MINIZIP=OFF", "-DUSE_EPOXY=OFF",
    "-DUSE_GDB_STUB=OFF", "-DUSE_PNG=OFF", "-DUSE_LZMA=OFF",
]

# Declared by the mGBA headers but only compiled with the video encoder: the bindings must not reference them.
MISSING_WITHOUT_FFMPEG = """
struct EReaderScan* EReaderScanLoadImagePNG(const char* filename);
struct EReaderScan* EReaderScanLoadImage(const void* pixels, unsigned width, unsigned height, unsigned stride);
struct EReaderScan* EReaderScanLoadImageA(const void* pixels, unsigned width, unsigned height, unsigned stride);
struct EReaderScan* EReaderScanLoadImage8(const void* pixels, unsigned width, unsigned height, unsigned stride);
void EReaderScanDestroy(struct EReaderScan*);
bool EReaderScanCard(struct EReaderScan*);
void EReaderScanOutputBitmap(const struct EReaderScan*, void* output, size_t stride);
bool EReaderScanSaveRaw(const struct EReaderScan*, const char* filename, bool strict);
"""


def run(*command: str | Path, cwd: Path | None = None) -> None:
    print("+", " ".join(str(part) for part in command), flush=True)
    subprocess.run([str(part) for part in command], cwd=cwd, check=True)


def clone(url: str, tag: str, commit: str, target: Path) -> None:
    """Downloads `tag` of `url` and refuses anything but the expected `commit`."""
    if not target.exists():
        run("git", "clone", "--quiet", "--depth", "1", "--branch", tag, url, target)
    head = subprocess.run(["git", "-C", str(target), "rev-parse", "HEAD"], check=True, capture_output=True,
                          text=True).stdout.strip()
    if head != commit:
        raise SystemExit(f"setup_headless_mgba: {target.name} is at {head}, expected {commit} ({url} {tag}); "
                         f"nothing was built. Delete {target} if the pin was changed on purpose.")


def library_file(build: Path) -> Path:
    pattern = "libmgba.*.*.*.dylib" if platform.system() == "Darwin" else "libmgba.so.*.*.*"
    found = sorted(build.glob(pattern))
    if not found:
        raise SystemExit(f"setup_headless_mgba: no {pattern} in {build}")
    return found[0]


def main() -> None:
    if platform.system() not in ("Darwin", "Linux"):
        raise SystemExit("setup_headless_mgba: only macOS and Linux are supported")
    WORK.mkdir(parents=True, exist_ok=True)
    bindings, source = WORK / "libmgba-py", WORK / "libmgba-py" / "mgba-src"
    build = source / "build"
    clone(BINDINGS_REPO, BINDINGS_TAG, BINDINGS_COMMIT, bindings)
    clone(MGBA_REPO, MGBA_TAG, MGBA_COMMIT, source)

    run("cmake", "-S", source, "-B", build, *CMAKE_OPTIONS)
    run("cmake", "--build", build, "--parallel")

    # The bindings read these two paths from _config.py; its Linux branch expects mGBA's Docker image.
    (bindings / "_config.py").write_text(
        "from pathlib import Path\n"
        f"path_to_libmgba_py = Path({str(bindings)!r})\n"
        f"path_to_mgba_root = Path({str(source)!r})\n"
        f"path_to_mgba_build = Path({str(build)!r})\n")
    blacklist = bindings / "_builder_blacklisted_lines.h"
    if "EReaderScanCard" not in blacklist.read_text():
        blacklist.write_text(blacklist.read_text() + MISSING_WITHOUT_FFMPEG)
    out = WORK / "out"
    run(sys.executable, "setup.py", "build", "--build-lib", out, cwd=bindings)

    if PACKAGE.exists():
        shutil.rmtree(PACKAGE)
    shutil.copytree(out / "mgba", PACKAGE)
    library = library_file(build)
    if platform.system() == "Darwin":
        shutil.copy(library, PACKAGE / "libmgba.0.10.dylib")
        # The extension looks for the library through @rpath: point it at its own folder.
        run("install_name_tool", "-add_rpath", "@loader_path", next(PACKAGE.glob("_pylib*.so")))
    else:
        shutil.copy(library, PACKAGE / "libmgba.so.0.10")  # loaded by gba_headless.py before the bindings
    print(f"Headless mGBA ready in {PACKAGE.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
