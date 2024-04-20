#pragma once

#include <fstream>
#include <string>
#include <vector>

struct ScoreEntry {
    bool valid = false;
    uint64_t timestamp = 0;
    std::string course = "";
    float flight = 0;
    float penalty = 0;
    float total = 0;
    std::vector<float> splits;
};

class ScoreManager {
   private:
    inline static int FILE_VERSION = 1;

    // see implementation
    struct Helper;

    std::vector<ScoreEntry> highScores_ = {};
    std::vector<ScoreEntry> recentScores_ = {};

    std::string filename;

    void read_(std::istream& input);

    void write_(std::ostream& output);

    void sort_();

   public:
    // FIXME: Need scores per course, not just a single one
    ScoreManager(std::string filename);
    ScoreEntry highScore() const {
        return highScores_.empty() ? ScoreEntry{.valid = false} : highScores_[0];
    }

    ScoreEntry lastHighScore() const {
        if (highScores_.empty()) return {.valid = false};
        if (recentScores_.empty() || highScores_[0].timestamp != recentScores_[0].timestamp) {
            // hi score is same as the current score
            return highScores_[0];
        }
        return highScores_.size() < 2 ? ScoreEntry{.valid = false} : highScores_[1];
    }

    ScoreEntry
    lastScore() const {
        return recentScores_.empty() ? ScoreEntry{.valid = false} : recentScores_[0];
    }

    const std::vector<ScoreEntry>& highScores() const {
        return highScores_;
    }

    const std::vector<ScoreEntry>& recentScores() const {
        return recentScores_;
    }

    void save();

    void add(ScoreEntry score) {
        highScores_.push_back(score);
        recentScores_.push_back(score);
        sort_();
    }
};