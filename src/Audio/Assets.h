
#include <memory>

#include "Audio.h"

struct AudioAssets {
    Music mainMenu;
    Music bgm;

    Sound wind;
    Sound woosh;
    Sound woosh2;
    Sound thump;
    Sound boost;
};

class Audio {
   public:
    std::unique_ptr<AudioSystem> system;
    std::unique_ptr<AudioBus> musicBus;
    std::unique_ptr<AudioBus> soundBus;
    std::unique_ptr<AudioAssets> assets;

    Audio() {
        system = std::make_unique<AudioSystem>();
        musicBus = std::make_unique<AudioBus>(*system);
        soundBus = std::make_unique<AudioBus>(*system);
    }

    void loadAssets() {
        assets = std::make_unique<AudioAssets>(std::move(AudioAssets{
            .mainMenu = Music(*musicBus, "assets/audio/music/title_theme.ogg"),
            .bgm = Music(*musicBus, "assets/audio/music/shifting_dunes.ogg"),
            .wind = Sound(*soundBus, "assets/audio/sound/wind-loop.ogg"),
            .woosh = Sound(*soundBus, "assets/audio/sound/woosh.wav"),
            .woosh2 = Sound(*soundBus, "assets/audio/sound/woosh2.wav"),
            .thump = Sound(*soundBus, "assets/audio/sound/thump.wav"),
            .boost = Sound(*soundBus, "assets/audio/sound/boost.ogg")}));
        assets->mainMenu.setLooping(true);
        assets->mainMenu.setVolume(0.8f);
        assets->bgm.setLooping(true);
        assets->bgm.setVolume(0.2f);
        assets->wind.setLooping(true);
        assets->boost.setLooping(true);
    }

    std::unique_ptr<Music> createMusic(std::string filename);

    std::unique_ptr<Sound> createSound(std::string filename);
};
