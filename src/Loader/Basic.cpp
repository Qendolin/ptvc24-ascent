#include "../Util/Log.h"
#include "Loader.h"

namespace loader {
std::string text(std::string filename) {
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

std::ifstream stream(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }
    return file;
}

// from https://stackoverflow.com/a/57838811/7448536
std::vector<uint8_t> binary(std::string filename) {
    // binary mode is only for switching off newline translation
    std::ifstream file(filename, std::ios::binary);
    file.unsetf(std::ios::skipws);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }

    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> vec;
    vec.resize(size);
    file.read(reinterpret_cast<char*>(vec.data()), vec.size());
    return vec;
}
}  // namespace loader
