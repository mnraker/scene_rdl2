# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

if(NOT IsWindowsPlatform)
  add_subdirectory(numaInfo)
  add_subdirectory(shmFbDump)
  add_subdirectory(shmFbTool)
  add_subdirectory(shmFootmarkDump)
endif()
add_subdirectory(snapshotDeltaDump)
add_subdirectory(threadPoolExecutorTest)
