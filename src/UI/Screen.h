#pragma once

class Screen {
   private:
    bool closed_ = false;

   public:
    // prevent copy
    Screen(Screen const&) = delete;
    Screen& operator=(Screen const&) = delete;

    Screen() {}
    virtual ~Screen() {}

    virtual void draw() = 0;

    void close() {
        closed_ = true;
    }

    bool isClosed() {
        return closed_;
    }
};