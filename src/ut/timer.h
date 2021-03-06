#ifndef _UT_TIMER_H_
#define _UT_TIMER_H_

#include <vector>
#include <chrono>

namespace utils{
class Timer{
    private:
        static std::vector<Timer> _timers;
        std::chrono::time_point<std::chrono::steady_clock> _time;
        bool _active;
    public:
        Timer(){
            _active = false;
        }
        bool active() { return _active; }
        void active(bool a) { _active = a; }
        static unsigned start();
        static double time(unsigned id);
        static double reset(unsigned id);
        static double stop(unsigned id);
};
};

#endif

