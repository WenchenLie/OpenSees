"""Guard against mixing Python ABIs in multi-version macOS builds."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "multi_python_build_macos.py"
SPEC = importlib.util.spec_from_file_location("macos_builder", SCRIPT)
builder = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(builder)


class PythonBuildConfigurationTests(unittest.TestCase):
    def test_tcl_runtime_and_startup_marker_on_stderr(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build = root / "py314" / "Release"
            build.mkdir(parents=True)
            source = root / "tcl-source"
            source.mkdir()
            (source / "init.tcl").write_text("# test Tcl runtime\n")
            (build / "CMakeCache.txt").write_text(f"TCL_INIT_FILE:FILEPATH={source / 'init.tcl'}\n")
            # OpenSees redirects Tcl's puts output to stderr.
            output = subprocess.CompletedProcess([], 0, "", "OPENSEES_RUNTIME_OK 3.8.0\n")
            with patch.object(builder.subprocess, "run", return_value=output) as run:
                builder.prepare_tcl_runtime(build, {"TCL_LIBRARY": "/wrong/tcl"})
            self.assertTrue((build.parent / "lib/tcl8.6/init.tcl").is_file())
            self.assertNotIn("TCL_LIBRARY", run.call_args.kwargs["env"])
            output.stderr = "application-specific initialization failed\nOPENSEES_RUNTIME_OK 3.8.0"
            with patch.object(builder.subprocess, "run", return_value=output):
                with self.assertRaisesRegex(ValueError, "启动验证失败"):
                    builder.prepare_tcl_runtime(build, {})

    def test_explicit_missing_interpreter_does_not_silently_skip(self):
        with self.assertRaises(OSError):
            builder.configurations(["3.14|/missing/opensees-test-python"], "arm64")

    def test_missing_automatic_versions_can_be_skipped(self):
        info = {"version": "3.14"}
        with patch.object(builder, "python_candidates", side_effect=[[], [Path("python")]]), \
                patch.object(builder, "inspect_python", return_value=info):
            self.assertEqual(builder.configurations(["3.11|auto", "3.14|auto"], "arm64"), [info])

    def test_duplicate_version_is_rejected(self):
        with patch.object(builder, "python_candidates", return_value=[Path("python")]), \
                patch.object(builder, "inspect_python", return_value={"version": "3.14"}):
            with self.assertRaisesRegex(ValueError, "重复"):
                builder.configurations(["3.14|auto", "3.14|auto"], "arm64")

    def test_wrong_architecture_or_version_is_rejected(self):
        for overrides in ({"arch": "x86_64"}, {"version": "3.15"}):
            info = dict(version="3.14", implementation="cpython", arch="arm64", bits=64)
            info.update(overrides)
            with self.subTest(overrides=overrides), \
                    patch.object(builder.subprocess, "check_output", return_value=json.dumps(info)):
                with self.assertRaisesRegex(ValueError, "需要原生"):
                    builder.inspect_python("python", "3.14", "arm64")

    def test_free_threaded_python_is_rejected(self):
        info = dict(version="3.14", implementation="cpython", arch="arm64", bits=64,
                    free_threaded=True, debug=False)
        with patch.object(builder.subprocess, "check_output", return_value=json.dumps(info)):
            with self.assertRaisesRegex(ValueError, "free-threaded"):
                builder.inspect_python("python", "3.14", "arm64")

    def test_both_cmake_python_families_must_match(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            info = dict(include=str(root / "3.14/include"), library=str(root / "3.14/libpython.dylib"),
                        executable=str(root / "3.14/python"))
            lines = [f"_Python_EXECUTABLE:INTERNAL={info['executable']}"]
            for family in ("Python", "Python3"):
                lines.extend([f"_{family}_INCLUDE_DIR:INTERNAL={info['include']}",
                              f"_{family}_LIBRARY_RELEASE:INTERNAL={info['library']}"])
            cache = root / "CMakeCache.txt"
            cache.write_text("\n".join(lines))
            builder.check_cmake_python(root, info)
            for field in ("INCLUDE_DIR", "LIBRARY_RELEASE"):
                mixed = [line.replace("3.14", "3.15")
                         if line.startswith(f"_Python3_{field}:") else line for line in lines]
                cache.write_text("\n".join(mixed))
                with self.subTest(field=field), self.assertRaisesRegex(ValueError, "Python3"):
                    builder.check_cmake_python(root, info)


if __name__ == "__main__":
    unittest.main()
