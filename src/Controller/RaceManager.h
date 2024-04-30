#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../ScoreManager.h"

#pragma region ForwardDecl
class CheckpointEntity;
class CharacterEntity;
#pragma endregion

// Handles the logic of measuring score and checkpoints
class RaceManager {
   public:
    struct RespawnPoint {
        glm::mat4 transform;
        float speed;
        float boostMeter;
    };

   private:
    int lastPassedCheckpoint = -1;
    bool started = false;
    bool ended = false;
    float flightTime = 0.0;
    float penaltyTime = 0.0;
    std::vector<CheckpointEntity *> checkpoints;
    RespawnPoint respawnPoint_;
    std::vector<float> splits;
    std::string courseName;
    const CharacterEntity *character_;

   public:
    RaceManager() = default;
    RaceManager(const CharacterEntity *character, std::string course_name, RespawnPoint spawn)
        : character_(character), courseName(course_name), respawnPoint_(spawn) {
    }

    void onCheckpointEntered(CheckpointEntity *checkpoint);

    void loadCheckpoints(CheckpointEntity *start);

    RespawnPoint respawnPoint() {
        return respawnPoint_;
    }

    void update(float delta_time);

    // @return timer time in seconds
    float timer() const {
        return flightTime;
    }

    // @return time difference to best split of next checkpoint
    float splitTimer() const;

    // @return penalty time in seconds
    float penalty() const {
        return penaltyTime;
    }

    bool hasStarted() const {
        return started;
    }

    bool hasEnded() const {
        return ended;
    }

    ScoreEntry score();
};