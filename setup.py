import os
import platform
import shutil
import sys
from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension
from pybind11.setup_helpers import build_ext
from setuptools import setup


def _truthy(name):
    return os.environ.get(name, "").lower() in (
        "1", "true", "yes", "on")


def _cibuildwheel():
    return os.environ.get("CIBUILDWHEEL") == "1"


# Python.org macOS builds are universal2. A fat
# binary cannot use -march=native. Never overwrite
# ARCHFLAGS: cibuildwheel sets them per-arch.
if sys.platform == "darwin":
    os.environ.setdefault(
        "MACOSX_DEPLOYMENT_TARGET", "11.0")
    if not _cibuildwheel() and "ARCHFLAGS" not in os.environ:
        os.environ["ARCHFLAGS"] = (
            "-arch " + platform.machine())


def _windows_gcc_available():
    if sys.platform != "win32":
        return False
    if shutil.which("g++"):
        return True
    if shutil.which("gcc"):
        os.environ.setdefault("CXX", "gcc")
        return True
    return False


def _use_mingw():
    # Public CPython wheels must use MSVC.
    if _cibuildwheel() or _truthy("FLSSS_MSVC"):
        return False
    return _windows_gcc_available()


def _want_native():
    if _cibuildwheel() or _truthy("FLSSS_PORTABLE"):
        return False
    if _truthy("FLSSS_NATIVE"):
        return True
    return False


ROOT = Path(__file__).parent.resolve()

try:
    import numpy as np
    numpy_inc = [np.get_include()]
except ImportError:
    numpy_inc = []

ext = Pybind11Extension(
    "flsss._core",
    ["python/flsss/_core.cpp"],
    include_dirs=[str(ROOT)] + numpy_inc,
    cxx_std=20,
)


class BuildExt(build_ext):
    def finalize_options(self):
        if (
            sys.platform == "win32"
            and not self.compiler
            and _use_mingw()
        ):
            self.compiler = "mingw32"
        super().finalize_options()

    def build_extensions(self):
        msvc = self.compiler.compiler_type == "msvc"
        native = _want_native()
        if msvc:
            # /GL+/LTCG can fail on this large TU in CI.
            opts = ["/O2", "/fp:fast", "/bigobj"]
            link = []
        else:
            # Pybind11Extension always injects MSVC flags
            # on Windows, even for MinGW.
            for e in self.extensions:
                e.extra_compile_args = [
                    a for a in e.extra_compile_args
                    if not str(a).startswith("/")]
                e.extra_link_args = [
                    a for a in e.extra_link_args
                    if not str(a).startswith("/")]
            opts = [
                "-O3",
                "-ffast-math",
                "-pthread",
                "-std=c++20",
            ]
            link = ["-pthread"]
            if native:
                opts += [
                    "-march=native",
                    "-mtune=native",
                    "-flto",
                ]
                link.append("-flto")
            if sys.platform == "win32":
                opts.append("-Wa,-mbig-obj")
        for e in self.extensions:
            e.extra_compile_args = (
                list(e.extra_compile_args) + opts)
            e.extra_link_args = (
                list(e.extra_link_args) + link)
        super().build_extensions()


setup(
    ext_modules=[ext],
    cmdclass={"build_ext": BuildExt},
)
