// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

//
//
#pragma once

#include <cstring>     // memset()
#include <ctime>       // std::time(), std::localtime(), std::mktime()
#include <sstream>
#include <string>
#ifndef _MSC_VER
#include <sys/time.h>  // gettimeofday()
#else
#include <scene_rdl2/common/platform/Endian.h> // winsock2 has timeval
#endif
#include <time.h>      // time_t

#if __cplusplus >= 201703L
#include <chrono>
#endif

namespace scene_rdl2 {
namespace time_util {

inline
void init(struct timeval &tv)
{
    std::memset(&tv, 0x0, sizeof(tv));
}

inline
std::string
timeStr(const struct timeval &tv, bool usec = true)
{
    struct tm *time_st = localtime((const time_t*)&tv.tv_sec);

    static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    std::ostringstream ostr;
    ostr << time_st->tm_year + 1900 << "/"
         << month[time_st->tm_mon] << "/"
         << time_st->tm_mday << " "
         << wday[time_st->tm_wday] << " "
         << time_st->tm_hour << ":"
         << time_st->tm_min << ":"
         << time_st->tm_sec;
    if (usec) {
        ostr << ":"
             << tv.tv_usec / 1000;
    }
    return ostr.str();
}

inline
std::string
timeStr(const time_t &t)
{
    struct timeval tv;
    tv.tv_sec = t;
    tv.tv_usec = 0;
    return timeStr(tv, false);
}

inline
float
utcOffsetHours()
{
    std::time_t currTime = std::time(nullptr);

    const time_t timeLocal = std::mktime(std::localtime(&currTime));

    // Greenwich Mean Time (GMT) and Coordinated Universal Time (UTC) share the same current time in practice
    const time_t timeUTC = std::mktime(std::gmtime(&currTime));

    float diffSec = (float)std::difftime(timeLocal, timeUTC);
    return diffSec / (60.0f * 60.0f);
}

inline
struct timeval
getCurrentTime()
{
#if __cplusplus >= 201703L
    struct timeval tv;
    auto now = std::chrono::system_clock::now();
    auto s = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    uint64_t tv_sec = s.count();
    uint64_t tv_usec = us.count() - tv_sec * 1000 * 1000;
    tv.tv_sec = static_cast<long>(tv_sec);
    tv.tv_usec = static_cast<long>(tv_usec);
    return tv;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
#endif
}

inline
std::string
currentTimeStr()
{
    return timeStr(getCurrentTime());
}

} // namespace time_util
} // namespace scene_rdl2

