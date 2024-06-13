#include "RaceManager.h"

#include <chrono>
#include <string>

#include "../Audio/Assets.h"
#include "../Game.h"
#include "../Input.h"
#include "../Scene/Character.h"
#include "../Scene/Objects/Checkpoint.h"
#include "../Scene/Objects/CheckpointMarker.h"
#include "../Util/Log.h"

template <typename T>
int indexOf(const std::vector<T> &vec, const T elem) {
    auto it = std::find(vec.cbegin(), vec.cend(), elem);
    if (it == vec.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(vec.cbegin(), it));
}

RaceManager::RaceManager(const CharacterEntity *character, std::string course_name, RespawnPoint spawn)
    : character_(character), courseName_(course_name), respawnPoint_(spawn) {
}

void RaceManager::onCheckpointEntered(CheckpointEntity *checkpoint) {
    int index = indexOf(checkpoints_, checkpoint);
    LOG_DEBUG("Entered checkpoint '" + std::to_string(index) + "'");

    if (ended_) {
        return;
    }

    // next checkpoint
    if (index > lastPassedCheckpoint_) {
        int skipped = std::max(index - lastPassedCheckpoint_ - 1, 0);
        penaltyTime_ += skipped * 5;
        timeSplits_[index] = timer();
        for (int i = 0; i < skipped; i++) {
            timeSplits_[index - i - 1] = timeSplits_[index];
        }

        lastPassedCheckpoint_ = index;
        respawnPoint_.transform = checkpoint->respawnTransformation().matrix();
        respawnPoint_.speed = glm::length(character_->velocity());
        respawnPoint_.boostMeter = character_->boostMeter();

        int next = lastPassedCheckpoint_ + 1;
        if (next >= checkpoints_.size()) {
            checkpointMarker_->setTarget(scene::NodeRef());
        } else {
            checkpointMarker_->setTarget(checkpoints_[next]->getBase());
        }

        Game::get().audio->assets->woosh.play3dEvent(checkpoint->getBase().transform().position(), 1.5f);
    }

    // last checkpoint (may also be first)
    if (index == checkpoints_.size() - 1) {
        ended_ = true;
    }
}

void RaceManager::update(float delta_time) {
    if (started_ && !ended_) {
        flightTime_ += delta_time;
    }
}

void RaceManager::loadCheckpoints(CheckpointEntity *start) {
    auto scene = start->getScene();
    checkpointMarker_ = scene.create<CheckpointMarkerEntity>();
    checkpointMarker_->setTarget(start->getBase());
    checkpoints_.clear();
    checkpoints_.push_back(start);
    CheckpointEntity *current = start;
    while (current->hasNextCheckpoint()) {
        CheckpointEntity *next = current->nextCheckpoint();
        checkpoints_.push_back(next);
        current = next;
    }
    timeSplits_.resize(checkpoints_.size());
}

float RaceManager::splitTimer() const {
    if (!started_) return 0;
    ScoreEntry best = Game::get().scores->highScore();
    if (!best.valid) return 0;
    float time = timer();
    if (ended_) return time - best.flight;
    int index = std::min(lastPassedCheckpoint_ + 1, static_cast<int>(best.splits.size() - 1));
    if (index < 0) return 0;
    if (index >= best.splits.size()) return 0;
    return time - best.splits[index];
}

float RaceManager::splitTime(int index) const {
    if (index < 0 || index >= timeSplits_.size()) return 0.0;
    ScoreEntry best = Game::get().scores->highScore();
    if (!best.valid) return 0;
    if (index >= best.splits.size()) return 0;
    return timeSplits_[index] - best.splits[index];
}

ScoreEntry RaceManager::score() {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return {
        .valid = ended_,
        .timestamp = timestamp,
        .course = courseName_,
        .flight = timer(),
        .penalty = penalty(),
        .total = timer() + penalty(),
        .splits = timeSplits_,
    };
}