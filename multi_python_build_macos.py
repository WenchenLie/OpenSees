#!/usr/bin/env python3
"""Native macOS builds with isolated CMake trees and CPython ABI-tagged modules."""
import argparse
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parent
PROBE = r'''
import json, platform, sys, sysconfig
from pathlib import Path
v = f"{sys.version_info.major}.{sys.version_info.minor}"
libdir = Path(sysconfig.get_config_var("LIBDIR") or sys.base_prefix)
candidates = [libdir / f"libpython{v}.dylib",
              libdir / (sysconfig.get_config_var("LDLIBRARY") or "missing"),
              Path(sys.base_prefix) / "Python"]
print(json.dumps(dict(version=v, executable=sys.executable,
    implementation=sys.implementation.name, arch=platform.machine(),
    bits=64 if sys.maxsize > 2**32 else 32, prefix=sys.base_prefix,
    include=sysconfig.get_path("include"),
    library=next((str(p) for p in candidates if p.is_file()), ""),
    suffix=sysconfig.get_config_var("EXT_SUFFIX"),
    free_threaded=bool(sysconfig.get_config_var("Py_GIL_DISABLED")),
    debug=bool(sysconfig.get_config_var("Py_DEBUG")))))
'''
SMOKE = r'''
import math, pathlib, sys
sys.path.insert(0, sys.argv[1])
import opensees as ops
assert pathlib.Path(ops.__file__).resolve().parent == pathlib.Path(sys.argv[1]).resolve()
ops.wipe()
ops.model("basic", "-ndm", 1, "-ndf", 1)
ops.node(1, 0.0)
ops.node(2, 2.0)
ops.fix(1, 1)
ops.uniaxialMaterial("Elastic", 1, 2.0e11)
ops.element("Truss", 1, 1, 2, 0.01, 1)
ops.timeSeries("Linear", 1)
ops.pattern("Plain", 1, 1)
ops.load(2, 100.0)
ops.constraints("Plain")
ops.numberer("Plain")
ops.system("BandSPD")
ops.algorithm("Linear")
ops.integrator("LoadControl", 1.0)
ops.analysis("Static")
assert ops.analyze(1) == 0
assert math.isclose(ops.nodeDisp(2, 1), 1e-7, rel_tol=1e-10)
ops.reactions()
assert math.isclose(ops.nodeReaction(1, 1), -100.0, rel_tol=1e-10)
print("PASS:", ops.__file__, "OpenSees", ops.version(), "位移与支座反力符合解析解")
ops.wipe()
'''


def run(args, env=None, capture=False):
    args = [str(a) for a in args]
    print("+", shlex.join(args), flush=True)
    result = subprocess.run(args, cwd=ROOT, env=env, check=True, text=True,
                            stdout=subprocess.PIPE if capture else None)
    return result.stdout if capture else None


def inspect_python(executable, version, arch):
    info = json.loads(subprocess.check_output(
        [str(executable), "-I", "-c", PROBE], text=True))
    if (info["version"] != version or info["implementation"] != "cpython"
            or info["bits"] != 64 or info["arch"] != arch):
        raise ValueError(f"需要原生 {arch} CPython {version}，实际为 {info['arch']} "
                         f"{info['implementation']} {info['version']}: {executable}")
    if info["free_threaded"] or info["debug"]:
        raise ValueError(f"暂不支持 free-threaded 或 debug Python: {executable}")
    if not info["suffix"] or not re.fullmatch(r"\.cpython-\d+-darwin\.so", info["suffix"]):
        raise ValueError(f"Python 模块后缀不受支持: {info['suffix']}")
    if not (Path(info["include"]) / "Python.h").is_file() or not info["library"]:
        raise ValueError(f"缺少 Python.h 或共享开发库: {executable}")
    return info


def python_candidates(version):
    candidates = [shutil.which(f"python{version}"),
                  f"/Library/Frameworks/Python.framework/Versions/{version}/bin/python{version}"]
    for prefix in ("/opt/homebrew", "/usr/local"):
        candidates.extend([f"{prefix}/opt/python@{version}/bin/python{version}",
                           f"{prefix}/bin/python{version}"])
    return [Path(p) for p in candidates if p and Path(p).is_file()]


def configurations(rows, arch):
    builds, seen = [], set()
    for row in rows:
        version, separator, executable = row.partition("|")
        version, executable = version.strip(), executable.strip()
        if not separator or not re.fullmatch(r"3\.\d+", version) or not executable:
            raise ValueError(f"Python 配置格式应为 3.14|auto 或 3.14|/绝对路径/python3.14: {row}")
        if version in seen:
            raise ValueError(f"重复的 Python 版本配置: {version}")
        seen.add(version)
        if executable == "auto":
            candidates = python_candidates(version)
            if not candidates:
                print(f"跳过 Python {version}：未安装。")
                continue
            errors = []
            for candidate in candidates:
                try:
                    builds.append(inspect_python(candidate, version, arch))
                    break
                except (ValueError, OSError, subprocess.CalledProcessError) as exc:
                    errors.append(str(exc))
            else:
                raise ValueError("\n".join(errors))
        else:
            path = Path(executable).expanduser()
            if not path.is_absolute():
                raise ValueError(f"请使用 Python 的绝对路径: {path}")
            builds.append(inspect_python(path, version, arch))
    if not builds:
        raise ValueError("未找到可编译的 Python。请编辑 build_macos.command 中的版本列表。")
    return sorted(builds, key=lambda info: tuple(map(int, info["version"].split("."))))


def check_cmake_python(build_dir, info):
    cache = {}
    for line in (build_dir / "CMakeCache.txt").read_text().splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line and ":" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    # Both FindPython and FindPython3 are used by the source tree.
    for family in ("Python", "Python3"):
        for field, expected in (("INCLUDE_DIR", info["include"]),
                                ("LIBRARY_RELEASE", info["library"])):
            actual = cache.get(f"_{family}_{field}")
            if not actual or Path(actual).resolve() != Path(expected).resolve():
                raise ValueError(f"CMake {family} {field} 与所选 Python 不一致: {actual}")
    actual = cache.get("_Python_EXECUTABLE")
    if not actual or Path(actual).resolve() != Path(info["executable"]).resolve():
        raise ValueError(f"CMake 选错了 Python 解释器: {actual}")


def prepare_tcl_runtime(build_dir, env):
    # Static Tcl retains its build-time prefix. Bundle its runtime at the
    # ../lib/tcl8.6 location Tcl searches relative to the executable.
    cache = (build_dir / "CMakeCache.txt").read_text()
    match = re.search(r"^TCL_INIT_FILE:[^=]+=(.+)$", cache, re.MULTILINE)
    if not match or not Path(match[1]).is_file():
        raise ValueError("未找到 Tcl 的 init.tcl，无法准备独立 OpenSees 程序的运行文件。")
    runtime = build_dir.parent / "lib" / "tcl8.6"
    shutil.copytree(Path(match[1]).parent, runtime, dirs_exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="tcl-check-", dir=build_dir) as temporary:
        script = Path(temporary) / "check.tcl"
        script.write_text('puts "OPENSEES_RUNTIME_OK [version]"\nexit\n')
        # Do not let a shell's TCL_LIBRARY mask an incorrectly packaged runtime.
        test_env = {k: v for k, v in env.items() if k not in ("TCL_LIBRARY", "TCLLIBPATH")}
        output = subprocess.run([str(build_dir / "OpenSees"), str(script)], cwd=temporary,
                                env=test_env, text=True, capture_output=True, check=True)
        combined = output.stdout + output.stderr
        if "OPENSEES_RUNTIME_OK " not in combined or "initialization failed" in combined:
            raise ValueError(f"OpenSees Tcl 启动验证失败:\n{combined}")
        print(next(line for line in combined.splitlines() if line.startswith("OPENSEES_RUNTIME_OK ")),
              flush=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description="macOS 原生多版本 OpenSees Python 编译")
    parser.add_argument("--python", action="append", metavar="VERSION|PATH",
                        help="可重复；PATH 为 auto 或解释器绝对路径")
    parser.add_argument("--list-python", action="store_true", help="仅检查 Python 配置")
    parser.add_argument("--configure-only", action="store_true", help="准备依赖并配置，不编译")
    parser.add_argument("--module-only", action="store_true", help="只编译 Python 模块")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 4, 8))
    options = parser.parse_args(argv)
    if sys.platform != "darwin" or platform.machine() not in ("arm64", "x86_64"):
        raise ValueError("请在原生 macOS arm64 或 x86_64 环境运行。")
    if options.jobs < 1:
        raise ValueError("--jobs 必须大于 0。")
    arch = platform.machine()
    builds = configurations(options.python or [f"3.{n}|auto" for n in range(11, 16)], arch)
    for info in builds:
        print(f"Python {info['version']}: {info['executable']} -> opensees{info['suffix']}")
        print(f"  Headers: {info['include']}\n  Library: {info['library']}")
    if options.list_python:
        return
    for tool in ("cmake", "conan", "clang", "clang++", "gfortran", "make"):
        if not shutil.which(tool):
            raise ValueError(f"缺少工具 {tool}。请安装 Xcode Command Line Tools 及 brew install cmake conan gcc。")
    conan_version = run(["conan", "--version"], capture=True)
    if not re.search(r"Conan version 2\.", conan_version):
        raise ValueError("此项目需要 Conan 2。")
    env = os.environ.copy()
    env.update(CC=shutil.which("clang"), CXX=shutil.which("clang++"),
               FC=shutil.which("gfortran"), CMAKE_POLICY_VERSION_MINIMUM="3.5")
    base = ROOT / "build" / f"macos-{arch}"
    base.mkdir(parents=True, exist_ok=True)
    profile = base / "conan-profile"
    if not profile.exists():
        run(["conan", "profile", "detect", "--name", profile], env=env)
    profile_data = json.loads(run(["conan", "profile", "show", "-pr:h", profile,
                                   "-pr:b", profile, "--format=json"], env=env, capture=True))
    expected_arch = "armv8" if arch == "arm64" else "x86_64"
    for role in ("host", "build"):
        settings = profile_data[role]["settings"]
        if (settings.get("os") != "Macos" or settings.get("arch") != expected_arch
                or settings.get("compiler") != "apple-clang"):
            raise ValueError(f"Conan 配置与原生 Apple Clang 环境不一致，请检查 {profile}")
    deps = base / "deps"
    run(["conan", "install", ROOT, "--output-folder", deps, "--build=missing",
         "-pr:h", profile, "-pr:b", profile, "-s", "build_type=Release",
         "-s", "compiler.cppstd=17", "-c", "tools.cmake.cmaketoolchain:user_presets="], env=env)
    toolchain = deps / "build/Release/generators/conan_toolchain.cmake"
    if not toolchain.is_file():
        raise ValueError(f"未生成 Conan toolchain: {toolchain}")
    modules = base / "python-modules"
    records = []
    for index, info in enumerate(builds):
        tag = "py" + info["version"].replace(".", "")
        build_dir = base / tag / "Release"
        print(f"\n=== Python {info['version']} / {arch}: {build_dir} ===", flush=True)
        args = ["cmake", "-S", ROOT, "-B", build_dir, "-G", "Unix Makefiles",
                f"-DCMAKE_TOOLCHAIN_FILE={toolchain}", "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_POLICY_VERSION_MINIMUM=3.5", f"-DCMAKE_OSX_ARCHITECTURES={arch}",
                f"-DCMAKE_Fortran_COMPILER={env['FC']}", "-DOPS_FINAL_TARGET=OpenSeesPy",
                "-DBUILD_TESTING=OFF"]
        for family in ("Python", "Python3"):
            args.extend([f"-D{family}_ROOT_DIR={info['prefix']}",
                         f"-D{family}_INCLUDE_DIR={info['include']}",
                         f"-D{family}_LIBRARY={info['library']}",
                         f"-D{family}_FIND_STRATEGY=LOCATION"])
        args.append(f"-DPython_EXECUTABLE={info['executable']}")
        run(args, env=env)
        check_cmake_python(build_dir, info)
        if options.configure_only:
            continue
        if index == 0 and not options.module_only:
            run(["cmake", "--build", build_dir, "--target", "OpenSees",
                 "--parallel", options.jobs], env=env)
            prepare_tcl_runtime(build_dir, env)
        run(["cmake", "--build", build_dir, "--target", "OpenSeesPy",
             "--parallel", options.jobs], env=env)
        modules.mkdir(parents=True, exist_ok=True)
        destination = modules / ("opensees" + info["suffix"])
        # A failed import/test must not overwrite a previously working module.
        with tempfile.TemporaryDirectory(prefix="verify-", dir=base) as staging:
            staged = Path(staging) / destination.name
            shutil.copy2(build_dir / "OpenSeesPy.dylib", staged)
            run([info["executable"], "-I", "-c", SMOKE, staging], env=env)
            pending = destination.with_suffix(".so.pending")
            shutil.copy2(staged, pending)
            pending.replace(destination)
        records.append(dict(info, module=str(destination), build_directory=str(build_dir)))
        print(f"已生成并验证：{destination}", flush=True)
    if options.configure_only:
        print("\n全部 Python 配置成功；尚未编译。")
    else:
        manifest = modules / "last-build.json"
        pending = manifest.with_suffix(".json.pending")
        pending.write_text(json.dumps(records, indent=2, ensure_ascii=False) + "\n")
        pending.replace(manifest)
        print(f"\n全部完成。Python 模块：{modules}")
        if not options.module_only:
            print(f"OpenSees 程序：{records[0]['build_directory']}/OpenSees")
        print(f"在对应 Python 中添加该目录到 sys.path，再 import opensees。")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"\n错误：{error}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n已取消；保留构建目录以便下次继续。", file=sys.stderr)
        sys.exit(130)
