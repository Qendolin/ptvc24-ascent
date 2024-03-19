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

static std::string loadTextFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }

    // Read the entire file into a string
    std::string content;
    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    content.resize(size);
    file.seekg(0, std::ios::beg);

    file.read(content.data(), size);

    return content;
}

// from https://stackoverflow.com/a/57838811/7448536
static std::vector<uint8_t> loadBinaryFile(const std::string filename) {
    // binary mode is only for switching off newline translation
    std::ifstream file(filename, std::ios::binary);
    file.unsetf(std::ios::skipws);

    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> vec;
    vec.reserve(size);
    vec.insert(vec.begin(),
               std::istream_iterator<uint8_t>(file),
               std::istream_iterator<uint8_t>());
    return vec;
}