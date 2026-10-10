// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <Python.h>

#include <filesystem>
#include <iostream>
#include <string>

int main()
{
    if (std::string(PY_VERSION) != "3.10.13" || std::string(Py_GetVersion()).rfind("3.10.13 ", 0) != 0)
    {
        std::cerr << "JE Python: header/runtime version mismatch\n";
        return 1;
    }
    const std::wstring home = std::filesystem::path(DEFAULT_LY_PYTHONHOME).wstring();
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.write_bytecode = 0;
    PyStatus status = PyConfig_SetString(&config, &config.home, home.c_str());
    if (!PyStatus_Exception(status))
    {
        status = Py_InitializeFromConfig(&config);
    }
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
    {
        std::cerr << "JE Python: isolated initialization failed\n";
        return 1;
    }
    const int result = PyRun_SimpleString(
        "import sys, zlib, sqlite3\n"
        "assert sys.version_info[:3] == (3, 10, 13)\n"
        "assert zlib.decompress(zlib.compress(b'JE embedded Python' * 256)) == b'JE embedded Python' * 256\n"
        "with sqlite3.connect(':memory:') as db:\n"
        "    assert db.execute('select 6 * 7').fetchone() == (42,)\n");
    const int finalization = Py_FinalizeEx();
    if (result != 0 || finalization != 0)
    {
        std::cerr << "JE Python: embedded compression/database check failed\n";
        return 1;
    }
    std::cout << "JE local Python 3.10.13: embedded C API roundtrip passed\n";
    return 0;
}
