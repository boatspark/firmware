#ifndef __TIMER_H
#define __TIMER_H

class MilliTimer {
   public:
    MilliTimer(system_tick_t period, bool runFirstTime = false)
        : _period(period), _runFirstTime(runFirstTime), _last(0) {}
    bool timeout() {
        if (_runFirstTime) {
            _runFirstTime = false;
        } else if (millis() - _last < _period) {
            return false;
        }
        _last = millis();
        return true;
    }
    system_tick_t remaining() { return _period - (millis() - _last); }

   private:
    system_tick_t _period;
    bool _runFirstTime;
    system_tick_t _last;
};

#endif