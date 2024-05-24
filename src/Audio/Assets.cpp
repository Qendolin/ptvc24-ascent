#include "Assets.h"

std::unique_ptr<Music> Audio::createMusic(std::string filename) {
    return std::make_unique<Music>(*musicBus, filename);
}

std::unique_ptr<Sound> Audio::createSound(std::string filename) {
    return std::make_unique<Sound>(*soundBus, filename);
}