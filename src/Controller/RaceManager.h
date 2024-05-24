#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../ScoreManager.h"

#pragma region ForwardDecl
class CheckpointEntity;
class CharacterEntity;
class CheckpointMarkerEntity;
class Sound;
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
    int lastPassedCheckpoint_ = -1;
    bool started_ = false;
    bool ended_ = false;
    float flightTime_ = 0;
    float penaltyTime_ = 0;
    std::vector<CheckpointEntity *> checkpoints_;
    RespawnPoint respawnPoint_;
    std::vector<float> timeSplits_;
    std::string courseName_;
    const CharacterEntity *character_;
    CheckpointMarkerEntity *checkpointMarker_;

   public:
    RaceManager() = default;
    RaceManager(const CharacterEntity *character, std::string course_name, RespawnPoint spawn);

    void onCheckpointEntered(CheckpointEntity *checkpoint);

    void loadCheckpoints(CheckpointEntity *start);

    RespawnPoint respawnPoint() {
        return respawnPoint_;
    }

    void update(float delta_time);

    // @return timer time in seconds
    float timer() const {
        return flightTime_;
    }

    // @return time difference to best split of next checkpoint
    float splitTimer() const;

    // @return penalty time in seconds
    float penalty() const {
        return penaltyTime_;
    }

    bool hasStarted() const {
        return started_;
    }

    bool hasEnded() const {
        return ended_;
    }

    ScoreEntry score();
};