// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <winsock2.h>
#include <windows.h>
#include <openssl/dtls1.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
static_assert(sizeof(SOCKET) == sizeof(void*), "Windows x64 socket type");
