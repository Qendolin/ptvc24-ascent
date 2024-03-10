#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <stacktrace>
#include <string>

#define LOG(msg) \
    std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define PANIC(msg) \
    throw std::runtime_error(msg + std::string("\n") + std::to_string(std::stacktrace::current()));

static std::string loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }

    // Read the entire file into a string
    std::string content;
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    content.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(content.data(), size);

    return content;
}