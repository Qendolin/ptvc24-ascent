#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
// you can't believe how much time these two little shits have cost me today
#undef min
#undef max

#include "../Util/Log.h"
#include "Audio.h"

AudioSystem::AudioSystem() {
    soloud_ = new SoLoud::Soloud();
    soloud_->init();
}

AudioSystem::~AudioSystem() {
    soloud_->deinit();
    delete soloud_;
}

void AudioSystem::update(glm::vec3 position, glm::vec3 direction) {
    soloud_->set3dListenerPosition(position.x, position.y, position.z);
    soloud_->set3dListenerAt(direction.x, direction.y, direction.z);
    soloud_->update3dAudio();
}

AudioBus::AudioBus(AudioSystem &system) : system(system) {
    bus_ = new SoLoud::Bus();
    handle_ = system.soloud_->play(*bus_);
}

AudioBus::~AudioBus() {
    bus_->stop();
    delete bus_;
}

bool AudioBus::isPlaying() {
    return system.soloud_->isValidVoiceHandle(handle_);
}

void AudioBus::play() {
    if (system.soloud_->isValidVoiceHandle(handle_)) return;
    handle_ = system.soloud_->play(*bus_);
}

void AudioBus::stop() {
    if (handle_ == 0) return;
    bus_->stop();
    handle_ = 0;
}

void AudioBus::fadeFilterParam(int filter, int attribute, float value, float time_sec) {
    system.soloud_->fadeFilterParameter(handle_, filter, attribute, value, time_sec);
}

void AudioBus::setFilterParam(int filter, int attribute, float value) {
    system.soloud_->setFilterParameter(handle_, filter, attribute, value);
}

void AudioBus::setVolume(float volume) {
    system.soloud_->setVolume(handle_, volume);
}

Music::Music(AudioBus &bus, std::string filename) : bus(bus) {
    wav_ = new SoLoud::WavStream();
    auto result = wav_->load(filename.c_str());
    if (result != 0) {
        PANIC("Failed to load music from '" + filename + "'");
    }
    wav_->setSingleInstance(true);
}

Music::~Music() {
    if (wav_ != nullptr)
        wav_->stop();
    delete wav_;
}

float Music::getVolume() {
    return volume_;
}

void Music::setVolume(float volume) {
    volume_ = volume;
    if (handle_ == 0) return;

    bus.system.soloud_->setVolume(handle_, volume);
}

void Music::setPan(float pan) {
    pan_ = pan;
    if (handle_ == 0) return;

    bus.system.soloud_->setPan(handle_, pan);
}

void Music::setSpeed(float speed) {
    if (speed <= 0.0001f) {
        LOG_WARN("cannot set the speed to 0 or less");
        speed = 0.0001f;
    }

    speed_ = speed;
    if (handle_ == 0) return;

    bus.system.soloud_->setRelativePlaySpeed(handle_, speed);
}

void Music::play() {
    if (handle_ != 0 && paused_) {
        setPaused(false);
    } else if (handle_ == 0) {
        handle_ = bus.bus_->play(*wav_, volume_, pan_, false);
        bus.system.soloud_->setLooping(handle_, looping_);
        bus.system.soloud_->setRelativePlaySpeed(handle_, speed_);
        bus.system.soloud_->setProtectVoice(handle_, true);
        paused_ = false;
    }
}

void Music::pause() {
    setPaused(true);
}

void Music::setPaused(bool pause) {
    paused_ = pause;
    if (handle_ == 0) return;
    bus.system.soloud_->setPause(handle_, pause);
}

void Music::stop() {
    if (handle_ == 0) return;
    bus.system.soloud_->stop(handle_);
    handle_ = 0;
}

bool Music::isPlaying() {
    if (handle_ == 0) return false;

    return !paused_ && bus.system.soloud_->isValidVoiceHandle(handle_);
}

void Music::setLooping(bool looping) {
    looping_ = looping;
    if (handle_ == 0) return;

    bus.system.soloud_->setLooping(handle_, looping);
}

bool Music::isLooping() {
    return looping_;
}

double Music::duration() {
    return wav_->getLength();
}

void Music::seek(double seconds) {
    bus.system.soloud_->seek(handle_, seconds);
}

Sound::Sound(AudioBus &bus, std::string filename) : bus(bus) {
    wav_ = new SoLoud::Wav();
    auto result = wav_->load(filename.c_str());
    if (result != 0) {
        PANIC("Failed to load sound effect from '" + filename + "'");
    }
    wav_->set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, 0.2f);
    wav_->set3dDistanceDelay(false);
}

Sound::~Sound() {
    if (wav_ != nullptr)
        wav_->stop();
    delete wav_;
}

void Sound::stop() {
    wav_->stop();
}

void Sound::setVolume(float volume) {
    wav_->setVolume(volume);
}

void Sound::setLooping(bool looping) {
    wav_->setLooping(looping);
}

void Sound::setLoopPoint(double point) {
    wav_->setLoopPoint(point);
}

void Sound::set3dListenerRelative(bool relative) {
    wav_->set3dListenerRelative(relative);
}

void Sound::set3dMinMaxDistance(float min, float max) {
    wav_->set3dMinMaxDistance(min, max);
}

void Sound::set3dDopplerFactor(float factor) {
    wav_->set3dDopplerFactor(factor);
}

double Sound::duration() {
    return wav_->getLength();
}

SoundInstance3d Sound::play3dEvent(glm::vec3 position, float volume, glm::vec3 velocity) {
    auto handle = bus.bus_->play3d(*wav_, position.x, position.y, position.z, velocity.x, velocity.y, velocity.y, volume);
    return SoundInstance3d(*this, handle, false, volume, position, velocity);
}

SoundInstance2d Sound::play2dEvent(float volume, float pan) {
    auto handle = bus.bus_->play(*wav_, volume, pan);
    return SoundInstance2d(*this, handle, false, volume, pan);
}

SoundInstance3d *Sound::play3d(glm::vec3 position, float volume, glm::vec3 velocity) {
    auto handle = bus.bus_->play3d(*wav_, position.x, position.y, position.z, velocity.x, velocity.y, velocity.y, volume);
    return new SoundInstance3d(*this, handle, true, volume, position, velocity);
}

SoundInstance2d *Sound::play2d(float volume, float pan) {
    auto handle = bus.bus_->play(*wav_, volume, pan);
    return new SoundInstance2d(*this, handle, true, volume, pan);
}

SoundInstance::SoundInstance(Sound &sound, unsigned int handle, bool lifetime_bound, float volume)
    : sound(sound), handle_(handle), volume_(volume), liftimeBound_(lifetime_bound) {
}

SoundInstance::~SoundInstance() {
    if (liftimeBound_) {
        sound.bus.system.soloud_->stop(handle_);
    }
}

float SoundInstance::getVolume() {
    return volume_;
}

void SoundInstance::setVolume(float volume) {
    volume_ = volume;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->setVolume(handle_, volume);
}

void SoundInstance::setSpeed(float speed) {
    if (speed <= 0.0001f) {
        LOG_WARN("cannot set the speed to 0 or less");
        speed = 0.0001f;
    }

    speed_ = speed;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->setRelativePlaySpeed(handle_, speed);
}

void SoundInstance::play() {
    setPaused(false);
}

void SoundInstance::pause() {
    setPaused(true);
}

void SoundInstance::setPaused(bool pause) {
    paused_ = pause;
    if (handle_ == 0) return;
    sound.bus.system.soloud_->setPause(handle_, pause);
}

void SoundInstance::stop() {
    if (handle_ == 0) return;
    sound.bus.system.soloud_->stop(handle_);
    handle_ = 0;
}

bool SoundInstance::isPlaying() {
    if (handle_ == 0) return false;

    return !paused_ && sound.bus.system.soloud_->isValidVoiceHandle(handle_);
}

void SoundInstance::setLooping(bool looping) {
    looping_ = looping;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->setLooping(handle_, looping);
}

bool SoundInstance::isLooping() {
    return looping_;
}

void SoundInstance::seek(double seconds) {
    sound.bus.system.soloud_->seek(handle_, seconds);
}

SoundInstance3d::SoundInstance3d(Sound &sound, unsigned int handle, bool lifetime_bound, float volume, glm::vec3 position, glm::vec3 veloity)
    : SoundInstance(sound, handle, lifetime_bound, volume), position_(position_), velocity_(velocity_) {
}

void SoundInstance3d::setPosition(glm::vec3 position) {
    position_ = position;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->set3dSourcePosition(handle_, position.x, position.y, position.z);
}

glm::vec3 SoundInstance3d::getPosition() {
    return position_;
}

void SoundInstance3d::setVelocity(glm::vec3 velocity) {
    velocity_ = velocity;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->set3dSourceVelocity(handle_, velocity.x, velocity.y, velocity.z);
}

glm::vec3 SoundInstance3d::getVelocity() {
    return velocity_;
}

void SoundInstance3d::setPositionVelocity(glm::vec3 position, glm::vec3 velocity) {
    velocity_ = velocity;
    position_ = position;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->set3dSourceParameters(handle_, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
}

SoundInstance2d::SoundInstance2d(Sound &sound, unsigned int handle, bool lifetime_bound, float volume, float pan)
    : SoundInstance(sound, handle, lifetime_bound, volume), pan_(pan) {
}

void SoundInstance2d::setPan(float pan) {
    pan_ = pan;
    if (handle_ == 0) return;

    sound.bus.system.soloud_->setPan(handle_, pan);
}

float SoundInstance2d::getPan() {
    return pan_;
}