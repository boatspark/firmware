#include "Particle.h"

#include "BeaconScanner.h"

int16_t littleEndianToInt16(const uint8_t* data) {
    return (int16_t)((data[1] << 8) | data[0]);
}

void BeaconScanner::start() {
    if (_run) return;
    setup();
    _run = true;
    if (_thread == nullptr) {
        _thread = new Thread("BeaconScanner", threadproc, this);
    }
}

void BeaconScanner::stop() {
    _run = false;
}

/* static */ void BeaconScanner::threadproc(void* param) {
    BeaconScanner* scanner = static_cast<BeaconScanner*>(param);
    while (true) {
        if (scanner->_run) {
            scanner->scan();
        }
        delay(1);
    }
}

void BeaconScanner::setup() {
    BLE.selectAntenna(BleAntennaType::INTERNAL);
    BLE.on();
    _scanParams.version = BLE_API_VERSION;
    _scanParams.size = sizeof(BleScanParams);
    BLE.getScanParameters(&_scanParams);  // Get the default scan parameters
    _scanParams.timeout = 100;            // Change timeout to 1 second
    _scanParams.window = 160;
    _scanParams.scan_phys = BLE_PHYS_1MBPS;
    _scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    BLE.setScanParameters(&_scanParams);  // Set the modified scan parameters
}

void BeaconScanner::scan() {
    BLE.setScanParameters(&_scanParams);  // Set the modified scan parameters
    int count = BLE.scan(_scanResults, BEACONSCANNER_RESULT_MAX);

    _mutex.lock();
    for (int i = 0; i < count; i++) {
        processResult(_scanResults + i);
    }
    _mutex.unlock();
    purgeBeacons(BEACONSCANNER_DEFAULT_TIMEOUT_SECONDS);
}

void BeaconScanner::toJSON(JSONWriter* json) {
    _mutex.lock();
    if (json == NULL) return;
    json->name("beacons");
    json->beginArray();

    for (int i = 0; i < _beacons.size(); i++) {
        Beacon& beacon = _beacons.at(i);
        if (beacon.iscomplete()) {
            beacon.toJSON(json);
        }
    }

    json->endArray();
    _mutex.unlock();
}

void Beacon::toJSON(JSONWriter* json) {
    if (json == NULL) return;
    char mac[18];
    address.toString(mac, sizeof(mac));
    json->beginObject();
    json->name("mac").value(mac);
    json->name("rssi").value(rssi);
    json->name("t").value(temperature.value());
    json->name("h").value(humidity.value());
    json->name("b").value(batterylevel.value());
    json->name("v").value(voltage.value());
    json->endObject();
}

int BeaconScanner::beaconsComplete() {
    _mutex.lock();
    int count = 0;
    for (int i = 0; i < _beacons.size(); i++) {
        if (_beacons.at(i).iscomplete()) count++;
    }
    _mutex.unlock();
    return count;
}

void BeaconScanner::processResult(const BleScanResult* scanResult) {
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    size_t count = getServiceData(scanResult, buf, sizeof(buf));
    if (!isBeacon(buf, count)) return;  // not a BTHome beacon

    uint8_t i;
    for (i = 0; i < _beacons.size(); i++) {
        Beacon& beacon = _beacons.at(i);
        if (beacon.address == scanResult->address()) {
            // Found existing beacon to update
            beacon.rssi = scanResult->rssi();
            populateBeacon(beacon, buf, count);
            return;
        }
    }

    // New beacon to add
    Beacon beacon(scanResult->address());
    beacon.rssi = scanResult->rssi();
    populateBeacon(beacon, buf, count);
    _beacons.append(beacon);
}

void BeaconScanner::purgeBeacons(int maxAgeSeconds) {
    // Find the oldest allowed update time for the max age
    // Beacons that haven't been heard from since will be purged
    system_tick_t oldestAllowed = millis() - 1000 * maxAgeSeconds;

    _mutex.lock();
    for (int i = 0; i < _beacons.size(); i++) {
        // Need to cast to signed int32_t to allow for overflow in the comparison
        if (static_cast<int32_t>(_beacons.at(i).lastupdate()) <
            static_cast<int32_t>(oldestAllowed)) {
            _beacons.removeAt(i);
            i--;
        }
    }
    _mutex.unlock();
}

bool BeaconScanner::isBeacon(const uint8_t* buf, size_t len) const {
    // BTHome (min length is 9, ID is 0xd2fc, device info is 0x40)
    return len >= 9 && buf[0] == 0xd2 && buf[1] == 0xfc && (buf[2] & 0xe5) == 0x40;
}

size_t BeaconScanner::getServiceData(const BleScanResult* scanResult,
                                     uint8_t* buf,
                                     size_t len) const {
    BleAdvertisingData data = scanResult->advertisingData();
    return data.get(BleAdvertisingDataType::SERVICE_DATA, buf, len);
}

void BeaconScanner::populateBeacon(Beacon& beacon, const uint8_t* serviceData, size_t len) {
    size_t offset = 3;
    int16_t value;
    while (offset + 1 < len) {
        uint8_t id = serviceData[offset++];
        bool r = parseField(id, serviceData, offset, len, value);
        if (r) {
            switch (id) {
                case 0x01:  // battery level
                    beacon.batterylevel.update(value);
                    break;
                case 0x02:  // temperature
                    beacon.temperature.update(value);
                    break;
                case 0x03:  // humidity
                    beacon.humidity.update(value);
                    break;
                case 0x0c:  // voltage
                    beacon.voltage.update(value);
                    break;
            }
        }
    }
}

bool BeaconScanner::parseField(uint8_t objectId,
                               const uint8_t* buf,
                               size_t& offset,
                               size_t length,
                               int16_t& value) {
    value = 0;
    bool result = false;

    // See https://bthome.io/format/#supported-data
    switch (objectId) {
        case 0x00:  // packet id
        case 0x01:  // battery level
        case 0x09:  // count
        case 0x2e:  // humidity
        case 0x2f:  // moisture
        case 0x46:  // UV index
        case 0x15:  // battery
        case 0x16:  // battery charging
        case 0x17:  // carbon monoxide
        case 0x18:  // cold
        case 0x19:  // connectivity
        case 0x1a:  // door
        case 0x1b:  // garage door
        case 0x1c:  // gas
        case 0x0f:  // generic boolean
        case 0x1d:  // heat
        case 0x1e:  // light
        case 0x1f:  // lock
        case 0x20:  // moisture
        case 0x21:  // motion
        case 0x22:  // moving
        case 0x23:  // occupancy
        case 0x10:  // power
        case 0x25:  // presence
        case 0x26:  // problem
        case 0x27:  // running
        case 0x28:  // safety
        case 0x29:  // smoke
        case 0x2a:  // sound
        case 0x2b:  // tamper
        case 0x2c:  // vibration
        case 0x2d:  // window
        case 0x3a:  // button
            if (offset < length) {
                value = static_cast<int16_t>(static_cast<uint16_t>(buf[offset]));
                result = true;
            }
            offset += 1;
            break;

        case 0x02:  // Temperature (sint16, 2 bytes)
        case 0x03:  // Humidity (uint16, 2 bytes)
        case 0x0c:  // Voltage (uint16, 2 bytes)
        case 0x12:  // co2
        case 0x3d:  // count
        case 0x43:  // current
        case 0x08:  // dewpoint
        case 0x40:  // distance (mm)
        case 0x41:  // distance (m)
        case 0x52:  // gyroscope
        case 0x06:  // mass (kg)
        case 0x07:  // mass (lb)
        case 0x14:  // moisture
        case 0x0d:  // pm2.5
        case 0x0e:  // pm10
        case 0x3f:  // rotation
        case 0x44:  // speed
        case 0x45:  // temperature 0.1
        case 0x13:  // tvoc
        case 0x4a:  // voltage
        case 0x47:  // volume
        case 0x48:  // volume
        case 0x49:  // volume flow rate
        case 0x3c:  // dimmer
        case 0xf0:  // device type id
            if (offset + 1 < length) {
                value = littleEndianToInt16(&buf[offset]);
                result = true;
            }
            offset += 2;
            break;

        // Skip 3 byte fields
        case 0x42:  // duration
        case 0x0a:  // energy
        case 0x4b:  // gas
        case 0x05:  // illuminance
        case 0x0b:  // power
        case 0x04:  // pressure
        case 0xf2:  // firmware version
            offset += 3;
            break;

        // Skip 4 byte fields
        case 0x3e:  // count
        case 0x4d:  // energy
        case 0x4c:  // gas
        case 0x50:  // timestamp
        case 0x55:  // volume storage
        case 0x4f:  // water
        case 0xf1:  // firmware version
            offset += 4;
            break;

        // Skip variable length fields
        case 0x53:  // text
        case 0x54:  // raw
            if (offset < length) {
                offset += static_cast<size_t>(buf[offset]);
            }
            offset += 1;
            break;
    }

    return result;
}