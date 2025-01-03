#ifndef __GPS_h
#define __GPS_h

#include "Adafruit_GPS.h"
#include "Particle.h"

class GPS {
   public:
    GPS();
    void start();
    void suspend();
    void resume();
    void toJSON(JSONWriter* json);
    bool hasFix() { return _gps.fix; }

   private:
    static void threadproc(void* param);
    void run();

    enum State {
        STATE_STOPPED = 0,
        STATE_RUNNING,
        STATE_SUSPENDING,
        STATE_SUSPENDED,
        STATE_RESUMING,
    };

   private:
    Adafruit_GPS _gps;
    State _state;
    Thread* _thread;
    Mutex _mutex;
};

#endif  // def(__GPS_h)