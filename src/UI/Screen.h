#pragma once

class Screen {
   protected:
    bool opened_ = false;
    bool flag_ = false;

    virtual void draw_() = 0;

   public:
    // prevent copy
    Screen(Screen const&) = delete;
    Screen& operator=(Screen const&) = delete;

    Screen() {}
    virtual ~Screen() {}

    virtual void close() {
        opened_ = false;
        flag_ = true;
    }

    /**
     * Set when the screen is closed until it is reset
     */
    bool flagged() {
        return flag_;
    }

    /**
     * Clears the flag and returns its previous value
     */
    bool resetFlag() {
        bool value = flag_;
        flag_ = false;
        return value;
    }

    void draw() {
        if (opened_)
            draw_();
    }

    bool closed() {
        return !opened_;
    }

    bool opened() {
        return opened_;
    }
};