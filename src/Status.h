#ifndef __STATUS_H
#define __STATUS_H

#define PIN_CONNECTED D0
#define PIN_BEACONS D1
#define PIN_GPS D2

class Status {
   public:
    static void setup() {
        pinMode(PIN_CONNECTED, OUTPUT);
        pinMode(PIN_BEACONS, OUTPUT);
        pinMode(PIN_GPS, OUTPUT);
        loop(false, false, false);
    }

    static void loop(bool isConnected, bool isBeacons, bool isGPS) {
        digitalWrite(PIN_CONNECTED, isConnected ? HIGH : LOW);
        digitalWrite(PIN_BEACONS, isBeacons ? HIGH : LOW);
        digitalWrite(PIN_GPS, isGPS ? HIGH : LOW);
    }
};

#endif  // __STATUS_H