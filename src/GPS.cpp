#include "GPS.h"

// The GPS module on the Adafruit Wing is connected to Serial1
#define GPSSerial Serial1

GPS::GPS() : _gps(&GPSSerial), _state(STATE_STOPPED), _thread(nullptr) {}

void GPS::start() {
    if (_state != STATE_STOPPED) return;
    _gps.begin(9600);
    _gps.sendCommand("");  // make sure resumes if suspended
    _state = STATE_RUNNING;
    if (_thread == nullptr) {
        _thread = new Thread("GPS", threadproc, this);
    }
}

void GPS::suspend() {
    if (_state == STATE_STOPPED || _state == STATE_SUSPENDED) return;
    _state = STATE_SUSPENDING;
}

void GPS::resume() {
    if (_state == STATE_STOPPED || _state == STATE_RUNNING) return;
    _state = STATE_RESUMING;
}

/* static */ void GPS::threadproc(void* param) {
    GPS* gps = static_cast<GPS*>(param);
    while (true) {
        switch (gps->_state) {
            case STATE_RUNNING:
                gps->run();
                break;

            case STATE_SUSPENDING:
                gps->run();
                gps->_gps.standby();
                gps->_state = STATE_SUSPENDED;
                gps->_gps.fix = false;
                break;

            case STATE_RESUMING:
                gps->_gps.wakeup();
                gps->_state = STATE_RUNNING;
                break;
        }
        delay(1);
    }
}

void GPS::run() {
    while (_gps.available() > 0) {
        _gps.read();
        if (_gps.newNMEAreceived()) {
            char* last = _gps.lastNMEA();
            _mutex.lock();
            _gps.parse(last);
            _mutex.unlock();
        }
    }
}

void GPS::toJSON(JSONWriter* json) {
    if (json == nullptr) return;
    json->name("gps");
    json->beginObject();

    _mutex.lock();
    char buf[32];
    snprintf(buf, sizeof(buf), "20%02d%02d%02dT%02d:%02d:%02d.%03dZ", _gps.year, _gps.month,
             _gps.day, _gps.hour, _gps.minute, _gps.seconds, _gps.milliseconds);
    json->name("t").value(buf);
    json->name("fix").value(_gps.fix ? 1 : 0);
    if (_gps.fix) {
        json->name("sat").value(_gps.satellites);
        json->name("lat").value(_gps.latitude_fixed);   // divide by 10M to get degrees
        json->name("lng").value(_gps.longitude_fixed);  // divide by 10M to get degrees
        json->name("sog").value(_gps.speed, 2);
        json->name("cog").value(_gps.angle, 0);
        json->name("alt").value(_gps.altitude, 0);
        json->name("pdop").value(_gps.PDOP, 2);
        json->name("q").value(_gps.fixquality);
        json->name("q3").value(_gps.fixquality_3d);
    }
    _mutex.unlock();
    json->endObject();
}
