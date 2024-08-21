// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

//
//
#include "SockUtil.h"

#ifndef _MSC_VER
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <scene_rdl2/common/platform/Endian.h> // brings in winsock2.h
#endif

namespace scene_rdl2 {
namespace grid_util {

bool
setSockBufferSize(int sock, int level, int sizeBytes)
{
    int sendBuffSize = sizeBytes;
    if (::setsockopt(sock, level, SO_SNDBUF, (char*)&sendBuffSize, sizeof(sendBuffSize)) < 0) {
        return false;
    }

    int recvBuffSize = sizeBytes;
    if (::setsockopt(sock, level, SO_RCVBUF, (char*)&recvBuffSize, sizeof(recvBuffSize)) < 0) {
        return false;
    }

    return true;
}

} // namespace grid_util
} // namespace scene_rdl2

