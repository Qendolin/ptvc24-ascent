#include "Assets.h"

#include "../Game.h"
#include "../Settings.h"
#include "Audio.h"

std::unique_ptr<Music> Audio::createMusic(std::string filename) {
    return std::make_unique<Music>(*musicBus, filename);
}

std::unique_ptr<Sound> Audio::createSound(std::string filename) {
    return std::make_unique<Sound>(*soundBus, filename);
}

void Audio::update(glm::vec3 listener_position, glm::vec3 listener_direction) {
    auto settings = Game::get().settings.get();
    musicBus->setVolume(std::pow(settings.musicVolume, 2.0f));
    soundBus->setVolume(std::pow(settings.soundVolume, 2.0f));
    system->setVolume(std::pow(settings.masterVolume, 2.0f));
    system->update(listener_position, listener_direction);
}