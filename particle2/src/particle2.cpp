#include <Arduino.h>
#include <math.h>
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
#include "Air_Quality_Sensor.h"
#include <SensirionI2CSgp40.h>

GAS_GMXXX<TwoWire> gas;

// Define pins and constants
#define DUST_SENSOR_PIN D4
#define SENSOR_READING_INTERVAL 30000

// Dust sensor variables
unsigned long lastInterval;
unsigned long lowpulseoccupancy = 0;
unsigned long last_lpo = 0;
unsigned long duration;
float ratio = 0;
float concentration = 0;

// Air quality sensor
AirQualitySensor sensor(A0);

// VOC gas sensor
SensirionI2CSgp40 sgp40;

void setup()
{
    Serial.begin(9600);

    // Dust sensor setup
    pinMode(DUST_SENSOR_PIN, INPUT);
    lastInterval = millis();

    // Air quality sensor setup
    if (sensor.init()) {
        Serial.println("Air quality sensor ready.");
    } else {
        Serial.println("Air quality sensor ERROR!");
    }

    // Gas sensor setup
    gas.begin(Wire, 0x08);

    // VOC gas sensor setup
    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    sgp40.begin(Wire);

    uint16_t serialNumber[3];
    uint8_t serialNumberSize = 3;

    error = sgp40.getSerialNumber(serialNumber, serialNumberSize);

    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SerialNumber:");
        Serial.print("0x");
        for (size_t i = 0; i < serialNumberSize; i++) {
            uint16_t value = serialNumber[i];
            Serial.print(value < 4096 ? "0" : "");
            Serial.print(value < 256 ? "0" : "");
            Serial.print(value < 16 ? "0" : "");
            Serial.print(value, HEX);
        }
        Serial.println();
    }

    uint16_t testResult;
    error = sgp40.executeSelfTest(testResult);
    if (error) {
        Serial.print("Error trying to execute executeSelfTest(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (testResult != 0xD400) {
        Serial.print("executeSelfTest failed with error: ");
        Serial.println(testResult);
    }
}

void getVOCgasReading(){
    uint16_t error;
    char errorMessage[256];
    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t srawVoc = 0;

    delay(1000);

    error = sgp40.measureRawSignal(defaultRh, defaultT, srawVoc);
    if (error) {
        Serial.print("Error trying to execute measureRawSignal(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SRAW_VOC:");
        Serial.println(srawVoc);
        Particle.publish("SRAW_VOC", String(srawVoc), PRIVATE);
    }
}

void getGasReading(){
    uint32_t val = 0;

    val = gas.getGM102B();
    Serial.print("GM102B carbon monoxide (CO): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    Particle.publish("GM102B_CO", String(val), PRIVATE);

    delay(1000); // Adding delay to prevent publish limits

    val = gas.getGM302B();
    Serial.print("GM302B ammonia (NH3): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    Particle.publish("GM302B_NH3", String(val), PRIVATE);

    delay(1000); // Adding delay to prevent publish limits

    val = gas.getGM502B();
    Serial.print("GM502B nitrogen dioxide (NO2): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    Particle.publish("GM502B_NO2", String(val), PRIVATE);

    delay(1000); // Adding delay to prevent publish limits

    val = gas.getGM702B();
    Serial.print("GM702B volatile organic compounds (VOCs): ");
    Serial.print(val);
    Serial.print("  =  ");
    Serial.print(gas.calcVol(val));
    Serial.println("V");
    Particle.publish("GM702B_VOCs", String(val), PRIVATE);
}

void getAirQualityReadings(){
    int quality = sensor.slope();

    Serial.print("Sensor value: ");
    Serial.println(sensor.getValue());
    Particle.publish("AirQuality_SensorValue", String(sensor.getValue()), PRIVATE);

    if (quality == AirQualitySensor::FORCE_SIGNAL) {
        Serial.println("High pollution! Force signal active.");
        Particle.publish("AirQuality_Status", "High pollution! Force signal active.", PRIVATE);
    } else if (quality == AirQualitySensor::HIGH_POLLUTION) {
        Serial.println("High pollution!");
        Particle.publish("AirQuality_Status", "High pollution!", PRIVATE);
    } else if (quality == AirQualitySensor::LOW_POLLUTION) {
        Serial.println("Low pollution!");
        Particle.publish("AirQuality_Status", "Low pollution!", PRIVATE);
    } else if (quality == AirQualitySensor::FRESH_AIR) {
        Serial.println("Fresh air.");
        Particle.publish("AirQuality_Status", "Fresh air.", PRIVATE);
    }
}

void getDustSensorReadings()
{
    // Dust
    if (lowpulseoccupancy == 0)
    {
        lowpulseoccupancy = last_lpo;
    }
    else
    {
        last_lpo = lowpulseoccupancy;
    }

    ratio = lowpulseoccupancy / (SENSOR_READING_INTERVAL * 10.0);
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;

    Serial.printlnf("dust:");
    Serial.printlnf("LPO: %d", lowpulseoccupancy);
    Serial.printlnf("Ratio: %f%%", ratio);
    Serial.printlnf("Concentration: %f pcs/L", concentration);

    Particle.publish("Dust_LPO", String(lowpulseoccupancy), PRIVATE);
    Particle.publish("Dust_Ratio", String(ratio), PRIVATE);
    Particle.publish("Dust_Concentration", String(concentration), PRIVATE);
}

void loop()
{
    // Dust sensor
    duration = pulseIn(DUST_SENSOR_PIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - lastInterval) > SENSOR_READING_INTERVAL)
    {
        getDustSensorReadings();
        delay(1000); // Adding delay to prevent publish limits
        getAirQualityReadings();
        delay(1000); // Adding delay to prevent publish limits
        getGasReading();
        delay(1000); // Adding delay to prevent publish limits
        getVOCgasReading();

        lowpulseoccupancy = 0;
        lastInterval = millis();
    }
}
