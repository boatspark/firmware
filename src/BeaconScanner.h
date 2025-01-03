#ifndef __BeaconScanner_h__
#define __BeaconScanner_h__

#define BEACONSCANNER_DEFAULT_TIMEOUT_SECONDS (60)
#define BEACONSCANNER_RESULT_MAX (100)

class BeaconValue {
   public:
    BeaconValue() : _value(0), _lastupdate(0), _updated(false){};
    operator int16_t() { return value(); }

    // retrieve value and reset updated
    int16_t value() {
        _updated = false;
        return _value;
    }

    // set new value and mark as updated
    void update(int16_t newValue) {
        _value = newValue;
        _updated = true;
        _lastupdate = millis();
    }

    // isupdated() returns true if the value changed since the last retrieval
    bool isupdated() const { return _updated; }

    // isvalid() returns true if the value was ever set
    bool isvalid() const { return _lastupdate > (system_tick_t)0; }

    system_tick_t lastupdate() const { return _lastupdate; }
    system_tick_t age() const { return millis() - lastupdate(); }

   private:
    int16_t _value;
    system_tick_t _lastupdate;
    bool _updated;
};

class Beacon {
   public:
    Beacon(BleAddress address) : rssi(0) { this->address = address; }
    BleAddress address;
    BeaconValue temperature;
    BeaconValue humidity;
    BeaconValue batterylevel;
    BeaconValue voltage;
    int8_t rssi;

    system_tick_t lastupdate() const {
        return max(max(temperature.lastupdate(), humidity.lastupdate()),
                   max(batterylevel.lastupdate(), voltage.lastupdate()));
    }
    bool iscomplete() const {
        return temperature.isvalid() && humidity.isvalid() && batterylevel.isvalid() &&
               voltage.isvalid();
    }
    bool isupdate() const {
        return temperature.isupdated() | humidity.isupdated() || batterylevel.isupdated() ||
               voltage.isupdated();
    }

    void toJSON(JSONWriter* json);
};

class BeaconScanner {
   public:
    BeaconScanner() : _run(false), _thread(nullptr){};
    void start();
    void stop();
    void toJSON(JSONWriter* json);
    int beaconsTracked() { return _beacons.size(); }
    int beaconsComplete();

   private:
    static void threadproc(void* param);
    void setup();
    void scan();
    void processResult(const BleScanResult* scanResult);
    void purgeBeacons(int maxAgeSeconds);
    size_t getServiceData(const BleScanResult* scanResult, uint8_t* buf, size_t len) const;
    bool isBeacon(const uint8_t* buf, size_t len) const;
    void populateBeacon(Beacon& beacon, const uint8_t* serviceData, size_t len);
    bool parseField(uint8_t objectId,
                    const uint8_t* buf,
                    size_t& offset,
                    size_t length,
                    int16_t& value);

   private:
    BleScanParams _scanParams;
    BleScanResult _scanResults[BEACONSCANNER_RESULT_MAX];
    Vector<Beacon> _beacons;
    bool _run;
    Thread* _thread;
    Mutex _mutex;
};

#endif  // __BeaconScanner_h__