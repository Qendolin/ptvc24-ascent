#pragma once

#include <vector>

#include "../ScoreManager.h"

#pragma region ForwardDecl
class CheckpointEntity;
#pragma endregion

// Handles the logic of measuring score and checkpoints
class RaceManager {
    int lastPassedCheckpoint = -1;
    bool started = false;
    bool ended = false;
    double startTime = -1.0;
    double endTime = -1.0;
    float penaltyTime = 0.0;
    std::vector<CheckpointEntity *> checkpoints;
    std::vector<float> splits;
    std::string courseName;

   public:
    RaceManager() = default;
    RaceManager(std::string course_name) : courseName(course_name) {
    }

    void onCheckpointEntered(CheckpointEntity *checkpoint);

    void loadCheckpoints(CheckpointEntity *start);

    CheckpointEntity *getLastCheckpoint();

    // @return timer time in seconds
    float timer();

    // @return time difference to best split
    float splitTimer();

    // @return penalty time in seconds
    float penalty() {
        return penaltyTime;
    }

    bool hasStarted() {
        return started;
    }

    bool hasEnded() {
        return ended;
    }

    ScoreEntry score();
};