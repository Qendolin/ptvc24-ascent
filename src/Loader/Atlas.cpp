#include "Atlas.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_rect_pack.h>

#include <filesystem>

#include "../GL/Texture.h"
#include "../Util/Log.h"
#include "Loader.h"

namespace loader {
Atlas::Atlas(std::string filename, int32_t size, std::map<std::string, std::string> sources) : size_(size), filename_(filename) {
    bool generate = true;
    if (std::filesystem::exists(filename + ATLAS_FILE_EXT)) {
        generate = !checkAtlasMatch_(sources);
    }

    if (generate) {
        generate_(sources);
    } else {
        load_(sources);
    }
}

Atlas::~Atlas() {
    delete texture_;
}

void Atlas::generate_(std::map<std::string, std::string>& sources) {
    LOG_INFO("Generating atlas '" + filename_ + "'");
    std::vector<loader::Image> images;
    std::vector<std::string> keys;
    std::vector<stbrp_rect> rects;
    for (auto&& source : sources) {
        int index = static_cast<int>(images.size());
        images.emplace_back(loader::image(source.second));
        keys.emplace_back(source.first);
        rects.push_back(stbrp_rect{.id = index, .w = images.back().width, .h = images.back().height});
    }

    std::vector<stbrp_node> nodes;
    nodes.resize(size_ * 2);
    stbrp_context ctx;
    stbrp_init_target(&ctx, size_, size_, nodes.data(), nodes.size());
    bool success = stbrp_pack_rects(&ctx, rects.data(), rects.size());
    if (!success) {
        PANIC("Failed to pack atlas '" + filename_ + "'");
    }

    texture_ = new gl::Texture(GL_TEXTURE_2D);
    texture_->allocate(1, GL_RGBA8, size_, size_);

    CSV csv = {{"key", "file", "x", "y", "width", "height"}};
    for (auto&& rect : rects) {
        atlasMap_[keys[rect.id]] = {
            .x = rect.x,
            .y = rect.y,
            .w = rect.w,
            .h = rect.h,
        };
        glTextureSubImage2D(texture_->id(), 0, rect.x, rect.y, rect.w, rect.h, GL_RGBA, GL_UNSIGNED_BYTE, images[rect.id].data.get());
        std::vector<std::string> csv_row = {keys[rect.id], sources.at(keys[rect.id]), std::to_string(rect.x), std::to_string(rect.y), std::to_string(rect.w), std::to_string(rect.h)};
        csv.push_back(csv_row);
    }

    std::vector<uint8_t> pixels;
    pixels.resize(size_ * size_ * 4);
    glGetTextureImage(texture_->id(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.size(), pixels.data());
    stbi_write_png((filename_ + ".png").c_str(), size_, size_, 4, pixels.data(), size_ * 4);
    loader::writeCsv(filename_ + ATLAS_FILE_EXT, csv);
}

void Atlas::load_(std::map<std::string, std::string>& sources) {
    auto csv = loader::csv(filename_ + ATLAS_FILE_EXT);
    for (int i = 1; i < csv.size(); i++) {
        auto& row = csv[i];
        std::string key = row[0];
        atlasMap_[key] = {
            .x = std::stoi(row[2]),
            .y = std::stoi(row[3]),
            .w = std::stoi(row[4]),
            .h = std::stoi(row[5]),
        };
    }
    texture_ = loader::texture(filename_ + ".png", loader::TextureParameters{.mipmap = false});
}

bool Atlas::checkAtlasMatch_(std::map<std::string, std::string>& sources) {
    auto csv = loader::csv(filename_ + ATLAS_FILE_EXT);
    if (csv.size() != sources.size() + 1) {
        return false;
    }
    bool first_row = true;
    for (auto&& row : csv) {
        if (row.size() != 6) {
            return false;
        }
        if (first_row) {
            first_row = false;
            continue;
        }
        if (!sources.contains(row[0])) {
            // keys don't match
            return false;
        }
        if (sources.at(row[0]) != row[1]) {
            // files don't match
            return false;
        }
    }
    return true;
}

Atlas::Rect Atlas::rect(std::string key) const {
    if (atlasMap_.count(key) == 0) {
        PANIC("Key '" + key + "' is not contained in this atlas");
    }
    return atlasMap_.at(key);
}
}  // namespace loader
