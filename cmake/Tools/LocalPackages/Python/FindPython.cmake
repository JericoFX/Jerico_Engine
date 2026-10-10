# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/python/win_x64/FindPython.cmake.

set(Python_DIR "${CMAKE_CURRENT_LIST_DIR}")
find_package(Python 3.10.13 REQUIRED CONFIG NO_DEFAULT_PATH PATHS "${Python_DIR}")
