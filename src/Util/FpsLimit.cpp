#include "FpsLimit.h"

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>

#include <cmath>
#include <thread>

FpsLimiter::FpsLimiter() {
    handle_ = CreateWaitableTimer(NULL, FALSE, NULL);
}

void FpsLimiter::setTarget(double interval) {
    target_ = interval;
}

void FpsLimiter::start(double current_time_sec) {
    startTime_ = current_time_sec;
}

void FpsLimiter::end(double current_time_sec) {
    double elapsed = current_time_sec - startTime_;
    double ms_left = (target_ - elapsed) * 1000.0;
    if (ms_left <= 0.1) return;

    int64_t time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    int64_t wait_time = (int64_t)std::floor(ms_left * freq / 1000);
    // busy waiting because no sleep function is precise enough
    do {
        QueryPerformanceCounter((LARGE_INTEGER *)&time2);
        _mm_pause();
    } while ((time2 - time1) < wait_time);
}
#else
#include <time.h>
#include <x86intrin.h>

#include <cmath>

FpsLimiter::FpsLimiter() {
}

void FpsLimiter::setTarget(double interval) {
    target_ = interval;
}

void FpsLimiter::start(double current_time_sec) {
    startTime_ = current_time_sec;
}

void FpsLimiter::end(double current_time_sec) {
    double elapsed = current_time_sec - startTime_;
    double ms_left = (target_ - elapsed) * 1000.0;
    if (ms_left <= 0.1) return;

    struct timespec time1, time2;
    clock_gettime(CLOCK_MONOTONIC, &time1);

    int64_t wait_time = (int64_t)std::floor(ms_left * 1e6);  // Convert ms_left to nanoseconds
    // busy waiting because no sleep function is precise enough
    do {
        clock_gettime(CLOCK_MONOTONIC, &time2);
        _mm_pause();
    } while ((time2.tv_sec - time1.tv_sec) * 1e9 + (time2.tv_nsec - time1.tv_nsec) < wait_time);
}
#endif