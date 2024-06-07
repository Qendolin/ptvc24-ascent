#include "Loader.h"

// After
#include <sstream>
#include <string>

#include "../Util/Log.h"

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
    file.close();

    return content;
}

std::ifstream stream(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }
    return file;
}

CSV csv(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }

    std::vector<std::vector<std::string>> data;
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream token_stream(line);
        while (std::getline(token_stream, token, ';')) {
            // trim whitespace
            token.erase(token.find_last_not_of(' ') + 1);
            token.erase(0, token.find_first_not_of(' '));
            tokens.push_back(token);
        }
        data.push_back(tokens);
    }
    file.close();
    return data;
}

void writeCsv(std::string filename, CSV& csv) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }
    for (const auto& row : csv) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) {
                file << ';';
            }
        }
        file << '\n';
    }

    file.close();
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
    file.close();
    return vec;
}
}  // namespace loader
