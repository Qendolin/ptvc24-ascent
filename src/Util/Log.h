#pragma once

#include <exception>
#include <iostream>
#include <stacktrace>
#include <string>

// https://stackoverflow.com/a/40947954/7448536
#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)

#define LOG_INFO(msg) \
    std::cout << "[LOG " << __FILENAME__ << ":" << __LINE__ << "]: " << msg << std::endl

#define LOG_WARN(msg) \
    std::cout << "[\u001B[33mWRN\u001B[0m " << __FILENAME__ << ":" << __LINE__ << "]: " << msg << std::endl

// LOG_DEBUG is removed in release builds
#ifndef NDEBUG
#define LOG_DEBUG(msg) \
    std::cout << "[DBG " << __FILENAME__ << ":" << __LINE__ << "]: " << msg << std::endl
#else
// clang-format off
#define LOG_DEBUG(msg) do {} while (0)
// clang-format on
#endif

#define PANIC(msg) \
    throw std::runtime_error(msg + std::string("\n") + std::to_string(std::stacktrace::current()));
