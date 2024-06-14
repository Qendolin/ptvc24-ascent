#pragma once

#include <glm/glm.hpp>
#include <string>

// Reference: https://solhsa.com/soloud/

namespace SoLoud {
class Soloud;
class Bus;
class Wav;
class WavStream;
typedef unsigned int handle;
}  // namespace SoLoud

class SoundInstance;
class SoundInstance3d;
class SoundInstance2d;

class AudioSystem {
    friend class AudioBus;
    friend class Music;
    friend class Sound;
    friend class SoundInstance;
    friend class SoundInstance2d;
    friend class SoundInstance3d;

   private:
    SoLoud::Soloud *soloud_;
    float volume_ = 1;

   public:
    AudioSystem();

    ~AudioSystem();

    void update(glm::vec3 position, glm::vec3 direction);

    void setVolume(float volume);
};

class AudioBus {
    friend class Music;
    friend class Sound;

   private:
    SoLoud::Bus *bus_;
    SoLoud::handle handle_ = 0;

   public:
    AudioSystem &system;

    AudioBus(AudioSystem &system);

    ~AudioBus();

    bool isPlaying();

    void play();

    void stop();

    void fadeFilterParam(int filter, int attribute, float value, float time_sec);

    void setFilterParam(int filter, int attribute, float value);

    void setVolume(float volume);
};

class Music {
   private:
    SoLoud::WavStream *wav_;
    SoLoud::handle handle_ = 0;

    bool paused_ = false;
    bool looping_ = false;
    float volume_ = 1;
    float speed_ = 1;
    float pan_ = 0;

   public:
    AudioBus &bus;
    Music(AudioBus &bus, std::string filename);

    // prevent copy
    Music(Music const &) = delete;
    Music &operator=(Music const &) = delete;

    // allow move
    Music(Music &&other);

    ~Music();

    float getVolume();

    void setVolume(float volume);

    void setPan(float pan);

    void setSpeed(float speed);

    void play();

    void pause();

    void setPaused(bool pause);

    void stop();

    bool isPlaying();

    void setLooping(bool looping);

    bool isLooping();

    double duration();

    void seek(double seconds);
};

class Sound {
   private:
    SoLoud::Wav *wav_;

   public:
    AudioBus &bus;
    Sound(AudioBus &bus, std::string filename);

    // prevent copy
    Sound(Sound const &) = delete;
    Sound &operator=(Sound const &) = delete;

    // allow move
    Sound(Sound &&other);

    ~Sound();

    void stop();

    void setVolume(float volume);

    void setLooping(bool looping);

    void setLoopPoint(double point);

    void set3dListenerRelative(bool relative);

    void set3dMinMaxDistance(float min, float max);

    void set3dDopplerFactor(float factor);

    double duration();

    SoundInstance3d play3dEvent(glm::vec3 position, float volume, glm::vec3 velocity = glm::vec3{0, 0, 0});

    SoundInstance2d play2dEvent(float volume, float pan = 0.0f);

    SoundInstance3d *play3d(glm::vec3 position, float volume, glm::vec3 velocity = glm::vec3{0, 0, 0});

    SoundInstance2d *play2d(float volume, float pan = 0.0f);
};

class SoundInstance {
   protected:
    SoLoud::handle handle_ = 0;
    bool liftimeBound_ = true;

    bool paused_ = false;
    bool looping_ = false;
    float volume_ = 1;
    float speed_ = 1;

   public:
    Sound &sound;
    SoundInstance(Sound &sound, unsigned int handle, bool lifetime_bound, float volume);

    // prevent copy
    SoundInstance(SoundInstance const &) = delete;
    SoundInstance &operator=(SoundInstance const &) = delete;

    virtual ~SoundInstance();

    float getVolume();

    void setVolume(float volume);

    void setSpeed(float speed);

    void play();

    void pause();

    void setPaused(bool pause);

    void stop();

    bool isPlaying();

    void setLooping(bool looping);

    bool isLooping();

    void seek(double seconds);
};

/**
 * Note: A sound source is not actually moved by its velocity. It is all static.
 */
class SoundInstance3d : public SoundInstance {
   private:
    glm::vec3 position_;
    glm::vec3 velocity_;

   public:
    SoundInstance3d(Sound &sound, unsigned int handle, bool lifetime_bound, float volume, glm::vec3 position, glm::vec3 veloity);

    void setPosition(glm::vec3 position);

    glm::vec3 getPosition();

    void setVelocity(glm::vec3 velocity);

    glm::vec3 getVelocity();

    void setPositionVelocity(glm::vec3 position, glm::vec3 velocity);
};

class SoundInstance2d : public SoundInstance {
   private:
    float pan_;

   public:
    SoundInstance2d(Sound &sound, unsigned int handle, bool lifetime_bound, float volume, float pan);

    void setPan(float pan);

    float getPan();
};
