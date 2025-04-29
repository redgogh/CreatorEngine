/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 RedGogh All rights reserved.                          *|
|*                                                                                  *|
|*    Licensed under the Apache License, Version 2.0 (the "License");               *|
|*    you may not use this file except in compliance with the License.              *|
|*    You may obtain a copy of the License at                                       *|
|*                                                                                  *|
|*        http://www.apache.org/licenses/LICENSE-2.0                                *|
|*                                                                                  *|
|*    Unless required by applicable law or agreed to in writing, software           *|
|*    distributed under the License is distributed on an "AS IS" BASIS,             *|
|*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.      *|
|*    See the License for the specific language governing permissions and           *|
|*    limitations under the License.                                                *|
|*                                                                                  *|
\* -------------------------------------------------------------------------------- */

/* Create by Red Gogh on 2025/4/22 */

#pragma once

#include <cstdio>
#include <chrono>
#include <thread>

#define GOGH_COLOR_RESET   "\033[0m"
#define GOGH_COLOR_RED     "\033[31m"
#define GOGH_COLOR_GREEN   "\033[32m"
#define GOGH_COLOR_YELLOW  "\033[33m"
#define GOGH_COLOR_BLUE    "\033[34m"
#define GOGH_COLOR_CYAN    "\033[36m"
#define GOGH_COLOR_GRAY    "\033[90m"

inline void _LogPrefix(char* buffer, size_t size) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;

    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &time_t_now);
#else
    localtime_r(&time_t_now, &local_tm);
#endif

    std::ostringstream oss;
    oss << std::this_thread::get_id();
    snprintf(buffer, size, "[%02d:%02d:%02d.%03lld] [tid:%s]",
             local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
             static_cast<long long>(ms), oss.str().c_str());
}

#define GOGH_LOG_IMPL(color, level, ...) \
    do { \
        char _buf[128]; \
        _LogPrefix(_buf, sizeof(_buf)); \
        printf("%s " color "[%-5s]" GOGH_COLOR_RESET " --- ", _buf, level); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)

#define GOGH_LOGGER_INFO(...)  GOGH_LOG_IMPL(GOGH_COLOR_GREEN,  "INFO",  __VA_ARGS__)
#define GOGH_LOGGER_WARN(...)  GOGH_LOG_IMPL(GOGH_COLOR_YELLOW, "WARN",  __VA_ARGS__)
#define GOGH_LOGGER_ERROR(...) GOGH_LOG_IMPL(GOGH_COLOR_RED,    "ERROR", __VA_ARGS__)

#ifdef NDEBUG
#  define GOGH_LOGGER_DEBUG(...) ((void*)0)
#else
#  define GOGH_LOGGER_DEBUG(...) GOGH_LOG_IMPL(GOGH_COLOR_CYAN,   "DEBUG", __VA_ARGS__)
#endif /* NDEBUG */