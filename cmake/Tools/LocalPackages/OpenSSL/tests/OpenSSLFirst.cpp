// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <openssl/dtls1.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <winsock2.h>
#include <windows.h>
static_assert(sizeof(SOCKET) == sizeof(void*), "Windows x64 socket type");
