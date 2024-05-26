#ifdef _WIN32
class FpsLimiter {
   private:
    void* handle_ = nullptr;
    double startTime_ = 0.0;
    double target_ = 1.0 / 100.0;

   public:
    FpsLimiter();

    void setTarget(double interval);

    void start(double current_time_ms);

    void end(double current_time_sec);
};

#else

class FpsLimiter {
   private:
   public:
    FpsLimiter();

    void setTarget(double interval);

    void start(double current_time_ms);

    void end(double current_time_ms);
};

#endif