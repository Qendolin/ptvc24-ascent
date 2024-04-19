#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <utility>
#include <string>

class TaskCompletionView {
   public:
    virtual bool isFinished() = 0;
    virtual bool isStarted() = 0;
};

void assertTrue(std::atomic<bool>& value, std::string message);

template <class T>
class Task : public TaskCompletionView {
   private:
    std::atomic<bool> finished_{false};
    std::atomic<bool> started_{false};
    std::thread thread_;
    std::function<void(T& out)> callback_;
    T result_ = {};

    static void checkFinished_();

   public:
    template <typename Func, typename... Args>
    Task(Func&& function, Args&&... args) {
        callback_ = [=](T& out) {
            std::invoke(std::forward<Func>(function), out, args...);
        };
    }

    ~Task() {
        if (started_.load() && thread_.joinable())
            thread_.detach();
    }

    bool isFinished() override {
        return finished_.load();
    }

    bool isStarted() override {
        return started_.load();
    }

    T& result() {
        assertTrue(finished_, "Task must be finished");
        return result_;
    }

    void runAsync() {
        if (started_.exchange(true))
            return;
        thread_ = std::thread([this]() {
            callback_(result_);
            finished_.store(true);
        });
    }

    void runSync() {
        if (started_.exchange(true))
            return;
        callback_(result_);
        finished_.store(true);
    }
};
