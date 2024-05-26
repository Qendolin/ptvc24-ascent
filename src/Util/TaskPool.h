#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <stacktrace>
#include <string>
#include <thread>
#include <utility>

class TaskCompletionView {
   public:
    virtual bool isFinished() = 0;
    virtual bool isStarted() = 0;
};

template <class T>
class TaskPool : public TaskCompletionView {
   private:
    std::atomic<int> running_{0};
    std::atomic<bool> started_{false};
    std::vector<std::thread> threads_;
    int threadCount_;
    std::vector<std::function<void(T& out)>> operations_;
    std::mutex mutex_;

    T result_ = {};

   public:
    TaskPool() {
        threadCount_ = std::thread::hardware_concurrency() - 1;
        running_.store(threadCount_);
    }

    ~TaskPool() {
        if (started_.load()) {
            for (auto&& thread : threads_) {
                if (thread.joinable())
                    thread.detach();
            }
        }
    }

    bool isFinished() override {
        return running_.load() == 0;
    }

    bool isStarted() override {
        return started_.load();
    }

    T& result() {
        if (running_.load() != 0) {
            std::runtime_error("Tasks must be finished!\n" + std::to_string(std::stacktrace::current()));
        }
        return result_;
    }

    void runAsync() {
        if (started_.exchange(true))
            return;

        for (int i = 0; i < threadCount_; i++) {
            threads_.emplace_back(
                std::thread([this]() {
                    while (true) {
                        std::function<void(T & out)> operation;
                        {
                            std::unique_lock<std::mutex> lock(mutex_);
                            if (operations_.empty()) {
                                break;
                            }
                            operation = operations_.back();
                            operations_.pop_back();
                        }
                        operation(result_);
                    }
                    running_.fetch_sub(1);
                }));
        }
    }

    void add(const std::function<void(T& out)>& operation) {
        if (started_.load()) {
            std::runtime_error("Cannot add tasks after start!\n" + std::to_string(std::stacktrace::current()));
        }
        operations_.push_back(operation);
    }

    void runSync() {
        if (started_.exchange(true))
            return;

        for (auto&& op : operations_) {
            op(result_);
        }

        running_.store(0);
    }
};
