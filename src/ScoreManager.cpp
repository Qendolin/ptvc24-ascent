#include "ScoreManager.h"

#include <tortellini.h>

#include <algorithm>
#include <filesystem>

#include "Loader/Loader.h"
#include "Util/Log.h"

ScoreManager::ScoreManager(std::string filename) : filename(filename) {
    // ensure file is created
    std::fstream file(filename, std::ios::out | std::ios::app);
    if (!file) {
        PANIC("Could not create score file");
    }
    file.close();
    file = std::fstream(filename, std::ios::in);
    read_(file);
    file.close();
    file = std::fstream(filename, std::ios::out | std::ios::trunc);
    write_(file);
    file.close();
}

// Just some private helper function that I don't want to declare in the header
struct ScoreManager::Helper {
    static ScoreEntry parseScore_(const tortellini::ini::section& section) {
        ScoreEntry entry = {
            .valid = true,
            .timestamp = section["timestamp"] | 0ULL,
            .course = section["course"] | "",
            .flight = section["flight"] | 0.0f,
            .penalty = section["penalty"] | 0.0f,
            .total = section["total"] | 0.0f,
        };

        int split_count = section["split_count"] | 0;
        for (int j = 0; j < split_count; j++) {
            float time = section["split_" + std::to_string(j)] | 0.0f;
            entry.splits.push_back(time);
        }
        return entry;
    }

    static void serializeScore_(const tortellini::ini::section& section, const ScoreEntry& entry) {
        section["timestamp"] = entry.timestamp;
        section["course"] = entry.course;
        section["flight"] = entry.flight;
        section["penalty"] = entry.penalty;
        section["total"] = entry.total;
        section["split_count"] = entry.splits.size();

        for (int j = 0; j < entry.splits.size(); j++) {
            section["split_" + std::to_string(j)] = entry.splits[j];
        }
    }
};

void ScoreManager::read_(std::istream& input) {
    tortellini::ini ini;
    input >> ini;

    recentScores_.clear();
    highScores_.clear();

    int version = ini["info"]["version"] | 0;
    if (version == 0) {
        // no data
        return;
    } else if (version != FILE_VERSION) {
        // version is incompatible
        LOG_INFO("Incompatible score file version: " + std::to_string(version));
        return;
    }

    int recent_entry_count = ini["info"]["recent_entry_count"] | 0;
    for (int i = 0; i < recent_entry_count; i++) {
        const auto& section = ini["recent_entry_" + std::to_string(i)];
        ScoreEntry entry = Helper::parseScore_(section);
        recentScores_.push_back(entry);
    }

    int high_entry_count = ini["info"]["high_entry_count"] | 0;
    for (int i = 0; i < high_entry_count; i++) {
        const auto& section = ini["high_entry_" + std::to_string(i)];
        ScoreEntry entry = Helper::parseScore_(section);
        highScores_.push_back(entry);
    }

    sort_();
}

void ScoreManager::sort_() {
    // sort scores by decending timestamp (recent scores first)
    std::sort(recentScores_.begin(), recentScores_.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.timestamp > b.timestamp;
    });

    // sort scores by decending timestamp (best scores first)
    std::sort(highScores_.begin(), highScores_.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.total < b.total;
    });
}

void ScoreManager::write_(std::ostream& output) {
    tortellini::ini ini;

    sort_();
    // keep only the 100 most recent scores
    if (recentScores_.size() > 100) {
        recentScores_.resize(100);
    }

    // keep only the 100 best scores
    if (highScores_.size() > 100) {
        highScores_.resize(100);
    }

    ini["info"]["version"] = FILE_VERSION;
    ini["info"]["recent_entry_count"] = recentScores_.size();
    for (int i = 0; i < recentScores_.size(); i++) {
        const auto& entry = recentScores_[i];
        const auto& section = ini["recent_entry_" + std::to_string(i)];
        Helper::serializeScore_(section, entry);
    }

    ini["info"]["high_entry_count"] = highScores_.size();
    for (int i = 0; i < highScores_.size(); i++) {
        const auto& entry = highScores_[i];
        const auto& section = ini["high_entry_" + std::to_string(i)];
        Helper::serializeScore_(section, entry);
    }
    output << ini;
}

void ScoreManager::save() {
    std::fstream file = std::fstream(filename, std::ios::out | std::ios::trunc);
    write_(file);
    file.close();
}