#pragma once

#include <array>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../Camera.h"
#include "../GL/Declarations.h"

// Referece:
// https://juandiegomontoya.github.io/particles.html

template <typename T>
struct Range {
    T min;
    T max;
    Range() : min(0), max(0) {}
    Range(T value) : min(value), max(value) {}
    Range(T min, T max) : min(min), max(max) {}

    T range() const {
        return max - min;
    }

    Range<T> &operator=(const T &value) {
        min = value;
        max = value;
        return *this;
    }
};

enum class ParticleBlending {
    None,
    AlphaClip,
    Additive,
};

struct ParticleMaterial {
    ParticleBlending blending = ParticleBlending::None;
    gl::Texture *sprite = nullptr;
    gl::Texture *tint = nullptr;
    gl::Texture *scale = nullptr;

    void destroy();
};

struct ParticleMaterialParams {
    ParticleBlending blending = ParticleBlending::None;
    // Path of the sprite texture. Make sure to save colors of transparent pixels.
    std::string sprite = "";
    std::string tint = "";
    std::string scale = "";
};

struct ParticleSettings {
    Range<float> frequency = 10;
    Range<int> count = 1;
    Range<float> life = 5;

    glm::vec3 position = glm::vec3{0, 0, 0};
    glm::vec3 direction = glm::vec3{0, 1, 0};
    // from 0 to 180 in degrees
    Range<float> spread = 0;
    glm::vec3 gravity = glm::vec3{0, 0, 0};

    Range<float> velocity = 10;
    Range<float> gravityFactor = 1;
    Range<float> drag = 0;
    Range<float> rotation = 0;
    Range<float> revolutions = 0;
    float emissivity = 0;
    glm::vec2 size = glm::vec2{1, 1};
    Range<float> scale = 1;
    float stretching = 0;
};

class ParticleEmitter {
   public:
    struct Segment {
        int index = 0;
        int length = 0;

        Segment(int index, int length) : index(index), length(length) {}
        Segment(std::pair<int, int> range) : index(range.first), length(range.second - range.first) {}
    };

   private:
    Segment segment_ = Segment(-1, 0);
    float timer_ = 0;
    int count_ = 0;
    float interval_ = 0;
    ParticleSettings settings_;

   public:
    bool enabled = false;
    std::string material;

    ParticleEmitter() = default;
    ParticleEmitter(ParticleSettings settings, Segment segment) : settings_(settings), segment_(segment) {
        enabled = true;
    };
    ~ParticleEmitter();

    Segment segment() const {
        return segment_;
    }

    float nextInterval() const {
        return interval_;
    }

    float intervalTimer() const {
        return timer_;
    }

    int intervalCount() const {
        return count_;
    }

    ParticleSettings &settings() {
        return settings_;
    }

    void update(float time_delta);
};

struct Emission;

class ParticleSystem {
   public:
    static const int MAX_EMITTERS = 256;

   private:
    int capacity_;
    int reserved_ = 0;
    gl::Buffer *particleBuffer_;
    gl::Buffer *freeBuffer_;
    gl::Buffer *freeHeadsBuffer_;
    gl::Buffer *emitterBuffer_;
    gl::ShaderPipeline *emitShader_;
    gl::ShaderPipeline *updateShader_;
    gl::ShaderPipeline *drawShader_;
    gl::ShaderPipeline *resetShader_;
    gl::VertexArray *quad_;
    gl::Sampler *spriteSampler_;
    gl::Sampler *tableSampler_;
    std::vector<std::pair<int, int>> segmentation_;
    std::array<ParticleEmitter, MAX_EMITTERS> emitterPool_;
    std::vector<ParticleEmitter *> emitters_;
    std::vector<int> freeEmitterIndices_;
    std::map<std::string, ParticleMaterial> materials_;

    void emit_(Emission &emission);

    std::pair<int, int> allocateSegment_(int length);
    void freeSegment_(int index, int length);

   public:
    ParticleSystem(int capacity);

    ~ParticleSystem();

    ParticleEmitter *add(ParticleSettings settings, std::string material);

    void remove(ParticleEmitter *emitter);

    void loadMaterial(std::string name, ParticleMaterialParams params);

    void update(float time_delta);

    void draw(Camera &camera);

    int capacity() const {
        return capacity_;
    }

    int reserved() const {
        return reserved_;
    }

    std::vector<ParticleEmitter *> emitters() {
        return emitters_;
    }

    std::map<std::string, ParticleMaterial> &materials() {
        return materials_;
    }
};