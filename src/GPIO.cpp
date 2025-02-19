#include "Particle.h"

#include "GPIO.h"

// (10K/2.7K) voltage divider
#define PIN_ADC_BAT1 A0
#define PIN_ADC_BAT2 A1

// D6 = Shore Power - needs PULLUP, will go to ground when power is on.
#define PIN_SHOREPOWER D2
// D8 = Bilge Pump (10K/2.7K) = HIGH means pump on
#define PIN_BILGEPUMP D4
// D7 = Engine (10K/2.7K) = HIGH means engine running
#define PIN_ENGINE D5

// Milliseconds for the debounce function
#define DEBOUNCE_INTERVAL 100

GPIOmonitor::GPIOmonitor()
    : _shorePowerCycles(0),
      _bilgePumpCycles(0),
      _engineCycles(0),
      _bat1(0),
      _bat2(0),
      _alert(ALERT_NONE) {}

void GPIOmonitor::setup() {
    _shorePower.interval(DEBOUNCE_INTERVAL);
    _shorePower.attach(PIN_SHOREPOWER, INPUT_PULLUP);

    _bilgePump.interval(DEBOUNCE_INTERVAL);
    _bilgePump.attach(PIN_BILGEPUMP, INPUT);

    _engine.interval(DEBOUNCE_INTERVAL);
    _engine.attach(PIN_ENGINE, INPUT);

    // Don't need to set pinMode for analog inputs
}

void GPIOmonitor::loop() {
    _engine.update();
    _shorePower.update();
    _bilgePump.update();
    if (_engine.rose()) {
        _engineCycles++;
        _alert |= ALERT_ENGINE_ON;
    }
    if (_engine.fell()) {
        _alert |= ALERT_ENGINE_OFF;
    }
    if (_shorePower.rose()) {
        _shorePowerCycles++;
        _alert |= ALERT_SHOREPOWER_LOST;
    }
    if (_shorePower.fell()) {
        _alert |= ALERT_SHOREPOWER_RESTORED;
    }
    if (_bilgePump.rose()) {
        _bilgePumpCycles++;
        _alert |= ALERT_BILGEPUMP;
    }
    _bat1 = convertADCtoV100(analogRead(PIN_ADC_BAT1));
    _bat2 = convertADCtoV100(analogRead(PIN_ADC_BAT2));
}

void GPIOmonitor::toJSON(JSONWriter* json) {
    json->name("gpio");
    json->beginObject();
    json->name("b1").value(battery1());
    json->name("b2").value(battery2());
    json->name("sp").value(ison_shorePower() ? 1 : 0);
    json->name("spc").value(_shorePowerCycles);
    json->name("bp").value(ison_bilgePump() ? 1 : 0);
    json->name("bpc").value(_bilgePumpCycles);
    json->name("eng").value(ison_engine() ? 1 : 0);
    json->name("engc").value(_engineCycles);
    json->endObject();
}

uint32_t GPIOmonitor::convertADCtoV100(uint32_t v) {
    // Convert ADC input reading to voltage * 100
    // Voltage divider is 10K/2.7K

    return (uint32_t)((double)(v * 33 * 127) / (27.0 * 409.5));
}
