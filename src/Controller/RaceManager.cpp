#include "RaceManager.h"

#include <chrono>
#include <string>

#include "../Game.h"
#include "../Input.h"
#include "../Scene/Objects/Checkpoint.h"
#include "../Util/Log.h"

template <typename T>
int32_t indexOf(const std::vector<T> &vec, const T elem) {
    auto it = std::find(vec.cbegin(), vec.cend(), elem);
    if (it == vec.end()) {
        return -1;
    }
    return static_cast<int32_t>(std::distance(vec.cbegin(), it));
}

void RaceManager::onCheckpointEntered(CheckpointEntity *checkpoint) {
    int32_t index = indexOf(checkpoints, checkpoint);
    LOG_DEBUG("Entered checkpoint '" + std::to_string(index) + "'");

    if (ended) {
        return;
    }

    if (!started) {
        if (index == 0) {
            started = true;
            startTime = Game::get().input->time();
            lastPassedCheckpoint = 0;
        }
        return;
    }

    // next checkpoint
    if (index > lastPassedCheckpoint) {
        int32_t skipped = std::max(index - lastPassedCheckpoint - 1, 0);
        penaltyTime += skipped * 5;
        lastPassedCheckpoint = index;
        splits[index] = timer();
    }

    // last checkpoint (may also be first)
    if (index == checkpoints.size() - 1) {
        ended = true;
        endTime = Game::get().input->time();
    }
}

void RaceManager::loadCheckpoints(CheckpointEntity *start) {
    checkpoints.clear();
    checkpoints.push_back(start);
    CheckpointEntity *current = start;
    while (current->hasNextCheckpoint()) {
        CheckpointEntity *next = current->nextCheckpoint();
        checkpoints.push_back(next);
        current = next;
    }
    splits.resize(checkpoints.size());
}

CheckpointEntity *RaceManager::getLastCheckpoint() {
    if (lastPassedCheckpoint < 0) return nullptr;
    return checkpoints[lastPassedCheckpoint];
}

float RaceManager::timer() {
    if (!started) return 0;
    if (ended) return static_cast<float>(endTime - startTime);
    return static_cast<float>(Game::get().input->time() - startTime);
}

ScoreEntry RaceManager::score() {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return {
        .invalid = !ended,
        .timestamp = timestamp,
        .course = courseName,
        .flight = timer(),
        .penalty = penalty(),
        .total = timer() + penalty(),
        .splits = splits,
    };
}