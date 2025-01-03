#ifndef __GPIO_H
#define __GPIO_H

#include "Debounce.h"

class GPIOmonitor {
   public:
    GPIOmonitor();
    void setup();
    void loop();

    bool ison_shorePower() { return !_shorePower.read(); }
    bool ison_bilgePump() { return _bilgePump.read(); }
    bool ison_engine() { return _engine.read(); }
    int32_t battery1() { return _bat1; }
    int32_t battery2() { return _bat2; }
    uint8_t alert() {
        uint8_t s = _alert;
        _alert = ALERT_NONE;
        return s;
    }

    void toJSON(JSONWriter* json);

    enum AlertState {
        ALERT_NONE = 0,
        ALERT_BILGEPUMP = 1,
        ALERT_SHOREPOWER_LOST = 2,
        ALERT_SHOREPOWER_RESTORED = 4,
        ALERT_ENGINE_ON = 8,
        ALERT_ENGINE_OFF = 16,
    };

   private:
    uint32_t convertADCtoV100(uint32_t v);

   private:
    Debounce _shorePower;
    int32_t _shorePowerCycles;
    Debounce _bilgePump;
    int32_t _bilgePumpCycles;
    Debounce _engine;
    int32_t _engineCycles;
    int32_t _bat1;
    int32_t _bat2;
    uint8_t _alert;
};

#endif  // __GPIO_H