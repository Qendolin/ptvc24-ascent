#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
#pragma endregion

namespace loader {
class Atlas {
   public:
    struct Rect {
        int32_t x;
        int32_t y;
        int32_t w;
        int32_t h;
    };

   private:
    inline static const std::string ATLAS_FILE_EXT = ".atlas";

    gl::Texture* texture_;
    std::map<std::string, Rect> atlasMap_;
    int32_t size_;
    std::string filename_;

    bool checkAtlasMatch_(std::map<std::string, std::string>& sources);

    void generate_(std::map<std::string, std::string>& sources);

    void load_(std::map<std::string, std::string>& sources);

   public:
    Atlas(std::string filename, int32_t size, std::map<std::string, std::string> sources);
    ~Atlas();

    Rect rect(std::string key) const;

    int32_t size() const {
        return size_;
    }

    gl::Texture& texture() const {
        return *texture_;
    }
};
}  // namespace loader