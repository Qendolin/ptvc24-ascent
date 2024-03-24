#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <stacktrace>
#include <string>

#define LOG(msg) \
    std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define PANIC(msg) \
    throw std::runtime_error(msg + std::string("\n") + std::to_string(std::stacktrace::current()));
