# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
"""Exercise the produced CPython runtime, without package indexes or a host fallback."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--package-root", required=True, type=Path)
parser.add_argument("--venv-output", required=True, type=Path)
parser.add_argument("--debug", action="store_true")
args = parser.parse_args()
home = args.package_root.resolve() / "python"
expected_executable = home / ("python_d.exe" if args.debug else "python.exe")
assert Path(sys.executable).resolve() == expected_executable.resolve(), sys.executable
assert Path(sys.base_prefix).resolve() == home, sys.base_prefix
assert sys.version_info[:3] == (3, 10, 13), sys.version
assert bool(hasattr(sys, "gettotalrefcount")) == args.debug, sys.version
assert sys.flags.isolated and sys.flags.no_user_site

os.environ["TCL_LIBRARY"] = str(home / "tcl" / "tcl8.6")
os.environ["TK_LIBRARY"] = str(home / "tcl" / "tk8.6")
assert Path(os.environ["TCL_LIBRARY"]).is_dir()
assert Path(os.environ["TK_LIBRARY"]).is_dir()

import bz2
import ctypes
import hashlib
import lzma
import pyexpat
import sqlite3
import ssl
import tkinter
import zlib

for module in (bz2, ctypes, lzma, sqlite3, ssl, tkinter):
    assert Path(module.__file__).resolve().is_relative_to(home), module.__file__
payload = bytes(range(256)) * 32
for codec in (zlib, bz2, lzma):
    assert codec.decompress(codec.compress(payload)) == payload
assert zlib.ZLIB_RUNTIME_VERSION == "1.2.13", zlib.ZLIB_RUNTIME_VERSION
assert sqlite3.sqlite_version == "3.40.1", sqlite3.sqlite_version
with sqlite3.connect(":memory:") as database:
    database.execute("create table values_local(value blob)")
    database.execute("insert into values_local values(?)", (payload,))
    assert database.execute("select value from values_local").fetchone()[0] == payload
assert ssl.OPENSSL_VERSION.startswith("OpenSSL 1.1.1u "), ssl.OPENSSL_VERSION
assert ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT).verify_mode == ssl.CERT_REQUIRED
assert hashlib.sha256(b"abc").hexdigest() == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
callback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)(lambda value: value + 7)
assert callback(35) == 42
assert tkinter.Tcl().eval("info patchlevel") == "8.6.12"
assert pyexpat.EXPAT_VERSION == "expat_2.4.6", pyexpat.EXPAT_VERSION
parsed = []
xml_parser = pyexpat.ParserCreate()
xml_parser.StartElementHandler = lambda name, attrs: parsed.append(name)
xml_parser.Parse(b"<local><value/></local>", True)
assert parsed == ["local", "value"]

if not args.debug:
    assert args.venv_output.is_absolute() and not args.venv_output.exists()
    env = os.environ.copy()
    env.update(PIP_NO_INDEX="1", PIP_CONFIG_FILE="NUL", PIP_DISABLE_PIP_VERSION_CHECK="1")
    subprocess.run([sys.executable, "-I", "-B", "-m", "venv", str(args.venv_output), "--without-pip"],
                   env=env, check=True)
    venv_python = args.venv_output / "Scripts" / "python.exe"
    subprocess.run([str(venv_python), "-I", "-B", "-m", "ensurepip", "--upgrade", "--default-pip"],
                   env=env, check=True)
    subprocess.run([str(venv_python), "-I", "-B", "-c",
                    "import sys,setuptools,pip; assert sys.version_info[:3] == (3,10,13); "
                    "assert sys.prefix != sys.base_prefix; assert pip.__version__ == '23.0.1'; "
                    "assert setuptools.__version__ == '65.5.0'"], env=env, check=True)

print("JE local CPython 3.10.13: runtime, compression, database, SSL, ctypes, Tcl and Expat passed")
