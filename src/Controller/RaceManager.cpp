#include "RaceManager.h"

#include <chrono>
#include <string>

#include "../Game.h"
#include "../Input.h"
#include "../Scene/Character.h"
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
            lastPassedCheckpoint = 0;
        }
        return;
    }

    // next checkpoint
    if (index > lastPassedCheckpoint) {
        int32_t skipped = std::max(index - lastPassedCheckpoint - 1, 0);
        penaltyTime += skipped * 5;
        splits[index] = timer();
        for (int i = 0; i < skipped; i++) {
            splits[index - i] = splits[index];
        }

        lastPassedCheckpoint = index;
        respawnPoint_.transform = checkpoint->respawnTransformation().matrix();
        respawnPoint_.speed = glm::length(character_->velocity());
    }

    // last checkpoint (may also be first)
    if (index == checkpoints.size() - 1) {
        ended = true;
    }
}

void RaceManager::update(float delta_time) {
    if (started && !ended) {
        flightTime += delta_time;
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

float RaceManager::splitTimer() const {
    if (!started) return 0;
    ScoreEntry best = Game::get().scores->highScore();
    if (!best.valid) return 0;
    float time = timer();
    if (ended) return time - best.flight;
    int index = std::min(lastPassedCheckpoint + 1, static_cast<int>(best.splits.size() - 1));
    if (index < 0) return 0;
    return time - best.splits[index];
}

ScoreEntry RaceManager::score() {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return {
        .valid = ended,
        .timestamp = timestamp,
        .course = courseName,
        .flight = timer(),
        .penalty = penalty(),
        .total = timer() + penalty(),
        .splits = splits,
    };
}