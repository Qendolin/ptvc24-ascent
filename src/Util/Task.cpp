#include "Task.h"

#include "Log.h"

// This avoids including "PANIC" in the header
void assertTrue(std::atomic<bool> &value, std::string message) {
    if (!value.load()) {
        PANIC(message);
    }
}