"""Configure root registration with synthetic modules; no unfinished renderer code."""
import pathlib
import shutil
import subprocess
import sys
import tempfile

root = pathlib.Path(sys.argv[1])
with tempfile.TemporaryDirectory(prefix="opensim-renderer-") as temp:
    source = pathlib.Path(temp) / "source"
    source.mkdir()
    shutil.copy2(root / "CMakeLists.txt", source)
    shutil.copytree(root / "cmake", source / "cmake")
    for name in ("core", "math", "scene", "renderer"):
        module = source / "src" / name
        (module / "include").mkdir(parents=True)
        (module / "include" / "fixture.hpp").write_text("#pragma once\n")
        deps = " PUBLIC_DEPS core math scene" if name == "renderer" else ""
        content = f"opensim_add_module({name} HEADERS include/fixture.hpp{deps})\n"
        if name == "renderer":
            content += 'file(WRITE "${CMAKE_BINARY_DIR}/renderer-registered" "yes")\n'
        (module / "CMakeLists.txt").write_text(content)
    binary = pathlib.Path(temp) / "build"
    result = subprocess.run([sys.argv[2], "-S", str(source), "-B", str(binary), "-G", "Ninja",
        "-DOPENSIM_BUILD_TESTS=OFF", "-DOPENSIM_BUILD_APP=OFF",
        "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"], capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise SystemExit(result.stdout + result.stderr)
    assert (binary / "renderer-registered").is_file(), "Renderer omitted from headless build"
    assert not (binary / "_deps").exists(), "Headless fixture acquired dependencies"
    cache = (binary / "CMakeCache.txt").read_text()
    assert "CMAKE_C_COMPILER:" not in cache, "SDL C language enabled by include alone"
    print("Headless renderer registered without SDL, Catch2, Python discovery or C compiler")
