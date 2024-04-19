#pragma once

#include <fstream>
#include <string>
#include <vector>

struct ScoreEntry {
    bool invalid = true;
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
    ScoreEntry highScore_ = {};
    ScoreEntry lastScore_ = {};  // most recent score

    std::string filename;

    void read_(std::istream& input);

    void write_(std::ostream& output);

   public:
    ScoreManager(std::string filename);

    ScoreEntry highScore() const {
        return highScore_;
    }

    ScoreEntry lastScore() const {
        return lastScore_;
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
        if (score.total < highScore_.total)
            highScore_ = score;
        recentScores_.push_back(score);
        if (score.timestamp > lastScore_.timestamp)
            lastScore_ = score;
    }
};