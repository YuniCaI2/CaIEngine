#include <chrono>

namespace FrameWork {
    class Timer {
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;
    public:
        using Duration = Clock::duration;

    public:
        Timer() : startTime(Clock::now()) {}
        void Restart() {
            startTime = Clock::now();
        }

        Duration GetElapsed() const {
            return Clock::now() - startTime;
        }

        double GetElapsedMilliTime() const {
            return std::chrono::duration<double, std::milli>(GetElapsed()).count();
        }

        double GetElapsedSeconds() const {
            return GetElapsedMilliTime() / 1000.0f;
        }

    private:
        TimePoint startTime;
    };
}
