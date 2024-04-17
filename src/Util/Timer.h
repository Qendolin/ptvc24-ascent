#pragma once

struct Timer {
   private:
    float value_ = 0.0;

   public:
    Timer() = default;

    void set(float seconds) {
        value_ = seconds;
        if (value_ < 0) value_ = 0;
    }

    void add(float seconds) {
        value_ += seconds;
        if (value_ < 0) value_ = 0;
    }

    void update(float delta) {
        value_ -= delta;
        if (value_ < 0) value_ = 0;
    }

    float value() {
        return value_;
    }

    bool isZero() {
        return value_ == 0.0;
    }

    operator float() const {
        return value_;
    }

    Timer& operator+=(const float& rhs) {
        value_ += rhs;
        return *this;
    }

    Timer& operator=(const float& rhs) {
        value_ = rhs;
        return *this;
    }
};
